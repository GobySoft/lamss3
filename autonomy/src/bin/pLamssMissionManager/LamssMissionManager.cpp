// Copyright 2014 
// Henrik Schmidt
// Toby Schneider 
// Massachusetts Institute of Technology 

#include <dlfcn.h>

#include <numeric>

#include <boost/thread.hpp>

#include "goby/common/logger.h"
#include "goby/acomms/connect.h"
#include "goby/common/time.h"

#include "LamssMissionManager.h"

#include "goby/moos/moos_protobuf_helpers.h"

using namespace lamss::shared::fsm;
using namespace goby::common::logger;
using namespace goby::common::logger_lock;
namespace gpb = goby::moos::protobuf;
using goby::common::goby_time;
using goby::glog;

const double pi = 3.14159;

LamssMissionManager* LamssMissionManager::inst_ = 0;
void* LamssMissionManager::driver_library_handle_= 0;

int main(int argc, char * argv[])
{
    return goby::moos::run<LamssMissionManager>(argc, argv);
}

LamssMissionManager* LamssMissionManager::get_instance()
{
    if(!inst_)    {
        lamss::shared::fsm::cfg_.reset(new LamssMissionManagerConfig);
        inst_ = new LamssMissionManager(*lamss::shared::fsm::cfg_);
    }
    return inst_;
}

LamssMissionManager::LamssMissionManager(LamssMissionManagerConfig& cfg)
    : GobyMOOSApp(&cfg),
      cfg_(cfg),
      scheduler_(true),
      fsm_(scheduler_.create_processor<LamssMissionManagerFSM>()),
      last_slow_loop_t_(0)
{

    goby::glog.set_lock_action(lock);
    goby::glog.add_group("fsm", goby::common::Colors::lt_magenta);
    goby::glog.add_group("helm", goby::common::Colors::lt_green);
    goby::glog.add_group("mac", goby::common::Colors::lt_blue);
    goby::glog.add_group("gram", goby::common::Colors::yellow);

    // set up subscriptions before starting separate thread
    subscribe("LAMSS_DEPLOY_IN", &LamssMissionManager::handle_deploy, this);
    subscribe("LAMSS_PROSECUTE_IN", &LamssMissionManager::handle_prosecute, this);
    subscribe("VEHICLE_ID", &LamssMissionManager::handle_vehicle_id, this);
    subscribe("IFS_DATA_IN", &LamssMissionManager::handle_ifs_data, this);    

    // intialize MOOSGeodesy

    m_Geodesy.Initialise(cfg_.common().lat_origin(),cfg_.common().lon_origin());

    scheduler_.initiate_processor(fsm_);  
  
    fsm_thread_.reset(new boost::thread(boost::bind(&boost::statechart::fifo_scheduler<>::operator(), &scheduler_, 0)));
  
    glog.is(VERBOSE) && glog << cfg_.DebugString() << std::endl;

}

void LamssMissionManager::loop()
{
    double curr_time = MOOSTime();
    if (last_acomms_ < 1)
      last_acomms_ = curr_time;

    try_publish(); // publish messages queued from state machine

    // process some events at a slower rate
    const int slow_loop_period = 10;
    if(curr_time > last_slow_loop_t_ + slow_loop_period)
        slow_loop(curr_time);


}

void LamssMissionManager::slow_loop(double curr_time)
{
  // Here we should add the check for blackout_duration and prosecute_duration.

  if (prosecute_mode_active_)
    {
      double remaining = (prosecute_time_ + last_prosecute_cmd_.prosecute_duration()) - curr_time;
      if (remaining <= 0.0)
	{
	  //	  LAMSS_DEPLOY new_deploy = last_deploy_cmd_;
	  std::string serialized;
	  if (!MOOSStrCmp(cfg_.deploy_location(),"station"))
	    {
	      last_deploy_cmd_.set_deploy_x(last_nav_.local_fix().x());
	      last_deploy_cmd_.set_deploy_y(last_nav_.local_fix().y());
	    }
	  serialize_for_moos(&serialized,last_deploy_cmd_);    
	  publish("LAMSS_DEPLOY_IN", serialized);
	  glog.is(DEBUG1) && glog 
	    << "Prosecute duration exceeded - commanding Deploy" 
	    << std::endl;
	}
      else 
	{
	  glog.is(DEBUG1) && glog 
	    << "Prosecute time remaining: " << remaining
	    << std::endl;
	  publish("PROSECUTE_REMAINING",remaining);
	}
    }
  if ( mission_active_ )
    {
      if ( !abort_mode_active_ )
	{
	  double remaining = last_acomms_ + last_deploy_cmd_.deploy_duration() - curr_time ;
	  if (remaining <= 0.0)
	    {
	      glog.is(DEBUG1) && glog 
		<< "Deploy duration exceeded - initiating Abort" 
		<< std::endl;
	      abort_requested_ = true;
	      abort_time_ = curr_time;
	      queue_event<EvNewMission>();
	    }
	  else 
	    {
	      glog.is(DEBUG1) && glog 
		<< "Deploy time remaining: " << remaining
		<< std::endl;
	      publish("DEPLOY_REMAINING",remaining);
	      abort_requested_ = false;
	      abort_mode_active_ = false;
	    }
	  update_abort_mission();
	}
      else
	{
	  double remaining =  abort_duration_ - (curr_time - abort_time_);
	  glog.is(DEBUG1) && glog 
	    << "Abort time remaining: " << remaining
	    << std::endl;
	  publish("ABORT_REMAINING",remaining);
	}
    }

  last_slow_loop_t_ = curr_time;
}

void  LamssMissionManager::update_abort_mission()
{
  double range = sqrt(pow(last_nav_.local_fix().x() - cfg_.abort_x(),2)  +
		      pow(last_nav_.local_fix().y() - cfg_.abort_y(),2));
  double heading = atan2(cfg_.abort_x() - last_nav_.local_fix().x(),  
			 cfg_.abort_y() - last_nav_.local_fix().y()) * 180./M_PI;
  double duration = range/cfg_.abort_speed();
  std::stringstream ss;
  ss << "heading=" << heading << "#duration=" << duration ;; 
  publish("ABORT_HEADING", ss.str());
  std::stringstream ss1;
  ss1 << "speed=" << cfg_.abort_speed() ;
  publish("ABORT_SPEED", ss1.str());
  std::stringstream ss2;
  ss2 << "depth=" << cfg_.abort_depth() ;
  publish("ABORT_DEPTH", ss2.str());

  abort_duration_ = duration;
}  

void LamssMissionManager::try_publish()
{
    boost::mutex::scoped_try_lock lk(moos_mutex_);
    if(lk.owns_lock())   
    {
        while(!publish_queue_.empty())
	{
            glog.is(DEBUG1) && glog << "Publishing: " << publish_queue_.front().GetKey() << ": " << publish_queue_.front().GetAsString()  << std::endl;
            publish(publish_queue_.front());
            publish_queue_.pop_front();
	}
    }
}

void LamssMissionManager::handle_ifs_data(const CMOOSMsg& msg)
{
    goby::moos::protobuf::FrontSeatInterfaceData data;
    parse_for_moos(msg.GetString(), &data);

    if(data.has_node_status())
    {
        last_nav_ = data.node_status();
    }
}

void LamssMissionManager::handle_deploy(const CMOOSMsg& msg)
{
  LAMSS_DEPLOY deploy_cmd;
  parse_for_moos(msg.GetString(), &deploy_cmd);
  last_deploy_cmd_ = deploy_cmd;
  deploy_received_ = true;

  if (deploy_cmd._src_id() == cfg_.topside_id())
    last_acomms_ = MOOSTime();

  if (mission_active_)
    queue_event<EvNewMission>();
  else
    queue_event<EvStartMission>();

}

void LamssMissionManager::handle_prosecute(const CMOOSMsg& msg)
{
  LAMSS_PROSECUTE prosecute_cmd;
  parse_for_moos(msg.GetString(), &prosecute_cmd);
  last_prosecute_cmd_ = prosecute_cmd;

  if (prosecute_cmd._src_id() == cfg_.topside_id())
    last_acomms_ = MOOSTime();
  // prosecute only accepted if already in active mission
  if (mission_active_)
    {
      prosecute_received_ = true;
      queue_event<EvNewMission>();
    }
  else
    {
      goby::glog << group("fsm") 
		 << "Not in actictive mission -  Prosecute ignored"
		 << std::endl;
    }
  
}


void LamssMissionManager::handle_vehicle_id(const CMOOSMsg& msg)
{
    boost::mutex::scoped_lock lock(moos_mutex_);
    vehicle_id_ = msg.GetDouble();
}

