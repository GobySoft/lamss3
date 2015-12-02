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
      initial_deploy_done_(false),
      slow_loop_period_(10),
      last_slow_loop_t_(0)
{

    goby::glog.set_lock_action(lock);
    goby::glog.add_group("fsm", goby::common::Colors::lt_magenta);
    goby::glog.add_group("helm", goby::common::Colors::lt_green);
    goby::glog.add_group("mac", goby::common::Colors::lt_blue);
    goby::glog.add_group("gram", goby::common::Colors::yellow);

    set_ignore_stale(false);

    // set up subscriptions before starting separate thread
    subscribe("LAMSS_STATUS_IN", &LamssMissionManager::handle_status, this);
    subscribe("LAMSS_DEPLOY_IN", &LamssMissionManager::handle_deploy, this);
    subscribe("LAMSS_PROSECUTE_IN", &LamssMissionManager::handle_prosecute, this);
    subscribe("MISSION_STATE", &LamssMissionManager::handle_mission_state, this);
    subscribe("VEHICLE_ID", &LamssMissionManager::handle_vehicle_id, this);
    subscribe("VEHICLE_NAME", &LamssMissionManager::handle_vehicle_name, this);
    subscribe("IFS_DATA_IN", &LamssMissionManager::handle_ifs_data, this);    
    subscribe("LAMSS_TRANSIT_IN", &LamssMissionManager::handle_transit_route, this);    
    subscribe("OPREG_TRAJECTORY_PERIM_DIST", &LamssMissionManager::handle_opbox_distance, this);    
    subscribe("DESIRED_DEPTH", &LamssMissionManager::handle_desired_depth, this);    
    subscribe("GOTO_SURFACE", &LamssMissionManager::handle_goto_surface, this);    
    subscribe("TRACKING", &LamssMissionManager::handle_tracking, this);    
    subscribe("TARGET_BEARING", &LamssMissionManager::handle_target_bearing, this);    
    subscribe("SURVEY_COMPLETED", &LamssMissionManager::handle_survey_flag, this);    

    // intialize MOOSGeodesy

    m_Geodesy.Initialise(cfg_.common().lat_origin(),cfg_.common().lon_origin());

    scheduler_.initiate_processor(fsm_);  
  
    fsm_thread_.reset(new boost::thread(boost::bind(&boost::statechart::fifo_scheduler<>::operator(), &scheduler_, 0)));
  
    glog.is(VERBOSE) && glog << cfg_.DebugString() << std::endl;

    // Avoid abort on startup
    last_acomms_ = MOOSTime();
    mission_start_time_ = MOOSTime();

    if (cfg_.homing_beacon_frequency() > 0)
      publish("HOMING_BEACON_FREQUENCY",cfg_.homing_beacon_frequency());  

}

void LamssMissionManager::loop()
{
    double curr_time = MOOSTime();
    current_time_ = curr_time;

    if (last_acomms_ < 1)
      last_acomms_ = curr_time;


    try_publish(); // publish messages queued from state machine

    // wait 30 seconds sending the initial deploy commands
    if (!initial_deploy_done_ && curr_time >= mission_start_time_ + 30.0)
      {
	initial_transit();
	initial_deploy();
	initial_deploy_done_ = true;
      }

    // process some events at a slower rate

    if(curr_time > last_slow_loop_t_ + slow_loop_period_)
        slow_loop(curr_time);


}

void LamssMissionManager::slow_loop(double curr_time)
{
  // Here we should add the check for blackout_duration and prosecute_duration.
  boost::lock_guard<boost::mutex> lock(moos_mutex_);


  if (prosecute_mode_active_)
    {
    double station_range = 
      sqrt(pow(last_nav_.local_fix().x()-last_deploy_cmd_.deploy_x(),2)+
	   pow(last_nav_.local_fix().y() - last_deploy_cmd_.deploy_y(),2));
    if (cfg_.operation_radius() > 0 && 
	station_range >= cfg_.operation_radius_percent()*0.01*cfg_.operation_radius())
	  {
	    send_deploy();
	    publish("SURVEY_COMPLETED","TRUE");
	    goby::glog << group("helm") 
		       << "DEPLOY Executed: Approaching op_radius during prosecute"
		       <<"station_range = " << station_range 
		       <<", operation_radius = "<< cfg_.operation_radius()
		       << std::endl;
	  }

    double remaining = last_prosecute_cmd_.target_timestamp() + 
      last_prosecute_cmd_.prosecute_duration() - 
      curr_time;
    if (remaining <= 0.0)
      {
	//	  LAMSS_DEPLOY new_deploy = last_deploy_cmd_;
	send_deploy();
	publish("SURVEY_COMPLETED","TRUE");
	goby::glog << group("helm") 
		   << "Prosecute duration exceeded - commanding Deploy" 
		   << std::endl;
      }
    else 
      {
	goby::glog << group("helm") 
		   << "Prosecute time remaining: " << remaining
		   << std::endl;
	publish("PROSECUTE_REMAINING",remaining);
      }
    }

  // Check if Deploy Duration exceeded

  if ( mission_active_ && !homing_mode_active_)
    {
      if ( !abort_mode_active_ && !drifting_mode_active_)
	{
	  double remaining = last_acomms_ + last_deploy_cmd_.deploy_duration() - curr_time ;
	  if (remaining <= 0.0)
	    { 
	      goby::glog << group("helm") 
			 << "Deploy duration exceeded - initiating Abort" 
			 << std::endl;
	      // allow auv to get to abort depth before homing
	      last_homing_prosecute_ = current_time_ - 0.5*cfg_.homing_interval();
	      queue_event<EvNewMission>();
	      queue_event<EvStartAbort>();
	    }
	  else 
	    {
	      goby::glog << group("helm") 
			 << "Deploy time remaining: " << remaining
			 << std::endl;
	      publish("DEPLOY_REMAINING",remaining);
	    }
	  update_abort_mission();
	}
      else if (abort_mode_active_)
	{
	  double remaining =  abort_duration_ - (curr_time - abort_time_);
	  if (remaining > 0.0)
	    {
	      goby::glog << group("helm") 
		<< "Abort time remaining: " << remaining
		<< std::endl;
	      publish("ABORT_REMAINING",remaining);
	      if (cfg_.homing_beacon() && 
		  (curr_time - last_homing_prosecute_) >= cfg_.homing_interval())
		{
		  homing_prosecute();
		}
	    }
	  else
	    {
	      queue_event<EvNewMission>();
	      queue_event<EvStartDrifting>();
	    }
	}
    }
  else if (homing_mode_active_)
    {
      if (curr_time - last_homing_prosecute_ >= 
	  cfg_.homing_duration() - slow_loop_period_)
	{
	  // Finish homing prosecute and extend abort duration
	      queue_event<EvResumeAbort>();
	      abort_duration_ += cfg_.homing_duration() - slow_loop_period_;
	}
    }

  last_slow_loop_t_ = curr_time;
}

void LamssMissionManager::send_deploy()
{
  LAMSS_DEPLOY redeploy = last_deploy_cmd_;;
  std::string serialized;
  redeploy.set_timestamp(MOOSTime());
  redeploy.set__src_id(vehicle_id_);
  redeploy.set_destinationplatformid(vehicle_id_);
  //  if (!MOOSStrCmp(cfg_.deploy_location(),"station"))
  if (cfg_.deploy_location() == LamssMissionManagerConfig::STATION)
    {
      redeploy.set_deploy_x(last_nav_.local_fix().x());
      redeploy.set_deploy_y(last_nav_.local_fix().y());
      // Re-deploy forced to loiter if local
      redeploy.set_deploy_mode(LAMSS_DEPLOY::DEPLOY_MODE_LOITER);
      redeploy.set_depth_mode(LAMSS_DEPLOY::DEPTH_MODE_SINGLE);
    }
  // Auto prosecute only on initial deploy
  redeploy.set_auto_prosecute(false);
  serialize_for_moos(&serialized,redeploy);    
  publish("LAMSS_DEPLOY_OUT", serialized);

}

void LamssMissionManager::homing_prosecute()
{
  LAMSS_PROSECUTE prosecute_cmd;
  prosecute_cmd.set__src_id(vehicle_id_);
  prosecute_cmd.set_destinationplatformid(vehicle_id_);
  prosecute_cmd.set_target_timestamp(current_time_);
  prosecute_cmd.set_target_number(2);
  prosecute_cmd.set_target_x(cfg_.abort_x());
  prosecute_cmd.set_target_y(cfg_.abort_y());
  prosecute_cmd.set_prosecute_duration(cfg_.prosecute_duration());
  prosecute_cmd.set_target_speed(0);
  prosecute_cmd.set_array_angle(60);

  publish("HOMING_BEACON_FREQUENCY",cfg_.homing_beacon_frequency());  
  publish("TRACKER_COMMAND","RESET");

  std::string serialized;
  serialize_for_moos(&serialized,prosecute_cmd);    
  publish("LAMSS_PROSECUTE_OUT", serialized);
  glog.is(DEBUG1) && glog << "Homing Prosecute requested" 
			  << std::endl;
}

void  LamssMissionManager::update_abort_mission()
{
  double range = sqrt(pow(last_nav_.local_fix().x() - cfg_.abort_x(),2)  +
		      pow(last_nav_.local_fix().y() - cfg_.abort_y(),2));
  double heading = atan2(cfg_.abort_x() - last_nav_.local_fix().x(),  
			 cfg_.abort_y() - last_nav_.local_fix().y()) * 180./M_PI;
  while (heading < 0)
    heading += 360;
  while (heading >= 360)
    heading -= 360;
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
  publish("SENSOR_DEPTH_TRACKING",ss2.str());
  publish("SENSOR_DEPTH_DEPLOY",ss2.str());
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

void LamssMissionManager::initial_deploy()
{
  // Handles OUTGOING_COMMANDS for ownship, usually Initial Deploy
  LAMSS_DEPLOY init_deploy_cmd = cfg_.initial_deploy();
  init_deploy_cmd.set_timestamp(MOOSTime());

  // Note that the initial will have topside origin to allow resetting
  // deploy_duration by poking for no-modem use.
  init_deploy_cmd.set__src_id(cfg_.topside_id());
  init_deploy_cmd.set_destinationplatformid(vehicle_id_);

  // Auto prosecute only on initial deploy
  if (initial_deploy_done_)
    init_deploy_cmd.set_auto_prosecute(false);

  std::string serialized;
  serialize_for_moos(&serialized,init_deploy_cmd);    
  publish("LAMSS_DEPLOY_OUT", serialized);
}

void LamssMissionManager::initial_transit()
{
  // Handles OUTGOING_COMMANDS for ownship, usually Initial Deploy
  LAMSS_TRANSIT init_transit_cmd = cfg_.initial_transit();
  init_transit_cmd.set__time(MOOSTime());

  // Note that the initial will have topside origin to allow resetting
  // deploy_duration by poking for no-modem use.
  init_transit_cmd.set__src_id(cfg_.topside_id());
  init_transit_cmd.set_destinationplatformid(vehicle_id_);

  std::string serialized;
  serialize_for_moos(&serialized,init_transit_cmd);    
  publish("LAMSS_TRANSIT_OUT", serialized);
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
  // Only handle messages for me
  if (deploy_cmd.destinationplatformid() == vehicle_id_)
    {
      boost::lock_guard<boost::mutex> lock(moos_mutex_);
      last_deploy_cmd_ = deploy_cmd;
      if (!command_received_)
	{
	  command_received_ = true;
	  command_received_cond_.notify_one();
	}

      post_transit_route();

      if (deploy_cmd._src_id() == cfg_.topside_id())
	last_acomms_ = MOOSTime();
      
      publish("SURVEY_COMPLETED","FALSE");

      if (mission_active_)
	queue_event<EvNewMission>();
      if (last_deploy_cmd_.deploy_mode() == LAMSS_DEPLOY::DEPLOY_MODE_ABORT)
	{
	  // allow auv to get to abort depth before homing
	  last_homing_prosecute_ = current_time_ - 0.5*cfg_.homing_interval();
	  update_abort_mission();
	  queue_event<EvStartAbort>();
	}
      else
	queue_event<EvStartDeploy>();

    }
}

void LamssMissionManager::handle_prosecute(const CMOOSMsg& msg)
{
  LAMSS_PROSECUTE prosecute_cmd;
  parse_for_moos(msg.GetString(), &prosecute_cmd);
  // Only handle messages for me
  if (prosecute_cmd.destinationplatformid() == vehicle_id_)
    {
      boost::lock_guard<boost::mutex> lock(moos_mutex_);
      last_prosecute_cmd_ = prosecute_cmd;
      
      if (prosecute_cmd._src_id() == cfg_.topside_id())
	last_acomms_ = MOOSTime();
      // prosecute only accepted if already in active mission
      if (mission_active_)
	{
	  if (!abort_mode_active_)
	    {
	      queue_event<EvNewMission>();
	      queue_event<EvStartProsecute>();
	    }
	  else
	    {
	      queue_event<EvUpdateHeading>();
	      last_homing_prosecute_ = current_time_;
	    }
	}
      else
	{
	  goby::glog << group("fsm") 
		     << "Not in active mission -  Prosecute ignored"
		     << std::endl;
	}
    }  
}

void LamssMissionManager::handle_status(const CMOOSMsg& msg)
{
  LAMSS_STATUS status_msg;
  parse_for_moos(msg.GetString(), &status_msg);
  // If coming from topside count as being connected
  if (status_msg.node() == cfg_.topside_id())
	last_acomms_ = MOOSTime();
}

void LamssMissionManager::handle_vehicle_id(const CMOOSMsg& msg)
{
    boost::mutex::scoped_lock lock(moos_mutex_);
    vehicle_id_ = msg.GetDouble();
}

void LamssMissionManager::handle_vehicle_name(const CMOOSMsg& msg)
{
    boost::mutex::scoped_lock lock(moos_mutex_);
    vehicle_name_ = msg.GetString();
}

void LamssMissionManager::handle_mission_state(const CMOOSMsg& msg)
{
  std::string mission_state = msg.GetString();
  if (MOOSStrCmp(mission_state,"DRIFTING"))
    {
      boost::lock_guard<boost::mutex> lock(moos_mutex_);
      if (mission_active_ && !drifting_mode_active_)
	{
	  queue_event<EvNewMission>();
	  queue_event<EvStartDrifting>();
	}
    }
}

void LamssMissionManager::handle_opbox_distance(const CMOOSMsg& msg)
{ 
  double opreg_perimeter_distance = msg.GetDouble();
  boost::lock_guard<boost::mutex> lock(moos_mutex_);
  if (prosecute_mode_active_ && opreg_perimeter_distance <  cfg_.min_opbox_distance() )
    {
      send_deploy();
      publish("SURVEY_COMPLETED","TRUE");
      goby::glog << group("helm") 
	<< "DEPLOY Executed: Too close to Opregion Perimeter"
	<< std::endl;
    }
}

void LamssMissionManager::handle_transit_route(const CMOOSMsg& msg)
{
  LAMSS_TRANSIT transit_cmd;
  parse_for_moos(msg.GetString(), &transit_cmd);
  last_transit_route_ = transit_cmd;
  transit_x.clear();
  transit_y.clear();
  if (fabs(last_transit_route_.x_1()) > 1.0 && fabs(last_transit_route_.y_1()) > 1.0)
    {
      transit_x.push_back(last_transit_route_.x_1());
      transit_y.push_back(last_transit_route_.y_1());
    }
  if (fabs(last_transit_route_.x_2()) > 1.0 && fabs(last_transit_route_.y_2()) > 1.0)
    {
      transit_x.push_back(last_transit_route_.x_2());
      transit_y.push_back(last_transit_route_.y_2());
    }
  if (fabs(last_transit_route_.x_3()) > 1.0 && fabs(last_transit_route_.y_3()) > 1.0)
    {
      transit_x.push_back(last_transit_route_.x_3());
      transit_y.push_back(last_transit_route_.y_3());
    }
  if (fabs(last_transit_route_.x_4()) > 1.0 && fabs(last_transit_route_.y_4()) > 1.0)
    {
      transit_x.push_back(last_transit_route_.x_4());
      transit_y.push_back(last_transit_route_.y_4());
    }
  if (fabs(last_transit_route_.x_5()) > 1.0 && fabs(last_transit_route_.y_5()) > 1.0)
    {
      transit_x.push_back(last_transit_route_.x_5());
      transit_y.push_back(last_transit_route_.y_5());
    }
}

void LamssMissionManager::post_transit_route()
{
 
  if (last_transit_route_.transit_mode() == LAMSS_TRANSIT::TRANSIT_MODE_ROUTE)
    {
      // remove trailing zero waypoints from transit route
      
      std::stringstream transit_route;
      std::string hash = "#";
      std::string colon = ":";
      std::string comma = ",";
      int n = transit_x.size();
      if (n>0)
	{
	  transit_route << "speed=" << last_transit_route_.transit_speed() 
			<< "#points=" 
			<< last_nav_.local_fix().x() << comma
			<< last_nav_.local_fix().y() << colon
;
	  if (last_deploy_cmd_.deploy_mode() == LAMSS_DEPLOY::DEPLOY_MODE_RETURN)
	    {
	      // If RETURN mode publish route in reverse order
	      for (int i=n-1; i>=0; i--)
		{
		  transit_route << transit_x[i] << comma << transit_y[i] ;
		  transit_route << colon ;
		}
	    }
	  else
	    {
	      for (int i=0; i<n; i++)
		{
		  transit_route << transit_x[i] << comma << transit_y[i] ;
		  transit_route << colon ;
		}
	    }
	  transit_route << last_deploy_cmd_.deploy_x() << comma
			<< last_deploy_cmd_.deploy_y() ;

 	  publish("TRANSIT_DEPTH", last_transit_route_.transit_depth());
	  publish("TRANSIT_ROUTE",transit_route.str());
	  publish("TRANSIT_MODE", "ROUTE");
	}
      else
	publish("TRANSIT_MODE", "DIRECT");

    } 
  else
    publish("TRANSIT_MODE", "DIRECT");
}

void LamssMissionManager::handle_desired_depth(const CMOOSMsg& msg)
{
  double depth_mismatch =  fabs(last_nav_.global_fix().depth()-msg.GetDouble());
  publish("DEPTH_MISMATCH",depth_mismatch);
}

void LamssMissionManager::handle_goto_surface(const CMOOSMsg& msg)
{
  if (MOOSStrCmp(toupper(msg.GetString()),"TRUE"))
    queue_event<EvGotoSurface>();
  else
    queue_event<EvEnableDiving>();
}

void LamssMissionManager::handle_tracking(const CMOOSMsg& msg)
{
  tracking_ = MOOSStrCmp(toupper(msg.GetString()),"TRACKING");
  // if in homing mode return to abort
  boost::mutex::scoped_try_lock lk(moos_mutex_);
  if (homing_mode_active_ && tracking_)
    {
      abort_duration_ += current_time_ - last_homing_prosecute_;
      double new_heading = target_bearing_;
      if (new_heading < 0)
	new_heading += 360;
      std::stringstream ss;
      ss << "heading=" << new_heading << "#duration=" << abort_duration_ ; 
      publish("ABORT_HEADING", ss.str());
      queue_event<EvResumeAbort>();
    }
}

void LamssMissionManager::handle_target_bearing(const CMOOSMsg& msg)
{
  target_bearing_ = msg.GetDouble();
}

void LamssMissionManager::handle_survey_flag(const CMOOSMsg& msg)
{
  if (MOOSStrCmp(toupper(msg.GetString()),"TRUE"))
    initial_deploy();
}

std::string LamssMissionManager::NodeReport(std::string node_name, std::string node_type, double node_x, double node_y, double node_spd, double node_hdg, double node_dep )
{

  double node_lat;
  double node_lon;
  std::string comma = ",";
  std::string output="NAME=" + node_name +",TYPE=" + node_type + comma;

  if (!m_Geodesy.UTM2LatLong(node_x,node_y,node_lat,node_lon))
    {
      glog.is(DEBUG2) && glog 
	<< "Error in UTM2LatLong conversion" << std::endl;
      return("Error in UTM2LatLong conversion\n");	  
    }

  output += "UTC_TIME=" + doubleToString(MOOSTime(),1);
  output += ",";

  output += "X=" + doubleToString(node_x,2);
  output += ",";

  output += "Y=" + doubleToString(node_y,2);
  output += ",";

  output += "LAT=" + doubleToString(node_lat,6);
  output += ",";

  output += "LON=" + doubleToString(node_lon,6);
  output += ",";

  output += "SPD=" + doubleToString(node_spd,2);
  output += ",";

  output += "HDG=" + doubleToString(node_hdg,2);

  output += ",";

  output += "DEPTH=" + doubleToString(node_dep,2);

  return(output);
}
