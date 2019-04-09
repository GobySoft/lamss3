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

#include "goby/acomms/protobuf/driver_base.pb.h"
#include "goby/acomms/protobuf/mm_driver.pb.h"

#include "ivp/NodeRecord.h"


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
      abort_x(cfg_.abort_x_offset()),
      abort_y(cfg_.abort_y_offset()),
      abort_x_offset(cfg_.abort_x_offset()),
      abort_y_offset(cfg_.abort_y_offset()),
      homing_beacon(cfg_.homing_array() != LamssMissionManagerConfig::NONE),
      slow_loop_period_(15),
      got_first_tracking_msg(false),
      in_track(false),
      depth_adapting(false),
      last_slow_loop_t_(0),
      filter_length(10)
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
    subscribe("COLLABORATOR_ID",
	      &LamssMissionManager::handle_collaborator_id, this);
    subscribe("IFS_DATA_IN", &LamssMissionManager::handle_ifs_data, this);
    subscribe("LAMSS_TRANSIT_IN", &LamssMissionManager::handle_transit_route, this);
    subscribe("OPREG_TRAJECTORY_PERIM_DIST", &LamssMissionManager::handle_opbox_distance, this);
    subscribe("DESIRED_DEPTH", &LamssMissionManager::handle_desired_depth, this);
    subscribe("GOTO_SURFACE", &LamssMissionManager::handle_goto_surface, this);
    subscribe("TRACKING", &LamssMissionManager::handle_tracking, this);
    subscribe("TARGET_BEARING", &LamssMissionManager::handle_target_bearing, this);
    subscribe("SURVEY_COMPLETED", &LamssMissionManager::handle_survey_flag, this);
    subscribe("GPS_UPDATE_RECEIVED", &LamssMissionManager::handle_gps_update, this);
    subscribe("IN_TRACK", &LamssMissionManager::handle_in_track, this);
    subscribe(cfg_.tracking_message(), &LamssMissionManager::handle_tracking_message, this);
    subscribe_pb(cfg_.track_signal_update_message(), &LamssMissionManager::handle_track_signal_update_message, this);
    // intialize MOOSGeodesy

    m_Geodesy.Initialise(cfg_.common().lat_origin(),cfg_.common().lon_origin());
    // Set up initial OpRegion if dynamic
    if (cfg_.dynamic_opregion())
      {
	opregion.clear();
	for (int i=0; i<cfg_.opbox_vertex_size(); i++)
	  if (!opregion.add_vertex(cfg_.opbox_vertex(i).x(),
				   cfg_.opbox_vertex(i).y(),true))
	    {
	      glog << "Error: OpRegion non-convex" << std::endl;
	    }
	opregion.set_label("OpRegion");
	std::string str = opregion.get_spec();
	publish("OPREGION_UPDATE",str);
	opregion_init = opregion;
	opregion.set_label("OpRegionInit");
	
      }

    camp_x_spd_filt.configAvg(filter_length);
    camp_y_spd_filt.configAvg(filter_length);
    
    // Avoid abort on startup
    last_acomms_ = MOOSTime();
    mission_start_time_ = MOOSTime();

    if (cfg_.homing_beacon_frequency() > 0)
      publish("HOMING_BEACON_FREQUENCY",cfg_.homing_beacon_frequency());

    glog.is(VERBOSE) && glog << cfg_.DebugString() << std::endl;

    // Start fsm thread
    scheduler_.initiate_processor(fsm_);

    fsm_thread_.reset(new boost::thread(boost::bind(&boost::statechart::fifo_scheduler<>::operator(), &scheduler_, 0)));


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
  // First update deploy and abort parameters
  if (cfg_.dynamic_opregion() && got_first_tracking_msg)
    update_deploy_parameters();
  
  // Here we should add the check for blackout_duration and prosecute_duration.
  boost::lock_guard<boost::mutex> lock(moos_mutex_);


  if (prosecute_mode_active_)
    {
    double station_range =
      sqrt(pow(last_nav_.local_fix().x()-last_deploy_cmd_.deploy_x(),2)+
	   pow(last_nav_.local_fix().y()-last_deploy_cmd_.deploy_y(),2));
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

    double remaining = last_prosecute_cmd_._time() +
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

  // Check if Deploy Duration exceeded or if tracking updates stale

  if ( mission_active_ && !homing_mode_active_)
    {
      if ( !abort_mode_active_ && !drifting_mode_active_)
	{
	  double stale_acomms = curr_time - last_acomms_;
	  double remaining = last_deploy_cmd_.deploy_duration() - stale_acomms;
	  if (stale_acomms >= last_deploy_cmd_.deploy_duration())
	    {
	      goby::glog << group("helm")
			 << "Deploy duration exceeded w/o acomms - initiating Abort"
			 << std::endl;
	      // allow auv to get to abort depth before homing
	      last_homing_prosecute_ = current_time_ - 0.5*cfg_.homing_interval();
	      if (cfg_.homing_array() == LamssMissionManagerConfig::USBL)
		publish("USBL_ENABLED","TRUE");
	      queue_event<EvNewMission>();
	      queue_event<EvStartAbort>();
	    }
	  else if (stale_acomms >= 0.25 * last_deploy_cmd_.deploy_duration() && !depth_adapting)
	    {
	      goby::glog << group("helm")
			 << "25% of deploy duration exceeded w/o acomms - going to acomms depth"
			 << std::endl;
		publish("DEPTH_MODE","ADAPTIVE_ACOMMS");
		depth_adapting = true;
	    }
	  else
	    {
	      goby::glog << group("helm")
			 << "Deploy time remaining: " << remaining
			 << std::endl;
	      publish("DEPLOY_REMAINING",remaining);
	    }
	  // Update abort mission for ice motion
	  update_abort_mission();
	}
      else if (abort_mode_active_)
	{
	  double remaining =  abort_duration_ - (curr_time - abort_time_);
	  if (remaining < 4*slow_loop_period_)
	    {
	      // Enter return mode 
	      homing_return();
	    }
	  else
	    {
	      goby::glog << group("helm")
		<< "Abort time remaining: " << remaining
		<< std::endl;
	      publish("ABORT_REMAINING",remaining);
	      // Update duration of abort heading behavior
	      //	      std::stringstream cc;
	      //	      cc << "duration=" << remaining;
	      //	      publish("ABORT_HEADING",cc.str());
	      update_abort_mission();
	      // If homing using towed array go to prosecute homing state
	      if (cfg_.homing_array() == LamssMissionManagerConfig::TOWED)
		{
		  if (homing_beacon &&
		      last_deploy_cmd_.homing_enabled() &&
		      (curr_time - last_homing_prosecute_) >= cfg_.homing_interval())
		    {
		      abort_duration_ = remaining;
		      homing_prosecute();
		    }
		}
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
	      std::stringstream ss;
	      ss << "duration=" << abort_duration_ ;
	      publish("ABORT_HEADING", ss.str());
	}
    }

  last_slow_loop_t_ = curr_time;
}

void LamssMissionManager::send_deploy()
{
  LAMSS_DEPLOY redeploy = last_deploy_cmd_;;
  std::string serialized;
  redeploy.set__time(MOOSTime());
  redeploy.set__src_id(vehicle_id_);
  redeploy.set__rcv_id(vehicle_id_);
  //  if (!MOOSStrCmp(cfg_.deploy_location(),"station"))
  if (cfg_.deploy_location() == LamssMissionManagerConfig::CURRENT)
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
  prosecute_cmd.set__rcv_id(vehicle_id_);
  prosecute_cmd.set__time(current_time_);
  prosecute_cmd.set_target_number(2);
  prosecute_cmd.set_target_x(abort_x);
  prosecute_cmd.set_target_y(abort_y);
  prosecute_cmd.set_prosecute_duration(cfg_.prosecute_duration());
  prosecute_cmd.set_target_speed(0);
  prosecute_cmd.set_array_angle(60);
  prosecute_cmd.set_center_frequency(cfg_.prosecute_center_frequency());
  prosecute_cmd.set_bandwidth(cfg_.prosecute_bandwidth());

  publish("HOMING_BEACON_FREQUENCY",cfg_.homing_beacon_frequency());
  publish("TRACKER_COMMAND","RESET");
  std::stringstream ss2;
  ss2 << "depth=" << cfg_.abort_depth() ;
  publish("DEPTH_MODE","SINGLE");
  publish("SENSOR_DEPTH_TRACKING",ss2.str());
  publish("SENSOR_DEPTH_DEPLOY",ss2.str());

  std::string serialized;
  serialize_for_moos(&serialized,prosecute_cmd);
  publish("LAMSS_PROSECUTE_OUT", serialized);
  glog.is(DEBUG1) && glog << "Homing Prosecute requested"
			  << std::endl;
}

void  LamssMissionManager::update_abort_mission()
{
  double range = sqrt(pow(last_nav_.local_fix().x() - abort_x,2)  +
		      pow(last_nav_.local_fix().y() - abort_y,2));
  double heading = atan2(abort_x - last_nav_.local_fix().x(),
			 abort_y - last_nav_.local_fix().y()) * 180./M_PI;
  while (heading < 0)
    heading += 360;
  while (heading >= 360)
    heading -= 360;
  double duration = range/cfg_.abort_speed();
  std::stringstream ss;
  ss << "heading=" << heading << "#duration=" << duration ;
  publish("ABORT_HEADING", ss.str());
  std::stringstream ss1;
  ss1 << "speed=" << cfg_.abort_speed() ;
  publish("ABORT_SPEED", ss1.str());
  std::stringstream ss2;
  ss2 << "depth=" << cfg_.abort_depth() ;
  publish("ABORT_DEPTH", ss2.str());
  std::stringstream ss3;
  ss3 << "points=" << abort_x << "," << abort_y;
  publish("ABORT_LOCATION", ss3.str());
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
  init_deploy_cmd.set__time(MOOSTime());

  init_deploy_cmd.set__rcv_id(vehicle_id_);

  // Auto prosecute only on initial deploy
  if (initial_deploy_done_)
    {
      // Note that the initial will have topside origin to allow resetting
      // deploy_duration by poking for no-modem use.
      //  init_deploy_cmd.set__src_id(cfg_.topside_id());
      init_deploy_cmd.set__src_id(vehicle_id_);
      init_deploy_cmd.set_auto_prosecute(false);
      // for auto redeploy, center at current abort location
      // This is important for dynamic scenarios such as in ICEX
      init_deploy_cmd.set_deploy_x(abort_x);
      init_deploy_cmd.set_deploy_y(abort_y);
      // If adapt depth mode, keep it for survey return
      if (depth_adapting)
	init_deploy_cmd.set_depth_mode(
	   LAMSS_DEPLOY::DEPTH_MODE_ADAPTIVE_ACOMMS);
      // If stale acomms, continue with current duration timer
      double stale_acomms = MOOSTime() - last_acomms_;
      double remaining = last_deploy_cmd_.deploy_duration() - stale_acomms;
      if (remaining < init_deploy_cmd.deploy_duration())
	init_deploy_cmd.set_deploy_duration(remaining);

    }
  std::string serialized;
  serialize_for_moos(&serialized,init_deploy_cmd);
  publish("LAMSS_DEPLOY_OUT", serialized);
}

void LamssMissionManager::homing_return()
{
  // Handles OUTGOING_COMMANDS for ownship, usually Initial Deploy
  LAMSS_DEPLOY homing_cmd = cfg_.initial_deploy();
  homing_cmd.set__time(MOOSTime());

  homing_cmd.set__rcv_id(vehicle_id_);

  // Note that the initial will have topside origin to allow resetting
  // deploy_duration by poking for no-modem use.
  //  init_deploy_cmd.set__src_id(cfg_.topside_id());
  homing_cmd.set__src_id(vehicle_id_);
  homing_cmd.set_auto_prosecute(false);
  // for auto redeploy, center at current abort location
  // This is important for dynamic scenarios such as in ICEX
  homing_cmd.set_deploy_x(abort_x);
  homing_cmd.set_deploy_y(abort_y);
  homing_cmd.set_deploy_mode(LAMSS_DEPLOY::DEPLOY_MODE_RETURN);
  // If adapt depth mode, keep it for survey return
  if (depth_adapting)
    homing_cmd.set_depth_mode(
	   LAMSS_DEPLOY::DEPTH_MODE_ADAPTIVE_ACOMMS);
  homing_cmd.set_homing_enabled(true);
  std::string serialized;
  serialize_for_moos(&serialized,homing_cmd);
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
  init_transit_cmd.set__rcv_id(vehicle_id_);

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
  if (deploy_cmd._rcv_id() == vehicle_id_)
    {
      boost::lock_guard<boost::mutex> lock(moos_mutex_);
      last_deploy_cmd_ = deploy_cmd;
      if (!command_received_)
	{
	  command_received_ = true;
	  command_received_cond_.notify_one();
	}
      // Set camp location at deploy for zamboni behaviors
      camp_x_deploy = camp_x_last + camp_x_spd_filt.getAvg()*
	(MOOSTime() - last_tracking_msg);
      camp_y_deploy = camp_y_last + camp_y_spd_filt.getAvg()*
	(MOOSTime() - last_tracking_msg);
      
      post_transit_route();

      if (deploy_cmd._src_id() == cfg_.topside_id())
	  last_acomms_ = MOOSTime();
      if (last_deploy_cmd_.depth_mode() == LAMSS_DEPLOY::DEPTH_MODE_ADAPTIVE_ACOMMS)
	depth_adapting = true;
      else
	depth_adapting = false;
      publish("SURVEY_COMPLETED","FALSE");
      
      // USBL homing enabled in DEPLOY/ABORT modes with USBL configured
      if (last_deploy_cmd_.homing_enabled() &&
         last_deploy_cmd_.deploy_mode() == LAMSS_DEPLOY::DEPLOY_MODE_RETURN &&
         cfg_.homing_array() == LamssMissionManagerConfig::USBL)
        publish("USBL_ENABLED","TRUE");
      else if (last_deploy_cmd_.deploy_mode() == LAMSS_DEPLOY::DEPLOY_MODE_ABORT &&
         cfg_.homing_array() == LamssMissionManagerConfig::USBL)
        publish("USBL_ENABLED","TRUE");
      else
        publish("USBL_ENABLED","FALSE");

      // Queue state transitions
      if (mission_active_)
	queue_event<EvNewMission>();

      if (last_deploy_cmd_.deploy_mode() == LAMSS_DEPLOY::DEPLOY_MODE_ABORT)
	{
	  // allow auv to get to abort depth before homing
	  last_homing_prosecute_ = current_time_ - 0.5*cfg_.homing_interval();
	  update_abort_mission();
	  queue_event<EvStartAbort>();
	}
      else if (last_deploy_cmd_.deploy_mode() == LAMSS_DEPLOY::DEPLOY_MODE_RETURN)
	{
	  queue_event<EvStartDeploy>();
	  queue_event<EvReturnToBase>();
	}
      else
	{
	  queue_event<EvStartDeploy>();
	}
    }
}

void LamssMissionManager::handle_prosecute(const CMOOSMsg& msg)
{
  LAMSS_PROSECUTE prosecute_cmd;
  parse_for_moos(msg.GetString(), &prosecute_cmd);
  // Only handle messages for me
  if (prosecute_cmd._rcv_id() == vehicle_id_)
    {
      boost::lock_guard<boost::mutex> lock(moos_mutex_);
      last_prosecute_cmd_ = prosecute_cmd;

      if (prosecute_cmd._src_id() == cfg_.topside_id())
	last_acomms_ = MOOSTime();
      // Force single depth for prosecute
        publish("DEPTH_MODE","SINGLE");

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
  // Disabled after ICEXTracker implemented. Only nav updates and commands count
#if 0
  if (status_msg.node() == cfg_.topside_id())
	last_acomms_ = MOOSTime();
#endif
}

void LamssMissionManager::handle_gps_update(const CMOOSMsg& msg)
{
  glog.is(VERBOSE) && glog << "Received GPS update: " << msg.GetString()
			   << std::endl;
  // reset deploy duration when getting GPS update
	last_acomms_ = MOOSTime();
}

void LamssMissionManager::handle_vehicle_id(const CMOOSMsg& msg)
{
    boost::mutex::scoped_lock lock(moos_mutex_);
    vehicle_id_ = msg.GetDouble();
}

void LamssMissionManager::handle_collaborator_id(const CMOOSMsg& msg)
{
    collaborator_id_ = msg.GetDouble();
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

void LamssMissionManager::handle_in_track(const CMOOSMsg& msg)
{
  std::string str = msg.GetString();
  in_track = MOOSStrCmp(str,"TRUE");
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
      abort_duration_ += 0.5*(current_time_ - last_homing_prosecute_);
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
    {
      glog.is(DEBUG1) && glog << "Survey completed - Returning to initial deploy" << std::endl;
      initial_deploy();
    }
}

void LamssMissionManager::handle_tracking_message(const CMOOSMsg& msg)
{
  lamss::icex::protobuf::VehiclePosition vehicle_pos;
  parse_for_moos(msg.GetString(), &vehicle_pos);
  // Only handle messages for me
  if (vehicle_pos.dest() == vehicle_id_)
    {
      double moos_x;
      double moos_y;
      double camp_x;
      double camp_y;
      if ( m_Geodesy.LatLong2LocalUTM(vehicle_pos.lat(), vehicle_pos.lon(),
				      moos_y, moos_x) &&
	   m_Geodesy.LatLong2LocalUTM(vehicle_pos.camp_lat(),
				      vehicle_pos.camp_lon(),
				      camp_y, camp_x))
	{
	  // save initial camp location
	  if (!got_first_tracking_msg)
	    {
	      camp_x_init = camp_x;
	      camp_y_init = camp_y;
	      got_first_tracking_msg = true;
	      last_tracking_msg = vehicle_pos.time();
	      camp_x_spd_filt.configAvg(filter_length);
	      camp_y_spd_filt.configAvg(filter_length);
	    }
	  else
	    {
	      // Update camp speed filters
	      camp_x_spd_filt.setVal((camp_x-camp_x_last)/
				     (vehicle_pos.time()-last_tracking_msg));
	      camp_y_spd_filt.setVal((camp_y-camp_y_last)/
				     (vehicle_pos.time()-last_tracking_msg));
	    }
	  // save camp location for possible deploy zambony being relative to deploy location
	  camp_x_last = camp_x;
	  camp_y_last = camp_y;
	  // Use current vehicle time stamp
	  last_tracking_msg = vehicle_pos.time();
	  

	  // reset deploy duration counter
	  last_acomms_ = MOOSTime();
	  glog.is(VERBOSE) && glog << "Received Tracking Update:"
				  << " lat=" << vehicle_pos.lat()
				  << " lon=" << vehicle_pos.lon()
				  << " my_x=" << moos_x
				  << " my_y=" << moos_y
				  << " camp_x=" << camp_x
				  << " camp_y=" << camp_y
				  << std::endl;
	  
	    NodeRecord node_report("camp_sargo", "icecamp");
	    node_report.setTimeStamp(vehicle_pos.time());
	    node_report.setX(camp_x);
	    node_report.setY(camp_y);
	    node_report.setLat(vehicle_pos.camp_lat());
	    node_report.setLon(vehicle_pos.camp_lon());
	    node_report.setDepth(vehicle_pos.camp_modem_depth());
	    //	    node_report.setSpeed(vehicle_pos.camp_drift_speed());
	    //	    node_report.setHeading(vehicle_pos.camp_drift_heading());
	    double camp_drift_speed = sqrt(pow(camp_x_spd_filt.getAvg(),2)+
				     pow(camp_y_spd_filt.getAvg(),2));
	    node_report.setSpeed(camp_drift_speed);
	    double camp_drift_heading =
	      atan2(camp_x_spd_filt.getAvg(),camp_y_spd_filt.getAvg())*180/M_PI;
	    if (camp_drift_heading < 0)
	      camp_drift_heading +=360;
	    node_report.setHeading(camp_drift_heading);
	    publish("NODE_REPORT", node_report.getSpec());

	}
      else
	{
	  glog.is(VERBOSE) && glog << "Invalid Tracking Update:" << std::endl;
	}
    }
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

void LamssMissionManager::handle_track_signal_update_message(const lamss::icex::protobuf::TrackingSignalUpdate& msg)
{
    goby::acomms::protobuf::DriverConfig cfg;
    cfg.set_modem_id(msg.tracking_modem_id());

    // just needs to be the start of a minute
    const int even_reference_time = 1449763200;
    const int odd_reference_time = even_reference_time + static_cast<int>(msg.ping_period())/2;

    switch(msg.current_vehicle())
    {
        case lamss::icex::protobuf::TrackingSignalUpdate::NONE:
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.mode,0");
            break;

        case lamss::icex::protobuf::TrackingSignalUpdate::EVEN_PLATFORM:
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.carrier_Hz,10500");
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.period," + goby::util::as<std::string>(msg.ping_period()));
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.reference_time," + goby::util::as<std::string>(even_reference_time));
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.duration_msx10,100");
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.bandwidth_Hz,3000");
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.mode,3");
            break;

        case lamss::icex::protobuf::TrackingSignalUpdate::ODD_PLATFORM:
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.carrier_Hz,10500");
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.period," + goby::util::as<std::string>(msg.ping_period()));
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.reference_time," + goby::util::as<std::string>(odd_reference_time));
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.duration_msx10,100");
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.bandwidth_Hz,3000");
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.mode,3");
            break;

        case lamss::icex::protobuf::TrackingSignalUpdate::UNIT:
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.carrier_Hz,13500");
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.period,1");
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.reference_time," + goby::util::as<std::string>(even_reference_time));
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.duration_msx10,35");
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.mode,4");
            break;

        case lamss::icex::protobuf::TrackingSignalUpdate::TOPSIDE_HOMING:
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.carrier_Hz,8250");
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.period,1");
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.reference_time," + goby::util::as<std::string>(even_reference_time));
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.duration_msx10,200");
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.bandwidth_Hz,1500");
            cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "nav.trackping.mode,3");
            break;
    }

    if(msg.has_tx_power_level())
    {
        int tx_level = msg.tx_power_level();
        cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "pwramp.txlevel," + goby::util::as<std::string>(tx_level));
    }

    if(msg.has_mod_header_version())
    {
        int hdr_version = msg.mod_header_version();
        cfg.AddExtension(micromodem::protobuf::Config::nvram_cfg, "psk.packet.mod_hdr_version," + goby::util::as<std::string>(hdr_version));
    }

    publish_pb("ACOMMS_DRIVER_CONFIG_UPDATE", cfg);

}

void LamssMissionManager::update_deploy_parameters()
{
  double camp_x = camp_x_last + camp_x_spd_filt.getAvg()*
    (MOOSTime()-last_tracking_msg);
  double camp_y = camp_y_last + camp_y_spd_filt.getAvg()*
    (MOOSTime()-last_tracking_msg);
  {
    // Update OpRegion to follow ice camp
    opregion = opregion_init;
    opregion.shift_horz(camp_x - camp_x_init);
    opregion.shift_vert(camp_y - camp_y_init);
    opregion.set_label("OpRegion");
    std::string poly = opregion.get_spec();
    std::stringstream cc;
    cc << "polygon=" << poly;
    publish("OPREGION_UPDATE",cc.str());
    // Zamboni Loiters for ice camp ops
    std::stringstream cent;
    cent << "center_assign="
	 << last_deploy_cmd_.deploy_x() + camp_x-camp_x_deploy
	 << "," << last_deploy_cmd_.deploy_y() + camp_y-camp_y_deploy;
    publish("SENSOR_DEPLOY",cent.str());
    //publish("HOMING_CONFIG",cent.str());
    // Consistent station keeping point
    std::stringstream statn;
    statn << "point="
	  << last_deploy_cmd_.deploy_x() + camp_x-camp_x_deploy
	  << "," << last_deploy_cmd_.deploy_y() + camp_y-camp_y_deploy;
    publish("DEPLOY_STATION",statn.str());
    // Move racetrack unless on trackline
    if (!in_track)
      {
	std::stringstream trck;
	trck << "points="
	     << last_deploy_cmd_.deploy_x() + camp_x-camp_x_deploy
	     << ","
	     << last_deploy_cmd_.deploy_y() + camp_y-camp_y_deploy;
	publish("SENSOR_RACETRACK",trck.str());
	
      }
  }
  // update abort location. Note that it's defined as offset relative to camp
  abort_x = camp_x + abort_x_offset;
  abort_y = camp_y + abort_y_offset;
  // update homing beacon location (assumed to be abort location
  std::stringstream bc;
  bc << "beacon_x=" << abort_x << ",beacon_y=" << abort_y ;
  publish("BEACON_UPDATE",bc.str());
}
