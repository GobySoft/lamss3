// Copyright 2014
// Henrik Schmidt
// Toby Schneider
// Massachusetts Institute of Technology

#include <dlfcn.h>

#include <numeric>

#include <boost/thread.hpp>

#include "goby/util/debug_logger.h"
#include "goby/acomms/connect.h"
#include "goby/time/legacy.h"

#include "LamssMissionManager.h"

#include "goby/moos/moos_protobuf_helpers.h"

#include "goby/acomms/protobuf/driver_base.pb.h"
#include "goby/acomms/protobuf/mm_driver.pb.h"
#include "goby/middleware/frontseat/bluefin/bluefin.pb.h"

#include "ivp/NodeRecord.h"


using namespace lamss::shared::fsm;
using namespace goby::util::logger;
using namespace goby::util::logger_lock;
namespace gpb = goby::middleware::frontseat::protobuf;
using goby::common::goby_time;
using goby::glog;

const double pi = 3.14159;

// force goby_frontseat_bluefin to link to avoid missing extension error (Extension "goby.moos.protobuf.bluefin_data" is not defined or is not an extension of "goby.moos.protobuf.FrontSeatInterfaceData".)
gpb::BluefinExtraData dummy;

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
  : goby::moos::GobyMOOSApp(&cfg),
    cfg_(cfg),
    scheduler_(true),
    fsm_(scheduler_.create_processor<LamssMissionManagerFSM>()),
    initial_deploy_done_(false),
    abort_x(cfg_.abort_x_offset()),
    abort_y(cfg_.abort_y_offset()),
    abort_x_offset(cfg_.abort_x_offset()),
    abort_y_offset(cfg_.abort_y_offset()),
    homing_beacon(cfg_.homing_array() != LamssMissionManagerConfig::NONE),
    homing_lock(false),
    slow_loop_period_(15),
    got_first_tracking_msg(false),
    in_track(false),
    depth_adapting(false),
    last_slow_loop_t_(0),
    collaborator_id_(-1),
    collaborator_name(""),
    new_origin_(false),
    filter_length(10),
    last_utm_update_(0),
    got_gps_nav(false)
{

  goby::glog.set_lock_action(lock);
  goby::glog.add_group("fsm", goby::util::Colors::lt_magenta);
  goby::glog.add_group("helm", goby::util::Colors::lt_green);
  goby::glog.add_group("mac", goby::util::Colors::lt_blue);
  goby::glog.add_group("gram", goby::util::Colors::yellow);

  set_ignore_stale(false);

  // set up subscriptions before starting separate thread
  if (cfg_.dynamic_utm())
    {
      subscribe("LAT_ORIGIN", &LamssMissionManager::handle_lat_origin, this);
      subscribe("LONG_ORIGIN", &LamssMissionManager::handle_lon_origin, this);
    }
    
  subscribe("MODE", &LamssMissionManager::handle_helm_mode, this);

#ifndef GEO_COMMAND
  subscribe("LAMSS_STATUS_IN",
	    &LamssMissionManager::handle_status, this);
  subscribe("LAMSS_DEPLOY_IN",
	    &LamssMissionManager::handle_deploy, this);
  subscribe("LAMSS_PROSECUTE_IN",
	    &LamssMissionManager::handle_prosecute, this);
  subscribe("LAMSS_TRANSIT_IN",
	    &LamssMissionManager::handle_transit_route, this);
#else
  subscribe("LAMSS_STATUS_GEO_IN",
	    &LamssMissionManager::handle_status, this);
  subscribe("LAMSS_DEPLOY_GEO_IN",
	    &LamssMissionManager::handle_deploy, this);
  subscribe("LAMSS_PROSECUTE_GEO_IN",
	    &LamssMissionManager::handle_prosecute, this);
  subscribe("LAMSS_TRANSIT_GEO_IN",
	    &LamssMissionManager::handle_transit_route, this);
#endif
  subscribe("LAMSS_STOVEPIPE_GEO_IN",
	    &LamssMissionManager::handle_stovepipe_geo_message, this);    
  subscribe("LAMSS_STOVEPIPE_IN",
	    &LamssMissionManager::handle_stovepipe_message, this);    
  // Note: stovepipes not yet converted to GEO
  subscribe("MISSION_STATE", &LamssMissionManager::handle_mission_state, this);
  subscribe("VEHICLE_ID", &LamssMissionManager::handle_vehicle_id, this);
  subscribe("VEHICLE_NAME", &LamssMissionManager::handle_vehicle_name, this);
  subscribe("COLLABORATOR_ID",
	    &LamssMissionManager::handle_collaborator_id, this);
  subscribe("COLLABORATOR_NAME", 
	    &LamssMissionManager::handle_collaborator_name, this);  
  if (cfg_.navigating_with_hydroman())
    subscribe("HYDROMAN_NAV_DATA", &LamssMissionManager::handle_hydroman_data, this);
  else
    subscribe("IFS_DATA_IN", &LamssMissionManager::handle_ifs_data, this);
  subscribe("OPREG_TRAJECTORY_PERIM_DIST", &LamssMissionManager::handle_opbox_distance, this);
  subscribe("DESIRED_DEPTH", &LamssMissionManager::handle_desired_depth, this);
  subscribe("GOTO_SURFACE", &LamssMissionManager::handle_goto_surface, this);
  subscribe("TRACKING", &LamssMissionManager::handle_tracking, this);
  subscribe("TARGET_BEARING", &LamssMissionManager::handle_target_bearing, this);
  subscribe("SURVEY_COMPLETED", &LamssMissionManager::handle_survey_flag, this);
  //    subscribe("GPS_UPDATE_RECEIVED", &LamssMissionManager::handle_gps_update, this);
  subscribe("IN_TRACK", &LamssMissionManager::handle_in_track, this);
  subscribe("HOMING_LOCK", &LamssMissionManager::handle_homing_lock, this);
  subscribe(cfg_.tracking_message(), &LamssMissionManager::handle_icex20_tracking_message, this);
  subscribe_pb(cfg_.track_signal_update_message(), &LamssMissionManager::handle_track_signal_update_message, this);
  subscribe("SURVEY_WPT_INDEX",
	    &LamssMissionManager::handle_survey_wpt_index, this);    
  subscribe("TRANSIT_WPT_INDEX",
	    &LamssMissionManager::handle_transit_wpt_index, this);    
  subscribe("TRANSIT_COMPLETED", &LamssMissionManager::handle_transit_flag, this);
  subscribe_pb("ICEX_VEHICLE_TRANSMIT_SELECTION_IN", &LamssMissionManager::handle_vehicle_transmit_selection, this);
  // intialize MOOSGeodesy

  m_Geodesy.Initialise(cfg_.common().lat_origin(),cfg_.common().lon_origin());
  // save initial abort location for dynamic UTM
  m_Geodesy.UTM2LatLong(abort_x,abort_y,abort_lat,abort_lon);

  // Set up initial OpRegion if dynamic
  if (cfg_.dynamic_opregion())
    {
      opregion.clear();
      for (int i=0; i<cfg_.opbox_vertex_size(); i++)
	if (!opregion.add_vertex(cfg_.opbox_vertex(i).x(),
				 cfg_.opbox_vertex(i).y(),true))
	  {
	    glog.is(WARN) && glog << "Error: OpRegion non-convex" << std::endl;
	  }
      opregion.set_label("OpRegion");
      std::string str = "polygon="+opregion.get_spec();
      publish("OPREGION_UPDATE",str);
      opregion_init = opregion;
      opregion_init.set_label("OpRegionInit");
	
    }

  camp_x_spd_filt.configAvg(filter_length);
  camp_y_spd_filt.configAvg(filter_length);
  transit_wpt_index = 0;
  survey_wpt_index = 0;
  new_wpt_index = false;
    
  // Make sure to publish the homing location in case we abort without tracking message arrived
  std::stringstream bc;
  bc << "beacon_x=" << abort_x << ",beacon_y=" << abort_y ;
  publish("BEACON_UPDATE",bc.str());
  publish("HOMING_X",abort_x);
  publish("HOMING_Y",abort_y);

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

void LamssMissionManager::update_utm_datum(double curr_time)
{
  double offset = sqrt( pow(last_nav_.local_fix().x(),2)
			+pow(last_nav_.local_fix().y(),2) );
  // make sure the utm update has propagated at least 30 sec between updates
  if (curr_time - last_utm_update_ >= 30 && offset >= cfg_.update_offset())
    {
      publish("LAT_ORIGIN",last_nav_.global_fix().lat());
      publish("LONG_ORIGIN",last_nav_.global_fix().lon());
      glog.is(VERBOSE) && glog << "New UTM Datum: "
			       << last_nav_.global_fix().lat()
			       << " N, "
			       << last_nav_.global_fix().lon()
			       << " E " << std::endl;
      lon_origin_ = last_nav_.global_fix().lon();
      lat_origin_ = last_nav_.global_fix().lat();
      new_origin_ = true;
      last_utm_update_ = curr_time;
    }

}

void LamssMissionManager::loop()
{
  double curr_time = MOOSTime();
  current_time_ = curr_time;
  // dynamic UTM only for GEO_COMMAND set

  if (new_origin_)
    {
      m_Geodesy.Initialise(lat_origin_,lon_origin_);
      UTM_update();
      new_origin_ = false;
    }
    
  if (last_acomms_ < 1)
    last_acomms_ = curr_time;


  try_publish(); // publish messages queued from state machine

  // wait 30 seconds sending the initial deploy commands
  if (got_gps_nav &&
      !initial_deploy_done_ &&
      curr_time >= mission_start_time_ + 30.0)
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

  double loiter_range =
    sqrt(pow(last_nav_.local_fix().x()-last_deploy_x_,2)+
	 pow(last_nav_.local_fix().y()-last_deploy_y_,2));


  // Here we should add the check for blackout_duration and prosecute_duration.
  boost::lock_guard<boost::mutex> lock(moos_mutex_);

  // Update UTM datum unless on trackline or in survey pattern
#ifdef GEO_COMMAND
  if (cfg_.dynamic_utm() &&
      //	deploy_mode_active_  && !in_track )
      deploy_mode_active_ && survey_wpt_index < 1  && !in_track )
    update_utm_datum(curr_time);	
#endif

  if (descent_state_active_)
    {
      if (touchdown_location_reached_ =
	  (loiter_range <= 2*last_deploy_cmd_.loiter_radius()))
	glog.is(VERBOSE) && glog << "Touchdown location range  " <<
	  loiter_range << " reached" << std::endl;
      ; 
      double touchdown_altitude;
      if (last_deploy_cmd_.has_deploy_altitude())
	touchdown_altitude = last_deploy_cmd_.deploy_altitude();
      else
	touchdown_altitude = 10;
			
      if (last_nav_.global_fix().altitude() > 0 &&
	  last_nav_.global_fix().altitude() <= touchdown_altitude + 10) {
	touchdown_altitude_reached_ = true;
	glog.is(VERBOSE) && glog << "Touchdown Altitude " <<
	  touchdown_altitude << " reached" << std::endl;
      }
      
      if (touchdown_location_reached_ && touchdown_altitude_reached_)  
	{
	  queue_event<EvTouchdown>();
	  touchdown_altitude_cond_.notify_one();
	  goby::glog.is(VERBOSE) && glog
	    << group("helm")
	    << "Touchdown location and depth reached: "
	    << std::endl;
	}
    }

  if (touchdown_state_active_)
    {
  
    }
  if (prosecute_mode_active_)
    {
      double station_range =
	sqrt(pow(last_nav_.local_fix().x()-last_deploy_x_,2)+
	     pow(last_nav_.local_fix().y()-last_deploy_y_,2));
      if (cfg_.operation_radius() > 0 &&
	  station_range >= cfg_.operation_radius_percent()*0.01*cfg_.operation_radius())
	{
	  send_deploy();
	  publish("SURVEY_COMPLETED","TRUE");
	  goby::glog.is(VERBOSE) && glog << group("helm")
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
	  goby::glog.is(VERBOSE) && glog << group("helm")
					 << "Prosecute duration exceeded - commanding Deploy"
					 << std::endl;
	}
      else
	{
	  goby::glog.is(VERBOSE) && glog << group("helm")
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
	  double deploy_time = curr_time - last_deploy_cmd_._time();
	  double stale_acomms = curr_time - last_acomms_;
	  double stale_gps = curr_time - last_gps_;
	  
	  if (last_deploy_cmd_.has_deploy_duration() &&
	      last_deploy_cmd_.deploy_duration() > 0)
	    {
	      if (deploy_time >= last_deploy_cmd_.deploy_duration() ||
		  stale_gps >= last_deploy_cmd_.deploy_duration())
		{
		  goby::glog.is(VERBOSE) && glog << group("helm")
						 << "Deploy duration exceeded w/o acomms or gps - initiating Abort"
						 << "\n"
						 << "Stale: acomms: " << stale_acomms 
						 << ", nav: " << stale_gps
						 << ", duration: "
						 << last_deploy_cmd_.deploy_duration()
						 << std::endl ;
		  // allow auv to get to abort depth before homing
		  last_homing_prosecute_ = current_time_ - 0.5*cfg_.homing_interval();
		  // Turn off sonar
		  publish("SONAR_CONTROL","OFF");
		  publish("SONAR_PROCESS","OFF");
		  
		  if (cfg_.homing_array() == LamssMissionManagerConfig::USBL)
		    publish("USBL_ENABLED","TRUE");
		  queue_event<EvNewMission>();
		  queue_event<EvStartAbort>();
		}
	      else if (cfg_.auto_depth_adaptation() &&
		       !prosecute_mode_active_ &&
		       stale_acomms >= 0.5 * last_deploy_cmd_.deploy_duration() &&
		       !depth_adapting )
		{
		  goby::glog.is(VERBOSE) && glog << group("helm")
						 << "50% of deploy duration exceeded w/o acomms "
						 << " going to acomms depth with collaborator"
						 << last_deploy_cmd_.collaborator()
						 << "\n"
						 << "Stale: acomms: " << stale_acomms 
						 << ", nav: " << stale_gps 
						 << std::endl ;
		  publish("DEPTH_MODE","ADAPTIVE_ACOMMS");
		  // Set acomms depth to current depth
		  std::stringstream ss ;
		  ss << "depth=" << last_nav_.global_fix().depth() ;
		  publish("ACOMMS_DEPTH",ss.str());	  
		  
		  depth_adapting = true;
		}
	      else
		{
		  double remaining = last_deploy_cmd_.deploy_duration() -
		    std::max(stale_acomms,stale_gps);
		  goby::glog.is(VERBOSE) && glog << group("helm")
						 << "Deploy time remaining: " << remaining
						 << "\n"
						 << "Stale: acomms: " << stale_acomms 
						 << ", nav: " << stale_gps 
						 << std::endl;
		  publish("DEPLOY_REMAINING",remaining);
		}
	    }

	  // Update abort mission for ice motion
	  update_abort_mission();
	}
      else if (abort_mode_active_)
	{
	  // Abort_duration is now updated in slow loop, not only at abort time.
	  // double remaining =  abort_duration_ - (curr_time - abort_time_);
	  double remaining =  abort_duration_;
	  if (remaining < 10*slow_loop_period_)
	    {
	      // Enter return mode 
	      goby::glog.is(VERBOSE) && glog << group("helm")
					     << "Entering homing_return" << std::endl;
	      if (cfg_.homing_array() == LamssMissionManagerConfig::USBL)
		{
		  // Make sure homing is on if available before activating return command
		  publish("USBL_ENABLED","TRUE");
		  homing_return();
		}
	      else
		goby::glog.is(VERBOSE) && glog << group("helm")
					       << "Abort time remaining: " << remaining
					       << std::endl;
	    }
	  else
	    {
	      goby::glog.is(VERBOSE) && glog << group("helm")
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
	  // Abort location now constantly updated in slow loop
	  /*
	    abort_duration_ += cfg_.homing_duration() - slow_loop_period_;
	    std::stringstream ss;
	    ss << "duration=" << abort_duration_ ;
	    publish("ABORT_HEADING", ss.str());
	  */
	}
    }

  last_slow_loop_t_ = curr_time;
}

void LamssMissionManager::UTM_update()
{
  // New UTM for invariant deploy lat/long
  m_Geodesy.LatLong2LocalUTM(last_deploy_lat_,last_deploy_lon_,
			     last_deploy_y_,last_deploy_x_);
  deploy_behavior_updates(false);
  // New UTM for invariant target lat/long
  m_Geodesy.LatLong2LocalUTM(last_target_lat_,last_target_lon_,
			     last_target_y_,last_target_x_);
  prosecute_behavior_updates(false);
  // New UTM for invariant abort lat/long
  m_Geodesy.LatLong2LocalUTM(abort_lat,abort_lon,
			     abort_y,abort_x);
  // camp locations
  m_Geodesy.LatLong2LocalUTM(camp_lat_last,camp_lon_last,
			     camp_y_last,camp_x_last);
  m_Geodesy.LatLong2LocalUTM(camp_lat_init,camp_lon_init,
			     camp_y_init,camp_x_init);
  m_Geodesy.LatLong2LocalUTM(camp_lat_deploy,camp_lon_deploy,
			     camp_y_deploy,camp_x_deploy);
  
  
  // Current transit route
  for (int i=0; i<transit_lat.size(); i++)
    {
      double x,y;
      m_Geodesy.LatLong2LocalUTM(transit_lat[i],transit_lon[i],y,x);
      transit_x[i] = x;
      transit_y[i] = y;
    }
  post_transit_route();
  
#ifndef GEO_COMMAND
  // Initial transit route updated for use by return state
  for (int i=0; i<initial_transit_lat.size(); i++)
    {
      double x,y;
      m_Geodesy.LatLong2LocalUTM(initial_transit_lat[i],initial_transit_lon[i],
				 y,x);
      initial_transit_x[i] = x;
      initial_transit_y[i] = y;
      LAMSS_TRANSIT::TransitWPT * wpt = initial_transit_cmd_.mutable_wpt(i);
      wpt->set_x(x);
      wpt->set_y(y);
    }
#endif 

  // Stovepipes. Invariant lat/long
  for (int i=0; i<Stovepipes.size(); i++)
    m_Geodesy.LatLong2LocalUTM(Stovepipes[i].lat,Stovepipes[i].lon,
			       Stovepipes[i].y,Stovepipes[i].x);

}

void LamssMissionManager::send_deploy()
{
#ifndef GEO_COMMAND
  LAMSS_DEPLOY redeploy = last_deploy_cmd_;
#else
  LAMSS_DEPLOY_GEO redeploy = last_deploy_cmd_;
#endif
  std::string serialized;
  redeploy.set__time(MOOSTime());
  redeploy.set__src_id(vehicle_id_);
  redeploy.set__rcv_id(vehicle_id_);
  //  if (!MOOSStrCmp(cfg_.deploy_location(),"station"))
  if (cfg_.deploy_location() == LamssMissionManagerConfig::CURRENT)
    {
#ifndef GEO_COMMAND
      redeploy.set_deploy_x(last_nav_.local_fix().x());
      redeploy.set_deploy_y(last_nav_.local_fix().y());
      // Re-deploy forced to loiter if local
      redeploy.set_deploy_mode(LAMSS_DEPLOY::DEPLOY_MODE_LOITER);
      redeploy.set_depth_mode(LAMSS_DEPLOY::DEPTH_MODE_SINGLE);
#else
      redeploy.set_deploy_longitude(last_nav_.global_fix().lon());
      redeploy.set_deploy_latitude(last_nav_.global_fix().lat());
      // Re-deploy forced to loiter if local
      redeploy.set_deploy_mode(LAMSS_DEPLOY_GEO::DEPLOY_MODE_LOITER);
      redeploy.set_depth_mode(LAMSS_DEPLOY_GEO::DEPTH_MODE_SINGLE);
#endif
    }
  // Auto prosecute only on initial deploy
  redeploy.set_auto_prosecute(false);
  serialize_for_moos(&serialized,redeploy);
#ifndef GEO_COMMAND
  publish("LAMSS_DEPLOY_IN", serialized);
#else
  publish("LAMSS_DEPLOY_GEO_IN", serialized);
#endif
}

void LamssMissionManager::homing_prosecute()
{
#ifndef GEO_COMMAND
  LAMSS_PROSECUTE prosecute_cmd;
  prosecute_cmd.set_target_x(abort_x);
  prosecute_cmd.set_target_y(abort_y);
#else
  LAMSS_PROSECUTE_GEO prosecute_cmd;
  prosecute_cmd.set_target_longitude(abort_lon);
  prosecute_cmd.set_target_latitude(abort_lat);
#endif  
  prosecute_cmd.set__src_id(vehicle_id_);
  prosecute_cmd.set__rcv_id(vehicle_id_);
  prosecute_cmd.set__time(current_time_);
  prosecute_cmd.set_target_number(2);
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
#ifndef GEO_COMMAND
  publish("LAMSS_PROSECUTE_IN", serialized);
#else
  publish("LAMSS_PROSECUTE_GEO_IN", serialized);
#endif  
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
#ifndef GEO_COMMAND
  LAMSS_DEPLOY init_deploy_cmd;
  if (cfg_.has_initial_deploy())
    init_deploy_cmd = cfg_.initial_deploy();
  //    init_deploy_cmd.CopyFrom(cfg_.initial_deploy());
  else
    glog.is(VERBOSE) && glog << "Using default initial deploy" << std::endl;
#else
  LAMSS_DEPLOY_GEO init_deploy_cmd ;
  if (cfg_.has_initial_deploy_geo())
    init_deploy_cmd = cfg_.initial_deploy_geo();
  //    init_deploy_cmd.CopyFrom(cfg_.initial_deploy_geo());
  else
    glog.is(VERBOSE) && glog << "Using default initial deploy" << std::endl;
#endif  
  init_deploy_cmd.set__time(MOOSTime());

  init_deploy_cmd.set__src_id(vehicle_id_);
  init_deploy_cmd.set__rcv_id(vehicle_id_);

  // Auto prosecute only on initial deploy
  if (initial_deploy_done_)
    {
      // Note that the initial will have topside origin to allow resetting
      // deploy_duration by poking for no-modem use.
      //  init_deploy_cmd.set__src_id(cfg_.topside_id());
      //init_deploy_cmd.set__src_id(vehicle_id_);
      init_deploy_cmd.set_auto_prosecute(false);
      // for auto redeploy, center at current abort location
      // This is important for dynamic scenarios such as in ICEX
#ifndef GEO_COMMAND
      init_deploy_cmd.set_deploy_x(abort_x);
      init_deploy_cmd.set_deploy_y(abort_y);
      // If adapt depth mode, keep it for survey return
      if (depth_adapting)
	init_deploy_cmd.set_depth_mode
	  (LAMSS_DEPLOY::DEPTH_MODE_ADAPTIVE_ACOMMS);
#else
      init_deploy_cmd.set_deploy_longitude(abort_lon);
      init_deploy_cmd.set_deploy_latitude(abort_lat);
      // If adapt depth mode, keep it for survey return
      if (depth_adapting)
	init_deploy_cmd.set_depth_mode
	  (LAMSS_DEPLOY_GEO::DEPTH_MODE_ADAPTIVE_ACOMMS);
#endif      

      init_deploy_cmd.set_deploy_upper_depth(cfg_.abort_depth());
      // If stale acomms, continue with current duration timer
      double stale_acomms = MOOSTime() - last_acomms_;
      double stale_gps = MOOSTime() - last_gps_;
      double remaining = last_deploy_cmd_.deploy_duration() - stale_acomms;
      if (remaining < init_deploy_cmd.deploy_duration())
	init_deploy_cmd.set_deploy_duration(remaining);

    }
  else
    {
      // Initialize default acomms depth
      std::stringstream ss;
      ss << "depth=" << init_deploy_cmd.deploy_upper_depth() ;
      publish("ACOMMS_DEPTH",ss.str());	  
    }
  std::string serialized;
  serialize_for_moos(&serialized,init_deploy_cmd);
#ifndef GEO_COMMAND
  publish("LAMSS_DEPLOY_IN", serialized);
#else
  publish("LAMSS_DEPLOY_GEO_IN", serialized);
#endif  
  // Initial deploy counts as acomms
  last_acomms_ = MOOSTime();
}

void LamssMissionManager::homing_return()
{
  // Handles OUTGOING_COMMANDS for ownship, usually Initial Deploy
#ifndef GEO_COMMAND
  LAMSS_DEPLOY homing_cmd;
  if (cfg_.has_initial_deploy())
    homing_cmd.CopyFrom(cfg_.initial_deploy());
  else
    glog.is(VERBOSE) && glog << "Using default initial deploy" << std::endl;
  homing_cmd.set_deploy_mode(LAMSS_DEPLOY::DEPLOY_MODE_RETURN);
#else
  LAMSS_DEPLOY_GEO homing_cmd;
  if (cfg_.has_initial_deploy_geo())
    homing_cmd.CopyFrom(cfg_.initial_deploy_geo());
  else
    glog.is(VERBOSE) && glog << "Using default initial deploy" << std::endl;
  homing_cmd.set_deploy_mode(LAMSS_DEPLOY_GEO::DEPLOY_MODE_RETURN);
#endif
  homing_cmd.set__time(MOOSTime());

  homing_cmd.set__rcv_id(vehicle_id_);

  // Note that the initial will have topside origin to allow resetting
  // deploy_duration by poking for no-modem use.
  //  init_deploy_cmd.set__src_id(cfg_.topside_id());
  homing_cmd.set__src_id(vehicle_id_);
  homing_cmd.set_auto_prosecute(false);
  // for auto redeploy, center at current abort location
  // This is important for dynamic scenarios such as in ICEX
#ifndef GEO_COMMAND
  homing_cmd.set_deploy_x(abort_x);
  homing_cmd.set_deploy_y(abort_y);
#else
  homing_cmd.set_deploy_latitude(abort_lat);
  homing_cmd.set_deploy_longitude(abort_lon);
#endif  
  homing_cmd.set_homing_enabled(true);
  // Force non abort in case we lost comms
  last_acomms_ = MOOSTime();
  std::string serialized;
  serialize_for_moos(&serialized,homing_cmd);
#ifndef GEO_COMMAND
  publish("LAMSS_DEPLOY_IN", serialized);
#else
  publish("LAMSS_DEPLOY_GEO_IN", serialized);
#endif

}

void LamssMissionManager::initial_transit()
{
  // Handles OUTGOING_COMMANDS for ownship, usually Initial Deploy
#ifndef GEO_COMMAND
  LAMSS_TRANSIT init_transit_cmd;
  if (cfg_.has_initial_transit())
    init_transit_cmd.CopyFrom(cfg_.initial_transit());
  else
    glog.is(VERBOSE) && glog << "Using default initial transit" << std::endl;
#else
  LAMSS_TRANSIT_GEO init_transit_cmd;
  if (cfg_.has_initial_transit_geo())
    init_transit_cmd.CopyFrom(cfg_.initial_transit_geo());
  else
    glog.is(VERBOSE) && glog << "Using default initial transit" << std::endl;
#endif
  init_transit_cmd.set__time(MOOSTime());

  // Note that the initial will have topside origin to allow resetting
  // deploy_duration by poking for no-modem use.
  init_transit_cmd.set__src_id(cfg_.topside_id());
  init_transit_cmd.set__rcv_id(vehicle_id_);

  std::string serialized;
  serialize_for_moos(&serialized,init_transit_cmd);
#ifndef GEO_COMMAND
  publish("LAMSS_TRANSIT_IN", serialized);
#else
  publish("LAMSS_TRANSIT_GEO_IN", serialized);
#endif  
  int n = initial_transit_cmd_.wpt_size();
  initial_transit_x.resize(n);
  initial_transit_y.resize(n);
  initial_transit_lat.resize(n);
  initial_transit_lon.resize(n);
  for (int i=0; i<n; i++)
    {
#ifndef GEO_COMMAND
      initial_transit_x[i] = initial_transit_cmd_.wpt(i).x();
      initial_transit_y[i] = initial_transit_cmd_.wpt(i).y();
      double lat,lon;
      m_Geodesy.UTM2LatLong(initial_transit_x[i],initial_transit_y[i],lat,lon);
      initial_transit_lat[i] = lat;
      initial_transit_lon[i] = lon;
#else
      initial_transit_lat[i] = initial_transit_cmd_.wpt(i).lat();
      initial_transit_lon[i] = initial_transit_cmd_.wpt(i).lon();
      double x,y;
      m_Geodesy.LatLong2LocalUTM(initial_transit_lat[i],initial_transit_lon[i],
				 y,x);
      initial_transit_x[i] = x;
      initial_transit_y[i] = y;
#endif
    }
}

void LamssMissionManager::handle_ifs_data(const CMOOSMsg& msg)
{
  goby::middleware::frontseat::protobuf::InterfaceData data;
  parse_for_moos(msg.GetString(), &data);

  if(data.has_node_status())
    {
      last_nav_ = data.node_status();

      if (!cfg_.navigating_with_hydroman())
	{
	  // post NAV_DATA
	  std::string serialized;
	  serialize_for_moos(&serialized,last_nav_);
	  publish("NAV_DATA", serialized);

	  // post NAV_*
	  publish("NAV_X", last_nav_.local_fix().x());
	  publish("NAV_Y", last_nav_.local_fix().y());
	  publish("NAV_LAT", last_nav_.global_fix().lat());
	  publish("NAV_LONG", last_nav_.global_fix().lon());
	  if (last_nav_.local_fix().has_z())
	    publish("NAV_Z", last_nav_.local_fix().z());
	  if (last_nav_.global_fix().has_depth())
	    publish("NAV_DEPTH", last_nav_.global_fix().depth());
	  const double pi = 3.14159;
	  if (last_nav_.pose().has_heading())
	    {
	      double yaw = -last_nav_.pose().heading() * pi / 180.0;
	      while (yaw < -pi) yaw += 2 * pi;
	      while (yaw >= pi) yaw -= 2 * pi;
	      publish("NAV_YAW", yaw);
	      publish("NAV_HEADING", last_nav_.pose().heading());
	    }	
	  publish("NAV_SPEED", last_nav_.speed().over_ground());	
	  if (last_nav_.pose().has_pitch())
	    publish("NAV_PITCH", last_nav_.pose().pitch() * pi / 180.0);
	  if (last_nav_.pose().has_roll())
	    publish("NAV_ROLL", last_nav_.pose().roll() * pi / 180.0);
	  if (last_nav_.global_fix().has_altitude())
	    publish("NAV_ALTITUDE", last_nav_.global_fix().altitude());	
			
        }
      // surface for GPS variable
      if (last_nav_.has_source() &&
	  last_nav_.source().position() == goby::middleware::frontseat::protobuf::Source::GPS)
	{
	  
	  if (!got_gps_nav)
	    {
	      if (cfg_.dynamic_opregion())
		{
		  // Set initial camp/launch position from GPS
		  camp_x_last = last_nav_.local_fix().x();
		  camp_y_last = last_nav_.local_fix().y();
		  camp_lat_last = last_nav_.global_fix().lat();
		  camp_lon_last = last_nav_.global_fix().lon();
		  
		  camp_x_init = last_nav_.local_fix().x();
		  camp_y_init = last_nav_.local_fix().y();
		  camp_lat_init = last_nav_.global_fix().lat();
		  camp_lon_init = last_nav_.global_fix().lon();

		  camp_x_deploy = last_nav_.local_fix().x();
		  camp_y_deploy = last_nav_.local_fix().y();
		  camp_lat_deploy = last_nav_.global_fix().lat();
		  camp_lon_deploy = last_nav_.global_fix().lon();
		}
	    }
	  std::stringstream ss;
	  ss << "Timestamp=" << std::setprecision(15) << last_nav_.time();
	  publish("GPS_UPDATE_RECEIVED", ss.str());
	  got_gps_nav = true;
	  last_gps_ = last_nav_.time();
	}	

    }
  // Handle landing and takeoff
  boost::lock_guard<boost::mutex> lock(moos_mutex_);
  operation_altitude_ = false;
  if (last_nav_.global_fix().altitude() >=
      cfg_.minimum_altitude() )
        {
      operation_altitude_ = true;
      operation_altitude_cond_.notify_one();
    }
  else
    operation_altitude_ = false;
  // Template: declare landing when less than 0.5 m altitude
  if (touchdown_state_active_ && last_nav_.global_fix().altitude() <= cfg_.touchdown_altitude())
    {
      touchdown_completed_ = true;
      queue_event<EvBottoming>();
    }
}

void LamssMissionManager::handle_hydroman_data(const CMOOSMsg& msg)
{
  goby::middleware::frontseat::protobuf::NodeStatus data;
  parse_for_moos(msg.GetString(), &data);

  if (cfg_.navigating_with_hydroman() == true)
    {
      // post NAV_*
      publish("NAV_X", data.local_fix().x());
      publish("NAV_Y", data.local_fix().y());
        
      double lat;
      double lon;
      if (!m_Geodesy.UTM2LatLong(data.local_fix().x(),data.local_fix().y(),lat,lon))
    	{
	  glog.is(DEBUG2) && glog
	    << "Error in UTM2LatLong conversion" << std::endl;
    	}
      publish("NAV_LAT", lat);
      publish("NAV_LONG",lon);
        
      if (data.local_fix().has_z())
	{
	  publish("NAV_Z", data.local_fix().z());
	}       
      if (data.global_fix().has_depth())
	{
	  publish("NAV_DEPTH", data.global_fix().depth());
	}		
      const double pi = 3.14159;
      if (data.pose().has_heading())
        {
	  double yaw = -data.pose().heading() * pi / 180.0;
	  while (yaw < -pi) yaw += 2 * pi;
	  while (yaw >= pi) yaw -= 2 * pi;
	  publish("NAV_YAW", yaw);
	  publish("NAV_HEADING", data.pose().heading());
	}	
      publish("NAV_SPEED", data.speed().over_ground());	
      if (data.pose().has_pitch())
	publish("NAV_PITCH", data.pose().pitch() * pi / 180.0);
      if (data.pose().has_roll())
	publish("NAV_ROLL", data.pose().roll() * pi / 180.0);
      if (data.global_fix().has_altitude())
	publish("NAV_ALTITUDE", data.global_fix().altitude());	
      // surface for GPS variable
        
      if (data.has_source() &&
	  data.source().position() == goby::middleware::frontseat::protobuf::Source::GPS)
        {
	  std::stringstream ss;
	  ss << "Timestamp=" << std::setprecision(15) << data.time();
	  publish("GPS_UPDATE_RECEIVED", ss.str());
	  got_gps_nav = true;
	  last_gps_ = MOOSTime();
	}
	
      // post NAV_DATA
      data.mutable_global_fix()->set_lat(lat);
      data.mutable_global_fix()->set_lon(lon);
      data.mutable_global_fix()->set_depth(data.global_fix().depth());
      std::string serialized;
      serialize_for_moos(&serialized,data);
      publish("NAV_DATA", serialized);
      last_nav_ = data;
    }

  // Handle landing and takeoff
  boost::lock_guard<boost::mutex> lock(moos_mutex_);
  operation_altitude_ = false;
  if (last_nav_.global_fix().altitude() >=
      cfg_.minimum_altitude())
    {
      operation_altitude_ = true;
      operation_altitude_cond_.notify_one();
    }
  else
    operation_altitude_ = false;
  // Template: declare landing when less than 0.5 m altitude
  if (touchdown_state_active_ && last_nav_.global_fix().altitude() <= 1.0)
    {
      touchdown_completed_ = true;
      queue_event<EvBottoming>();
    }
}

void LamssMissionManager::handle_deploy(const CMOOSMsg& msg)
{
#ifndef GEO_COMMAND
  LAMSS_DEPLOY deploy_cmd;
#else
  LAMSS_DEPLOY_GEO deploy_cmd;
#endif
  parse_for_moos(msg.GetString(), &deploy_cmd);
  // Only handle messages for me
  if (ownship_modem_ids_.count(deploy_cmd._rcv_id()))
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
      m_Geodesy.UTM2LatLong(camp_x_deploy,camp_y_deploy,
			    camp_lat_deploy,camp_lon_deploy);
      
      // save x,y and lat/long for fututrre UTM update handling
#ifndef GEO_COMMAND
      // for standard UTM LAMSS_DEPLOY command
      last_deploy_x_ = last_deploy_cmd_.deploy_x();
      last_deploy_y_ = last_deploy_cmd_.deploy_y();
      m_Geodesy.UTM2LatLong(last_deploy_x_,last_deploy_y_,
			    last_deploy_lat_,last_deploy_lon_);
#else
      // for UTM_LAMSS_GEO command
      last_deploy_lat_ = last_deploy_cmd_.deploy_latitude();
      last_deploy_lon_ = last_deploy_cmd_.deploy_longitude();
      m_Geodesy.LatLong2LocalUTM(last_deploy_lat_,last_deploy_lon_,
				 last_deploy_y_,last_deploy_x_);
#endif      
      post_transit_route();

      if (deploy_cmd._src_id() == cfg_.topside_id())
	last_acomms_ = MOOSTime();
      glog.is(VERBOSE) && glog << "Initial Deploy X: " << last_deploy_x_
			       << " Y: " << last_deploy_y_
			       << " Lat: " << last_deploy_lat_
			       << " Lon: " << last_deploy_lon_ << std::endl;
      // reset deploy flags
      publish("SURVEY_WPT_FLAG",0);
      publish("IN_TRACK","FALSE");
      
      // Update variables for HelmIvP

      deploy_behavior_updates(true);

      // Liftoff if bottomed

      // Queue state transitions
      if (mission_active_) {
	if (bottoming_state_active_)
	  queue_event<EvTakeOff>();
	queue_event<EvNewMission>();
      }
#ifndef GEO_COMMAND
      if (last_deploy_cmd_.deploy_mode() == LAMSS_DEPLOY::DEPLOY_MODE_ABORT)
#else
	if (last_deploy_cmd_.deploy_mode() == LAMSS_DEPLOY_GEO::DEPLOY_MODE_ABORT)
#endif
	  {
	    // allow auv to get to abort depth before homing
	    last_homing_prosecute_ = current_time_ - 0.5*cfg_.homing_interval();
	    update_abort_mission();
	    queue_event<EvStartAbort>();
	  }
#ifndef GEO_COMMAND
	else if (last_deploy_cmd_.deploy_mode() ==
		 LAMSS_DEPLOY::DEPLOY_MODE_RETURN)
#else
	else if (last_deploy_cmd_.deploy_mode() ==
		 LAMSS_DEPLOY_GEO::DEPLOY_MODE_RETURN)
#endif
	  {
	    queue_event<EvStartDeploy>();
	    queue_event<EvReturnToBase>();
	  }
#ifndef GEO_COMMAND
	else if (last_deploy_cmd_.deploy_mode() ==
		 LAMSS_DEPLOY::DEPLOY_MODE_LANDING)
#else
	else if (last_deploy_cmd_.deploy_mode() ==
		 LAMSS_DEPLOY_GEO::DEPLOY_MODE_LANDING)
#endif
	  {
	    queue_event<EvStartDeploy>();
	    queue_event<EvStartLanding>();
	  }
	else
	  queue_event<EvStartDeploy>();
	  
    }
}

void LamssMissionManager::deploy_behavior_updates(bool new_deploy)
{
  // new_deploy=true indicates a new deploy command
  // new_deploy=false indicates a change in UTM datum, and behaviors inherently
  // handling that should not be updated.
  
  // Setting state variables
#ifndef GEO_COMMAND
  LAMSS_DEPLOY::Deploy_ModeEnum deploy_mode = last_deploy_cmd_.deploy_mode();
  std::string deploy_mode_str = LAMSS_DEPLOY::Deploy_ModeEnum_Name(deploy_mode);
  MOOSChomp(deploy_mode_str,"DEPLOY_MODE_");
  publish("DEPLOY_MISSION",deploy_mode_str);

  std::string depth_mode_str =
    LAMSS_DEPLOY::Depth_ModeEnum_Name(last_deploy_cmd_.depth_mode());
  MOOSChomp(depth_mode_str,"DEPTH_MODE_");
  publish("DEPTH_MODE",depth_mode_str);

  std::string sonar_control_str =
    LAMSS_DEPLOY::Sonar_ControlEnum_Name(last_deploy_cmd_.sonar_control());
  MOOSChomp(sonar_control_str,"SONAR_CONTROL_");
  publish("SONAR_CONTROL",sonar_control_str);

  std::string sonar_process_str =
    LAMSS_DEPLOY::Sonar_ProcessEnum_Name(last_deploy_cmd_.sonar_process());
  MOOSChomp(sonar_process_str,"SONAR_PROCESS_");
  publish("SONAR_PROCESS",sonar_process_str);
  std::string collab_str;
  if (!last_deploy_cmd_.has_collaborator() ||
      last_deploy_cmd_.collaborator() == LAMSS_DEPLOY::DEFAULT)
    collab_str = "contact=" + collaborator_name;
  else
    {
      LAMSS_DEPLOY::ContactEnum collab = last_deploy_cmd_.collaborator();
      collab_str = "contact=" + LAMSS_DEPLOY::ContactEnum_Name(collab);
    }
#else
  LAMSS_DEPLOY_GEO::Deploy_ModeEnum deploy_mode = last_deploy_cmd_.deploy_mode();
  std::string deploy_mode_str = LAMSS_DEPLOY_GEO::Deploy_ModeEnum_Name(deploy_mode);
  MOOSChomp(deploy_mode_str,"DEPLOY_MODE_");
  publish("DEPLOY_MISSION",deploy_mode_str);

  std::string depth_mode_str =
    LAMSS_DEPLOY_GEO::Depth_ModeEnum_Name(last_deploy_cmd_.depth_mode());
  MOOSChomp(depth_mode_str,"DEPTH_MODE_");
  publish("DEPTH_MODE",depth_mode_str);

  std::string sonar_control_str =
    LAMSS_DEPLOY_GEO::Sonar_ControlEnum_Name(last_deploy_cmd_.sonar_control());
  MOOSChomp(sonar_control_str,"SONAR_CONTROL_");
  publish("SONAR_CONTROL",sonar_control_str);

  std::string sonar_process_str =
    LAMSS_DEPLOY_GEO::Sonar_ProcessEnum_Name(last_deploy_cmd_.sonar_process());
  MOOSChomp(sonar_process_str,"SONAR_PROCESS_");
  publish("SONAR_PROCESS",sonar_process_str);

  std::string collab_str;
  if (!last_deploy_cmd_.has_collaborator() ||
      last_deploy_cmd_.collaborator() == LAMSS_DEPLOY_GEO::DEFAULT)
    collab_str = "contact=" + collaborator_name;
  else
    {
      LAMSS_DEPLOY_GEO::ContactEnum collab = last_deploy_cmd_.collaborator();
      collab_str = "contact=" + LAMSS_DEPLOY_GEO::ContactEnum_Name(collab);
    }

#endif
  
  // Setting behaviour update variables
  
  std::stringstream ds;
  ds << "point=" << last_deploy_x_
     << "," << last_deploy_y_
     << "#speed=" << last_deploy_cmd_.deploy_speed();
  publish("DEPLOY_STATION",ds.str());
  publish("ORBIT_RADIUS",last_deploy_cmd_.loiter_radius());	

  std::string clkw;
  if (last_deploy_cmd_.clockwise())
    clkw = "true";
  else
    clkw = "false";
      

  std::stringstream sd;
  sd << "polygon=radial:" << last_deploy_x_
     << "," << last_deploy_y_
     << "," << last_deploy_cmd_.loiter_radius()
     << "," << last_deploy_cmd_.loiter_segments()
     << "#clockwise=" << clkw
     << "#speed=" << last_deploy_cmd_.deploy_speed();
  publish("SENSOR_DEPLOY",sd.str());
  
  // Custumized behaviors handling UTM updates inherently
  if (new_deploy)
    {
      std::stringstream srt;
      srt << "points=" << last_deploy_x_
	  << "," << last_deploy_y_
	  << "#heading=" << last_deploy_cmd_.survey_heading()
	  << "#track_length=" << last_deploy_cmd_.survey_length()
	  << "#track_width=" << last_deploy_cmd_.survey_width()
	  << "#clockwise=" << clkw
	  << "#speed=" << last_deploy_cmd_.deploy_speed();
      publish("SENSOR_RACETRACK",srt.str());
      
      std::stringstream szc;
      szc << "heading=" << last_deploy_cmd_.survey_heading()
	  << "#period=" << last_deploy_cmd_.survey_period()
	  << "#amplitude=" << last_deploy_cmd_.survey_width()
	  << "#speed=" << last_deploy_cmd_.deploy_speed();
      publish("ZIGZAG_CONFIG",szc.str());
    }
  std::stringstream szs;
  szs  << "points=zigzag:" << last_deploy_x_
       << "," << last_deploy_y_
       << "," << last_deploy_cmd_.survey_heading()
       << "," << last_deploy_cmd_.survey_length()
       << "," << last_deploy_cmd_.survey_period()
       << "," << last_deploy_cmd_.survey_width()
       << "#currix=" << survey_wpt_index;
  //       << "#capture_line=true" ;
  publish("ZIGZAG_STATUS",szs.str());
  
  std::stringstream slm;
  slm  << "points=format=lawnmower"
       << ",x=" << last_deploy_x_
       << ",y=" << last_deploy_y_
       << ",height=" << last_deploy_cmd_.survey_length()
       << ",width=" << last_deploy_cmd_.survey_width()
       << ",lane_width=" << last_deploy_cmd_.survey_period()
       << ",rows=east-west,degs=" << last_deploy_cmd_.survey_heading()
       << "#speed=" << last_deploy_cmd_.deploy_speed()
       << "#currix=" << survey_wpt_index;
  //       << "#capture_line=true" ;
  publish("SURVEY_CONFIG",slm.str());

  // Front mapping behaviors
  std::stringstream ssu;
  ssu  << "spiral_center_x=" << last_deploy_x_
       << "#spiral_center_y=" << last_deploy_y_
       << "#spiral_start_radius=" << last_deploy_cmd_.loiter_radius()
    //      << "#spiral_angle=" << last_deploy_cmd_.trail_angle()
       << "#speed=" << last_deploy_cmd_.deploy_speed();
  publish("SPIRAL_UPDATE",ssu.str());

  std::stringstream stf;
  stf  << "speed=" << last_deploy_cmd_.deploy_speed()
       << "#helix_ctr_depth=" << last_deploy_cmd_.deploy_upper_depth();
  publish("DEPLOY_TRACK_FRONT_UPDATE",stf.str());

  std::stringstream shu;
  shu  << "helix_start_x=" << last_deploy_x_
       << "#helix_start_y=" << last_deploy_y_
       << "#helix_radius=" << last_deploy_cmd_.loiter_radius()
       << "#helix_constant=" << last_deploy_cmd_.survey_period()
       << "#helix_ctr_depth=" << last_deploy_cmd_.deploy_upper_depth()
       << "#helix_hdg=" << last_deploy_cmd_.survey_heading()
       << "#clockwise=" << clkw
       << "#speed=" << last_deploy_cmd_.deploy_speed();
  publish("DEPLOY_HELIX_UPDATE",shu.str());

  std::stringstream sstu;
  sstu  << "tgt_x=" << last_deploy_x_
	<< "#tgt_y=" << last_deploy_y_;
  publish("TGT_TO_CLASS",sstu.str());
  sstu  << "#max_radius=" << last_deploy_cmd_.loiter_radius()
	<< "#min_depth=" << last_deploy_cmd_.deploy_upper_depth()
	<< "#max_depth=" << last_deploy_cmd_.deploy_lower_depth();
  publish("SAMPLE_UPDATE",sstu.str());

  
  // Mission parameter values
  
  std::stringstream sdd;
  sdd  << "depth=" << last_deploy_cmd_.deploy_upper_depth();
  publish("SENSOR_DEPTH_DEPLOY",sdd.str());
  publish("SENSOR_DEPTH_TRACKING",sdd.str());

  std::stringstream sda;
  sda  << "depth=" << last_deploy_cmd_.deploy_lower_depth();
  publish("SENSOR_DEPTH_ALTERNATE",sda.str());

  std::stringstream sa;
  sa  << "altitude=" << last_deploy_cmd_.deploy_altitude();
  publish("SENSOR_ALTITUDE",sa.str());

  std::stringstream gc;
  gc  << "period=" << last_deploy_cmd_.gps_interval();
  publish("GPS_CONFIGURATION",gc.str());

  std::stringstream syy;
  syy  << "min_depth=" << last_deploy_cmd_.deploy_upper_depth()
       << "#max_depth=" << last_deploy_cmd_.deploy_lower_depth();
  publish("SENSOR_YOYO",syy.str());
  syy << "#period=" << last_deploy_cmd_.survey_period();
  publish("SMART_YOYO",syy.str());
      
  std::stringstream dsp;
  dsp <<  "speed=" << last_deploy_cmd_.deploy_speed();
  publish("DEPLOY_SPEED",dsp.str());
      
  std::stringstream shg;
  shg  << "heading=" << last_deploy_cmd_.survey_heading();
  publish("SURVEY_HEADING",shg.str());
      
  publish("ADAPTIVE_DEPTH", collab_str);

  publish("TOGGLE_UPPER_DEPTH",last_deploy_cmd_.deploy_upper_depth());
  publish("TOGGLE_LOWER_DEPTH",last_deploy_cmd_.deploy_lower_depth());
  
#ifndef GEO_COMMAND
  // Trail behaviors
  std::stringstream stc;
  stc << collab_str
      << "#trail_range=" << last_deploy_cmd_.trail_range()
      << "#trail_angle=" << last_deploy_cmd_.trail_angle();
  if (last_deploy_cmd_.trail_angle_type() ==
      LAMSS_DEPLOY::TRAIL_ANGLE_TYPE_relative)
    stc << "#trail_angle_type=relative";
  else
    stc << "#trail_angle_type=absolute";

  publish("TRAIL_CONFIG",shg.str());
#endif

     	
#ifndef GEO_COMMAND
  if (last_deploy_cmd_.depth_mode() ==
      LAMSS_DEPLOY::DEPTH_MODE_ADAPTIVE_ACOMMS)
#else
    if (last_deploy_cmd_.depth_mode() ==
	LAMSS_DEPLOY_GEO::DEPTH_MODE_ADAPTIVE_ACOMMS)
#endif    
      {
	// Set acomms depth to current depth
	std::stringstream ss ;
	ss << "depth=" << last_nav_.global_fix().depth() ;
	publish("ACOMMS_DEPTH",ss.str());	  
	depth_adapting = true;
      }
    else
      depth_adapting = false;

  publish("SURVEY_COMPLETED","FALSE");
      
  // USBL homing enabled in DEPLOY/ABORT modes with USBL configured
  if (last_deploy_cmd_.homing_enabled() &&
#ifndef GEO_COMMAND
      last_deploy_cmd_.deploy_mode() == LAMSS_DEPLOY::DEPLOY_MODE_RETURN &&
#else
      last_deploy_cmd_.deploy_mode() == LAMSS_DEPLOY_GEO::DEPLOY_MODE_RETURN &&
#endif      
      cfg_.homing_array() == LamssMissionManagerConfig::USBL)
    {
      std::stringstream cent;
      cent << "center_assign="
	   << last_deploy_x_
	   << "," << last_deploy_y_;
      publish("HOMING_CONFIG",cent.str());
      publish("USBL_ENABLED","TRUE");
    }
  // USBL Homing for recovery
  else if (last_deploy_cmd_.homing_enabled() &&
#ifndef GEO_COMMAND
	   last_deploy_cmd_.deploy_mode() ==
	   LAMSS_DEPLOY::DEPLOY_MODE_RECOVERY &&
#else
	   last_deploy_cmd_.deploy_mode() ==
	   LAMSS_DEPLOY_GEO::DEPLOY_MODE_RECOVERY &&
#endif	   
	   cfg_.homing_array() == LamssMissionManagerConfig::USBL)
    {
      std::stringstream wpt;
      wpt << "points="
	  << abort_x
	  << "," << abort_y
	  << "#radius=" << last_deploy_cmd_.deploy_upper_depth()
	  << "#nm_radius=" << 2*last_deploy_cmd_.deploy_upper_depth();
      publish("HOMING_STATION",wpt.str());
      publish("USBL_ENABLED","TRUE");
    }
#ifndef GEO_COMMAND
  else if (last_deploy_cmd_.deploy_mode() == LAMSS_DEPLOY::DEPLOY_MODE_ABORT &&
#else
  else if (last_deploy_cmd_.deploy_mode() == LAMSS_DEPLOY_GEO::DEPLOY_MODE_ABORT &&
#endif	   
    cfg_.homing_array() == LamssMissionManagerConfig::USBL)
		  publish("USBL_ENABLED","TRUE");
  else
    publish("USBL_ENABLED","FALSE");

	   //
	   // Glider yoyo behavior
	   //
	   std::stringstream transit_yoyo;
	   transit_yoyo << "min_depth=" << last_deploy_cmd_.deploy_upper_depth()
	   << "#max_depth=" << last_deploy_cmd_.deploy_lower_depth() ;
	   double period = (2*last_deploy_cmd_.deploy_lower_depth() -
			    last_deploy_cmd_.deploy_lower_depth())/cos(M_PI/12);
	   std::stringstream pstr;
	   pstr << "#period=" << period ;
	   publish("SMART_YOYO", transit_yoyo.str() + pstr.str());
	   //
	   // Dynamic setting of glideslope and speed
	   //
	   int npar = cfg_.glide_parameters().size();
	   std::stringstream glideparameters;
	   if (npar > 0)
	     {
	       double diff_up = 1e6;
	       int closest_up = -1;
	       double diff_dn = 1e6;
	       int closest_dn = -1;
	       for (int i=0; i<npar; i++)
		 {
		   if (cfg_.glide_parameters(i).slope() >0 &&
		       fabs(last_deploy_cmd_.deploy_speed()-cfg_.glide_parameters(i).speed()) <=
		       diff_up)
		     {
		       closest_up = i;
		       diff_up = fabs(last_deploy_cmd_.deploy_speed()-cfg_.glide_parameters(i).speed());
		     }
		   else if (cfg_.glide_parameters(i).slope() <= 0 &&
			    fabs(last_deploy_cmd_.deploy_speed()-cfg_.glide_parameters(i).speed()) <=
			    diff_dn)
		     {
		       closest_dn = i;
		       diff_dn = fabs(last_deploy_cmd_.deploy_speed()-cfg_.glide_parameters(i).speed());
		     }
		 }
	       if (closest_up >= 0 && closest_dn >=0)
		 {
		   glideparameters << "#glideslopes=";
		   for (int i=0; i<npar; i++)
		     {
		       glideparameters << cfg_.glide_parameters(i).slope();
		       if (i < npar-1)
			 glideparameters << ",";
		     }
		   glideparameters << "#glidespeeds=";
		   for (int i=0; i<npar; i++)
		     {
		       glideparameters << cfg_.glide_parameters(i).speed();
		       if (i < npar-1)
			 glideparameters << ",";
		     }
		   glideparameters << "#utilities=";
		   for (int i=0; i<npar; i++)
		     {
		       if (i == closest_up)
			 glideparameters << cfg_.glide_parameters(i).utility();
		       else if (i == closest_dn)
			 glideparameters << cfg_.glide_parameters(i).utility();
		       else
			 glideparameters << 0;
		       if (i < npar-1)
			 glideparameters << ",";
		     }
		 }
	       else
		 {
		   glog.is_die() && glog << "Configuration Error: Both negative and positive valid glidelopes required"
		     
					 << std::endl;
		 }
	       
	     }
	   publish("GLIDER_TRANSIT_YOYO", transit_yoyo.str() + glideparameters.str());
	   
	   }
  
  void LamssMissionManager::handle_prosecute(const CMOOSMsg& msg)
  {
#ifndef GEO_COMMAND
    LAMSS_PROSECUTE prosecute_cmd;
#else
    LAMSS_PROSECUTE_GEO prosecute_cmd;
#endif   
    parse_for_moos(msg.GetString(), &prosecute_cmd);
    // Only handle messages for me
    if (ownship_modem_ids_.count(prosecute_cmd._rcv_id()))
      {
	boost::lock_guard<boost::mutex> lock(moos_mutex_);
	last_prosecute_cmd_ = prosecute_cmd;

	if (prosecute_cmd._src_id() == cfg_.topside_id())
	  last_acomms_ = MOOSTime();

	// save x,y and lat/long for UTM update handling
#ifndef GEO_COMMAND
	// for standard UTM MANTARAY_DEPLOY command
	last_target_x_ = last_prosecute_cmd_.target_x();
	last_target_y_ = last_prosecute_cmd_.target_y();
	m_Geodesy.UTM2LatLong(last_target_x_,last_target_y_,
			      last_target_lat_,last_target_lon_);
#else
	// for UTM_MANTARAY_GEO command
	last_target_lat_ = last_prosecute_cmd_.target_latitude();
	last_target_lon_ = last_prosecute_cmd_.target_longitude();
	m_Geodesy.LatLong2LocalUTM(last_target_lat_,last_target_lon_,
				   last_target_y_,last_target_x_);
#endif      

	prosecute_behavior_updates(true);
	  
	// Turn sonar and processing on
     
	publish("SONAR_CONTROL","ON");
	publish("SONAR_PROCESS","ON");
	publish("GOTO_SURFACE","FALSE");
     
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
	    goby::glog.is(VERBOSE) && glog << group("fsm")
					   << "Not in active mission -  Prosecute ignored"
					   << std::endl;
	  }
      }
  }

  void LamssMissionManager::prosecute_behavior_updates(bool new_prosecute)
  {

    // Setting state variables

#ifndef GEO_COMMAND
    LAMSS_PROSECUTE::Prosecute_TypeEnum prosecute_type =
      last_prosecute_cmd_.prosecute_type();
    std::string prosecute_type_str =
      LAMSS_PROSECUTE::Prosecute_TypeEnum_Name(prosecute_type);
    std::string angle_mode =
      LAMSS_PROSECUTE::Array_Angle_ModeEnum_Name(last_prosecute_cmd_.array_angle_mode());
#else
    LAMSS_PROSECUTE_GEO::Prosecute_TypeEnum prosecute_type =
      last_prosecute_cmd_.prosecute_type();
    std::string prosecute_type_str =
      LAMSS_PROSECUTE_GEO::Prosecute_TypeEnum_Name(prosecute_type);
    std::string angle_mode =
      LAMSS_PROSECUTE_GEO::Array_Angle_ModeEnum_Name(last_prosecute_cmd_.array_angle_mode());
#endif  
    MOOSChomp(prosecute_type_str,"PROSECUTE_TYPE_");
    publish("PROSECUTE_MISSION",prosecute_type_str);

    // Behavior updates
  
    // Force single depth for prosecute
    publish("DEPTH_MODE","SINGLE");
  
    std::stringstream sti;
    sti  << "TGT_=" << last_prosecute_cmd_.target_number();
    publish("TARGET_ID",sti.str());
      
    std::stringstream smt;
    smt  << "contact=" << last_prosecute_cmd_.target_number();
    publish("MY_TARGET",smt.str());
  
    std::stringstream spt;
    spt  << "contact=TGT_" << last_prosecute_cmd_.target_number();
    publish("PRIORITY_TRK",spt.str());
    publish("CURRENT_TARGET",spt.str());
      
    MOOSChomp(angle_mode,"ARRAY_ANGLE_MODE_");
    std::stringstream sta;
    sta  << "bearing_mode=" << angle_mode
	 << "#desired_angle=" << last_prosecute_cmd_.array_angle();
    publish("ARRAY_TRACK_ANGLE",sta.str());
  
  
    std::stringstream stc;
    stc  << "tgt_id=" << last_prosecute_cmd_.target_number()
	 << ",tgt_x=" << last_target_x_
	 << ",tgt_y=" << last_target_y_
	 << ",tgt_dep=" << last_prosecute_cmd_.target_depth()
	 << ",tgt_hdg=" << last_prosecute_cmd_.target_heading()
	 << ",tgt_spd=" << last_prosecute_cmd_.target_speed()
	 << ",tgt_utc=" << last_prosecute_cmd_._time();
    publish("TARGET_CUE",stc.str());

    // Publish note report for target cue
    double cue_lat, cue_lon;
    m_Geodesy.UTM2LatLong(last_target_x_,
			  last_target_y_,cue_lat,cue_lon);
    std::stringstream scue;
    scue  << "TGT_" << last_prosecute_cmd_.target_number();
    NodeRecord node_report(scue.str(), "track");
    node_report.setTimeStamp(last_prosecute_cmd_._time());
    node_report.setX(last_target_x_);
    node_report.setY(last_target_y_);
    node_report.setLat(cue_lat);
    node_report.setLon(cue_lon);
    node_report.setDepth(last_prosecute_cmd_.target_depth());
    node_report.setSpeed(last_prosecute_cmd_.target_speed());
    node_report.setHeading(last_prosecute_cmd_.target_heading());
    publish("NODE_REPORT", node_report.getSpec());
  
    // Prosecute parameters
    publish("TARGET_NUMBER",last_prosecute_cmd_.target_number());
    publish("PROSECUTE_DURATION",last_prosecute_cmd_.prosecute_duration());
  
  }

  void LamssMissionManager::handle_status(const CMOOSMsg& msg)
  {
#ifndef GEO_COMMAND
    LAMSS_STATUS status_msg;
#else
    LAMSS_STATUS_GEO status_msg;
#endif
    parse_for_moos(msg.GetString(), &status_msg);
    // If coming from topside count as being connected, 
    // except for ICEXTracker, where only nav updates and commands count
#if 1
    if (!got_first_tracking_msg && status_msg.node() == cfg_.topside_id())
      last_acomms_ = MOOSTime();
#endif
  }

  void LamssMissionManager::handle_helm_mode(const CMOOSMsg& msg)
  {
    helm_mode_ = msg.GetString();
    transit_state_ =
      MOOSStrCmp(msg.GetString(),"MISSION:DIVING:DEPLOY:TRANSIT");
  }

  void LamssMissionManager::handle_gps_update(const CMOOSMsg& msg)
  {
    glog.is(VERBOSE) && glog << "Received GPS update: " << msg.GetString()
			     << std::endl;
    // reset deploy duration when getting GPS update
    //	last_acomms_ = MOOSTime();
    if (!(MOOSStrCmp(msg.GetString(),"Null")))
      last_gps_ = MOOSTime();
  }

  void LamssMissionManager::handle_lat_origin(const CMOOSMsg& msg)
  {
    double new_lat = msg.GetDouble();
    if (!isnan(new_lat) && fabs(new_lat - lat_origin_) > 1e-6)
      {
	lat_origin_ = new_lat;
	new_origin_ = true;
      }
  }

  void LamssMissionManager::handle_lon_origin(const CMOOSMsg& msg)
  {
    double new_lon = msg.GetDouble();
    if (!isnan(new_lon) && fabs(new_lon - lon_origin_) > 1e-6)
      {
	lon_origin_ = new_lon;
	new_origin_ = true;
      }
  }

  void LamssMissionManager::handle_vehicle_id(const CMOOSMsg& msg)
  {
    boost::mutex::scoped_lock lock(moos_mutex_);
    vehicle_id_ = msg.GetDouble();
    platform_info.set_modem_id(vehicle_id_);
    ownship_modem_ids_.clear();
    ownship_modem_ids_.insert(vehicle_id_);
    for(auto id : cfg_.additional_modem_id())
      ownship_modem_ids_.insert(id);
    for (auto id : ownship_modem_ids_)
      {
	glog.is(VERBOSE) && glog << "Ownship modem ID: " << id << std::endl;
      }
    // get name from modemidtable
    goby::moos::ModemIdConvert converter;
    converter.read_lookup_file("../../cruise/current/data/modemidlookup.txt");
    platform_info.set_name(converter.get_name_from_id(vehicle_id_));
    //   publish_pb("PLATFORM_INFO",platform_info);
  }

  void LamssMissionManager::handle_collaborator_id(const CMOOSMsg& msg)
  {
    collaborator_id_ = msg.GetDouble();
  }

  void LamssMissionManager::handle_collaborator_name(const CMOOSMsg& msg)
  {
    collaborator_name = msg.GetString();
  }

  void LamssMissionManager::handle_vehicle_name(const CMOOSMsg& msg)
  {
    boost::mutex::scoped_lock lock(moos_mutex_);
    vehicle_name_ = msg.GetString();
    platform_info.set_name(vehicle_name_);
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

  void LamssMissionManager::handle_homing_lock(const CMOOSMsg& msg)
  {
    std::string str = msg.GetString();
    homing_lock = MOOSStrCmp(str,"TRUE");
  }

  void LamssMissionManager::handle_opbox_distance(const CMOOSMsg& msg)
  {
    double opreg_perimeter_distance = msg.GetDouble();
    if (!initial_deploy_done_) return;
    boost::lock_guard<boost::mutex> lock(moos_mutex_);
    //  if (prosecute_mode_active_ && opreg_perimeter_distance <  cfg_.min_opbox_distance() )
    if (opreg_perimeter_distance <  cfg_.min_opbox_distance() )
      {
	send_deploy();
	publish("SURVEY_COMPLETED","TRUE");
	goby::glog.is(VERBOSE) && glog << group("helm")
				       << "DEPLOY Executed: Too close to Opregion Perimeter"
				       << std::endl;
      }
  }

  void LamssMissionManager::handle_transit_route(const CMOOSMsg& msg)
  {
#ifndef GEO_COMMAND
    LAMSS_TRANSIT transit_cmd;
#else
    LAMSS_TRANSIT_GEO transit_cmd;
#endif
    parse_for_moos(msg.GetString(), &transit_cmd);
    // Only handle messages for me
    if (transit_cmd._rcv_id() == vehicle_id_)
      {
	configure_transit_route(transit_cmd);
	post_transit_route();
#ifndef GEO_COMMAND
	if (last_transit_route_.transit_mode() == LAMSS_TRANSIT::TRANSIT_MODE_ROUTE)
#else
	  if (last_transit_route_.transit_mode() == LAMSS_TRANSIT_GEO::TRANSIT_MODE_ROUTE)
#endif    
	    publish("TRANSIT_MODE","ROUTE");
	  else
	    publish("TRANSIT_MODE","DIRECT");
    
      }
  }

  void LamssMissionManager::handle_transit_flag(const CMOOSMsg& msg)
  {
    if (MOOSStrCmp(toupper(msg.GetString()),"TRUE"))
      {
	// Clear transit route
	transit_x.clear();
	transit_y.clear();
	transit_lat.clear();
	transit_lon.clear();
	publish("TRANSIT_MODE","DIRECT");
	glog.is(DEBUG1) && glog << "Transit completed " << std::endl;
      }
  }

#ifndef GEO_COMMAND
  void LamssMissionManager::configure_transit_route(LAMSS_TRANSIT transit_cmd)
#else
    void LamssMissionManager::configure_transit_route(LAMSS_TRANSIT_GEO transit_cmd)
#endif  
  {
    last_transit_route_ = transit_cmd;
    int n_wpt = transit_cmd.wpt_size();
    transit_x.resize(n_wpt);
    transit_y.resize(n_wpt);
    transit_lat.resize(n_wpt);
    transit_lon.resize(n_wpt);
    for (int i=0; i<n_wpt; i++)
      {
#ifndef GEO_COMMAND
	transit_x[i] = transit_cmd.wpt(i).x();
	transit_y[i] = transit_cmd.wpt(i).y();
	double lat,lon;
	m_Geodesy.UTM2LatLong(transit_x[i],transit_y[i],lat,lon);
	transit_lat[i] = lat;
	transit_lon[i] = lon;
#else
	transit_lat[i] = transit_cmd.wpt(i).lat();
	transit_lon[i] = transit_cmd.wpt(i).lon();
	double x,y;
	m_Geodesy.LatLong2LocalUTM(transit_lat[i],transit_lon[i],y,x);
	transit_x[i] = x;
	transit_y[i] = y;
#endif      
      }
    transit_wpt_index = 0;
  }

  void LamssMissionManager::post_transit_route()
  {
#ifndef GEO_COMMAND
    if (last_transit_route_.transit_mode() == LAMSS_TRANSIT::TRANSIT_MODE_ROUTE)
#else
      if (last_transit_route_.transit_mode() == LAMSS_TRANSIT_GEO::TRANSIT_MODE_ROUTE)
#endif    
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
		//			<< "#capture_line=true"
			    << "#points=" ;
	      //			<< last_nav_.local_fix().x() << comma
	      //			<< last_nav_.local_fix().y() << colon ;
#ifndef GEO_COMMAND
	      if (last_deploy_cmd_.deploy_mode() ==
		  LAMSS_DEPLOY::DEPLOY_MODE_RETURN)
#else
		if (last_deploy_cmd_.deploy_mode() ==
		    LAMSS_DEPLOY_GEO::DEPLOY_MODE_RETURN)
#endif	    
		  {
		    // If RETURN mode publish route in reverse order
		    for (int i=n-1; i>=0; i--)
		      {
			transit_route << transit_x[i] << comma << transit_y[i] ;
			if (i>0)
			  transit_route << colon ;
		      }
		  }
		else
		  {
		    for (int i=0; i<n; i++)
		      {
			transit_route << transit_x[i] << comma << transit_y[i] ;
			if (i < n-1)
			  transit_route << colon ;
		      }
		  }
	      //	  transit_route << last_deploy_x_ << comma
	      //			<< last_deploy_y_ ;

	      std::stringstream td;
	      td << "depth=" << last_transit_route_.transit_depth();
	      publish("TRANSIT_DEPTH", td.str());
	      publish("TRANSIT_ROUTE",transit_route.str());
	      //publish("TRANSIT_MODE", "ROUTE");
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
	// abort_duration_ += 0.5*(current_time_ - last_homing_prosecute_);
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

  void LamssMissionManager::handle_icex20_tracking_message(const CMOOSMsg& msg)
  {
    lamss::icex::protobuf::TrackingResult tracking_result;
    parse_for_moos(msg.GetString(), &tracking_result);
    // Only handle messages for me
    if (ownship_modem_ids_.count(tracking_result.dest()))
      {
	double delta_x = tracking_result.vehicle_pos().dx();
	double delta_y = tracking_result.vehicle_pos().dy();;
	double camp_x = tracking_result.hydrohole_pos().x();
	double camp_y = tracking_result.hydrohole_pos().y();
	double camp_lat;
	double camp_lon;
	bool valid_track = !isnan(delta_x) && !isnan(delta_y);
      
	if (m_Geodesy.UTM2LatLong(camp_x,camp_y,camp_lat,camp_lon))
	  {
	    // save initial camp location
	    if (!got_first_tracking_msg)
	      {
		camp_x_init = camp_x;
		camp_y_init = camp_y;
		m_Geodesy.UTM2LatLong(camp_x_init,camp_y_init,
				      camp_lat_init,camp_lon_init);
		//	      got_first_tracking_msg = true;
		//              last_tracking_msg = tracking_result.vehicle_tx_time();
		camp_x_spd_filt.configAvg(filter_length);
		camp_y_spd_filt.configAvg(filter_length);
	      }
	    else
	      {
		// Update camp speed filters
		camp_x_spd_filt.setVal((camp_x-camp_x_last)/
				       (tracking_result.vehicle_tx_time()-last_tracking_msg));
		camp_y_spd_filt.setVal((camp_y-camp_y_last)/
				       (tracking_result.vehicle_tx_time()-last_tracking_msg));
	      }
	    // save camp location for possible deploy zambony being relative to deploy location
	    camp_x_last = camp_x;
	    camp_y_last = camp_y;
	    m_Geodesy.UTM2LatLong(camp_x_last,camp_y_last,
				  camp_lat_last,camp_lon_last);
	  
	    // Use current vehicle time stamp
	    //	  last_tracking_msg = tracking_result.vehicle_tx_time();
	  
	    last_acomms_ = MOOSTime();
	    if (valid_track)
	      {
		// reset deploy duration counter if we got a valid track
		last_gps_ = MOOSTime();
		glog.is(VERBOSE) && glog << "Received Tracking Update:"
					 << " delta_x=" <<  delta_x 
					 << " delta_y=" << delta_y 
					 << " camp_x=" << camp_x
					 << " camp_y=" << camp_y
					 << std::endl;
	      }
	    else
	      glog.is(VERBOSE) && glog << "Invalid track, Camp Update:"
				       << " camp_x=" << camp_x
				       << " camp_y=" << camp_y
				       << std::endl;
	    
	    NodeRecord node_report(cfg_.topside_name(), "hydrohole");
	    node_report.setTimeStamp(tracking_result.vehicle_tx_time());
	    node_report.setX(camp_x);
	    node_report.setY(camp_y);
	    node_report.setLat(camp_lat);
	    node_report.setLon(camp_lon);
	    node_report.setDepth(0.0);
	    node_report.setSpeed(tracking_result.hydrohole_drift().speed());
	    node_report.setHeading(tracking_result.hydrohole_drift().heading());
	    //publish("NODE_REPORT", node_report.getSpec());

	    // Save depth for comms when adapt fails
	    std::stringstream ss ;
	    ss << "depth=" << last_nav_.global_fix().depth() ;
	    publish("ACOMMS_DEPTH",ss.str());	  

	    // Modem buoys and stovepipes

	    if (!got_first_tracking_msg)
	      {
		h_x_spd_filt.resize(tracking_result.modem_pos_size());
		h_y_spd_filt.resize(tracking_result.modem_pos_size());
		h_x_last.resize(tracking_result.modem_pos_size());
		h_y_last.resize(tracking_result.modem_pos_size());
		for (int i=0; i<tracking_result.modem_pos_size(); i++)
		  {
		    h_x_spd_filt[i].configAvg(filter_length);  
		    h_y_spd_filt[i].configAvg(filter_length);  
		  }
	      }

	    for (int i=0; i<tracking_result.modem_pos_size(); i++)
	      {
		double h_x = tracking_result.hydrohole_pos().x() + tracking_result.modem_pos(i).dx();
		double h_y = tracking_result.hydrohole_pos().y() + tracking_result.modem_pos(i).dy();
		double h_z = tracking_result.modem_pos(i).z();
		double h_lat;
		double h_lon;
		m_Geodesy.UTM2LatLong(h_x,h_y,h_lat,h_lon);
		std::stringstream mm;
		mm << "h" << i+1;
		NodeRecord node_report(mm.str(),"buoy");
		node_report.setTimeStamp(tracking_result.vehicle_tx_time());
		node_report.setX(h_x);
		node_report.setY(h_y);
		node_report.setLat(h_lat);
		node_report.setLon(h_lon);
		if(cfg_.beacon_depth_size() > i)
		  {
		    switch(tracking_result.rx_layer())
		      {
		      case lamss::icex::protobuf::RECEIVE_SHALLOW:
			node_report.setDepth(cfg_.beacon_depth(i).receive_shallow());
			break;
                      
		      case lamss::icex::protobuf::RECEIVE_DEEP:
		      case lamss::icex::protobuf::RECEIVE_ALL_ARRAY:
			node_report.setDepth(cfg_.beacon_depth(i).receive_deep());
			break;
		      
		      case lamss::icex::protobuf::RECEIVE_MAIN_CHANNEL:
			node_report.setDepth(cfg_.beacon_depth(i).transmit());
			break;
		      }
		  }
		else
		  {
		    node_report.setDepth(-h_z);
		  }
		if (got_first_tracking_msg)
		  {
		    // Update modem mooring speeds
		    h_x_spd_filt[i].setVal((h_x-h_x_last[i])/
					   (tracking_result.vehicle_tx_time()-
					    last_tracking_msg));
		    h_y_spd_filt[i].setVal((h_y-h_y_last[i])/
					   (tracking_result.vehicle_tx_time()-
					    last_tracking_msg));
		  }
		h_x_last[i] = h_x;
		h_y_last[i] = h_y;
		double spd = sqrt(pow(h_x_spd_filt[i].getAvg(),2) + pow(h_y_spd_filt[i].getAvg(),2) );
		double hdg = 180/M_PI * atan2(h_x_spd_filt[i].getAvg(),h_y_spd_filt[i].getAvg());
		if (hdg < 0)
		  hdg += 360;
		node_report.setSpeed(spd);
		node_report.setHeading(hdg);
		//node_report.setSpeed(tracking_result.hydrohole_drift().speed());
		//node_report.setHeading(tracking_result.hydrohole_drift().heading());
		publish("NODE_REPORT", node_report.getSpec());
	      }
	    got_first_tracking_msg = true;
	    last_tracking_msg = tracking_result.vehicle_tx_time();
	  }
	else
	  {
	    glog.is(VERBOSE) && glog << "Invalid Tracking Update:" << std::endl;
	  }
      }
  }

  void LamssMissionManager::handle_survey_wpt_index(const CMOOSMsg& msg)
  {
    int wpt_index = msg.GetDouble();
    if (wpt_index != survey_wpt_index)
      {
	survey_wpt_index = wpt_index;
	new_wpt_index = true;
      }
  }

  void LamssMissionManager::handle_transit_wpt_index(const CMOOSMsg& msg)
  {
    int wpt_index = msg.GetDouble();
    if (wpt_index != transit_wpt_index &&  wpt_index > 0 )
      { 
	new_transit_wpt_index = true;
	// pop last route waypoint in case we have to 
#ifndef GEO_COMMAND
	if (transit_state_ && transit_x.size() > 0)
	  if (last_deploy_cmd_.deploy_mode() ==
	      LAMSS_DEPLOY::DEPLOY_MODE_RETURN)
#else
	    if (transit_state_ && transit_x.size() > 0)
	      if (last_deploy_cmd_.deploy_mode() ==
		  LAMSS_DEPLOY_GEO::DEPLOY_MODE_RETURN)
#endif
		{
		  transit_x.pop_back();
		  transit_y.pop_back();
		  transit_lat.pop_back();
		  transit_lon.pop_back();
		}
	      else
		{
		  transit_x.erase(transit_x.begin());
		  transit_y.erase(transit_y.begin());
		  transit_lat.erase(transit_lat.begin());
		  transit_lon.erase(transit_lon.begin());
		}
	glog.is(DEBUG1) && glog << "Reached waypoint " << wpt_index
				<< " Pruning. Remaining waypoints: "
				<< transit_x.size() << std::endl;
	// Note that must post TRANSIT_ROUTE when returning from prosecute
	// post_transit_route();
      }
    transit_wpt_index = wpt_index;

  }

  std::string LamssMissionManager::NodeReport(std::string node_name,
					      std::string node_type,
					      double utc_time,
					      double node_x,
					      double node_y,
					      double node_spd,
					      double node_hdg,
					      double node_dep )
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

    output += "UTC_TIME=" + doubleToString(utc_time,1);
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

  void LamssMissionManager::handle_stovepipe_geo_message(const CMOOSMsg& msg)
  {
    LAMSS_STOVEPIPE_GEO stovepipe_geo_msg;
    parse_for_moos(msg.GetString(),&stovepipe_geo_msg);
    LAMSS_STOVEPIPE stovepipe_msg;
    stovepipe_msg.set_stovepipe_id(stovepipe_geo_msg.stovepipe_id());
    stovepipe_msg.set__time(stovepipe_geo_msg._time());
    stovepipe_msg.set_depth(stovepipe_geo_msg.depth());
    stovepipe_msg.set_heading(stovepipe_geo_msg.heading());
    stovepipe_msg.set_major_axis(stovepipe_geo_msg.major_axis());
    stovepipe_msg.set_minor_axis(stovepipe_geo_msg.minor_axis());
    stovepipe_msg.set_verteces(stovepipe_geo_msg.verteces());
    double x,y;
    m_Geodesy.LatLong2LocalUTM(stovepipe_geo_msg.stovepipe_latitude(),
			       stovepipe_geo_msg.stovepipe_longitude(),
			       y,x);
    stovepipe_msg.set_stovepipe_x(x);
    stovepipe_msg.set_stovepipe_y(y);
    update_stovepipes(stovepipe_msg);
  }

  void LamssMissionManager::handle_stovepipe_message(const CMOOSMsg& msg)
  {
    LAMSS_STOVEPIPE stovepipe_msg;

    parse_for_moos(msg.GetString(),&stovepipe_msg);
    glog.is(DEBUG1) && glog << "Got Stovepipe message, Stovepipe_ID " 
			    << stovepipe_msg.stovepipe_id() << std::endl;
    update_stovepipes(stovepipe_msg);
    // needs to be done twice to get right name
    // update_stovepipes(stovepipe_msg);
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
	cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.mode,0");
	break;

      case lamss::icex::protobuf::TrackingSignalUpdate::EVEN_PLATFORM:
	cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.carrier_Hz,10500");
	cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.period," + goby::util::as<std::string>(msg.ping_period()));
	cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.reference_time," + goby::util::as<std::string>(even_reference_time));
	cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.duration_msx10,100");
	cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.bandwidth_Hz,3000");
	cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.mode,3");
	break;

      case lamss::icex::protobuf::TrackingSignalUpdate::ODD_PLATFORM:
	cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.carrier_Hz,10500");
	cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.period," + goby::util::as<std::string>(msg.ping_period()));
	cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.reference_time," + goby::util::as<std::string>(odd_reference_time));
	cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.duration_msx10,100");
	cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.bandwidth_Hz,3000");
	cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.mode,3");
	break;

      case lamss::icex::protobuf::TrackingSignalUpdate::UNIT:
	cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.carrier_Hz,13500");
	cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.period,1");
	cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.reference_time," + goby::util::as<std::string>(even_reference_time));
	cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.duration_msx10,35");
	cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.mode,4");
	break;

        // case lamss::icex::protobuf::TrackingSignalUpdate::TOPSIDE_HOMING:
        //     cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.carrier_Hz,8250");
        //     cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.period,1");
        //     cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.reference_time," + goby::util::as<std::string>(even_reference_time));
        //     cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.duration_msx10,200");
        //     cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.bandwidth_Hz,1500");
        //     cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("nav.trackping.mode,3");
        //     break;
      }

    // if(msg.has_tx_power_level())
    // {
    //     int tx_level = msg.tx_power_level();
    //     cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("pwramp.txlevel," + goby::util::as<std::string>(tx_level));
    // }

    // if(msg.has_mod_header_version())
    // {
    //     int hdr_version = msg.mod_header_version();
    //     cfg.MutableExtension(goby::acomms::micromodem::protobuf::config)->add_nvram_cfg("psk.packet.mod_hdr_version," + goby::util::as<std::string>(hdr_version));
    // }

    publish_pb("ACOMMS_DRIVER_CONFIG_UPDATE", cfg);

  }

  void LamssMissionManager::update_deploy_parameters()
  {

    double now = MOOSTime();
    double camp_x = camp_x_last + camp_x_spd_filt.getAvg()*
      (now-last_tracking_msg);
    double camp_y = camp_y_last + camp_y_spd_filt.getAvg()*
      (now-last_tracking_msg);
    {
      if(now > last_opregion_update_time_ + cfg_.opregion_update_period_sec())
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
          last_opregion_update_time_ = now;
	}
      
      //publish("HOMING_CONFIG",cent.str());
      // Consistent station keeping point
      std::stringstream statn;
      statn << "point="
	    << last_deploy_x_ + camp_x-camp_x_deploy
	    << "," << last_deploy_y_ + camp_y-camp_y_deploy;
      publish("DEPLOY_STATION",statn.str());
      // Move racetrack unless on trackline
      if (!in_track)
	{
	  std::stringstream trck;
	  trck << "points="
	       << last_deploy_x_ + camp_x-camp_x_deploy
	       << ","
	       << last_deploy_y_ + camp_y-camp_y_deploy;
	  publish("SENSOR_RACETRACK",trck.str());
	
	}

#ifndef GEO_COMMAND
      if (last_deploy_cmd_.deploy_mode() ==
	  LAMSS_DEPLOY::DEPLOY_MODE_LOITER )
#else
	if (last_deploy_cmd_.deploy_mode() ==
	    LAMSS_DEPLOY_GEO::DEPLOY_MODE_LOITER )
#endif
	  {
	    // Zamboni Loiters for ice camp ops
	    std::stringstream cent;
	    cent << "center_assign="
		 << last_deploy_x_ + camp_x-camp_x_deploy
		 << "," << last_deploy_y_ + camp_y-camp_y_deploy;
	    publish("SENSOR_DEPLOY",cent.str());
	  }
    
      if (new_wpt_index)
	{
#ifndef GEO_COMMAND
	  if (last_deploy_cmd_.deploy_mode() ==
	      LAMSS_DEPLOY::DEPLOY_MODE_LAWNMOWER )
#else
	    if (last_deploy_cmd_.deploy_mode() ==
		LAMSS_DEPLOY_GEO::DEPLOY_MODE_LAWNMOWER )
#endif	  
	      {
		//
		// Save shifted lawnmower config for publication at next waypoint
		// capture
		//     format: "points=format=lawnmower,x=%10%,y=%11%,height=%19%,
		//     width=%20%,lane_width=%21%,rows=east-west,degs=%18%#speed=%12%"
		std::stringstream clwnm;
		clwnm <<  "points=format=lawnmower,x="
		      << last_deploy_x_ + camp_x-camp_x_deploy
		      << ",y="
		      << last_deploy_y_ + camp_y-camp_y_deploy
		      << ",height=" << last_deploy_cmd_.survey_length()
		      << ",width=" << last_deploy_cmd_.survey_width()
		      << ",lane_width=" << last_deploy_cmd_.survey_period()
		      << ",rows=east-west,degs=" << last_deploy_cmd_.survey_heading()
		      << "#currix=" << survey_wpt_index;
		//		  << "#capture_line=true";
		std::string survey_config = clwnm.str();
		publish("SURVEY_CONFIG",survey_config);
	      }
#ifndef GEO_COMMAND
	    else if (last_deploy_cmd_.deploy_mode() ==
		     LAMSS_DEPLOY::DEPLOY_MODE_ZIGZAG )
#else
	    else if (last_deploy_cmd_.deploy_mode() ==
		     LAMSS_DEPLOY_GEO::DEPLOY_MODE_ZIGZAG )
#endif	  
	      {
		//
		// same for zigzag
		//
		std::stringstream czz;
		czz <<  "points=zigzag:"
		    << last_deploy_x_ + camp_x-camp_x_deploy
		    << "," << last_deploy_y_ + camp_y-camp_y_deploy
		    << "," << last_deploy_cmd_.survey_heading()
		    << "," << last_deploy_cmd_.survey_length()
		    << "," << last_deploy_cmd_.survey_period()
		    << "," << last_deploy_cmd_.survey_width()
		    << "#currix=" << survey_wpt_index;
		//		<< "#capture_line=true" ;
		std::string zigzag_status = czz.str();
		//	    publish("SURVEY_CONFIG",zigzag_status);
		publish("ZIGZAG_STATUS",zigzag_status);
	      }
	
	  new_wpt_index = false;
	}
      //
      // Update stovepipes and publish
      //
      for (int i=0; i< Stovepipes.size(); i++)
	{
	  publish_stovepipe(i,camp_x,camp_y);
	}
    }
    // update abort location. Note that it's defined as offset relative to camp
    abort_x = camp_x + abort_x_offset;
    abort_y = camp_y + abort_y_offset;
    m_Geodesy.UTM2LatLong(abort_x,abort_y,abort_lat,abort_lon);

    if (!homing_lock)
      {
	// update homing beacon location (assumed to be abort location
	std::stringstream bc;
	bc << "beacon_x=" << abort_x << ",beacon_y=" << abort_y ;
	publish("BEACON_UPDATE",bc.str());
	publish("HOMING_X",abort_x);
	publish("HOMING_Y",abort_x);

      }
  }

  void LamssMissionManager::publish_stovepipe(int i, double camp_x, double camp_y)
  {
    double degs = 90.0-Stovepipes[i].heading;
    if (degs > 360)
      degs -= 360;
    else if (degs < 0)
      degs += 360;
    std::string name;
    if (Stovepipes[i].spawned)
      name = "avd_obstacle_stvp_" + intToString(Stovepipes[i].ID);
    else
      {
	name = "_stvp_" + intToString(Stovepipes[i].ID);
	Stovepipes[i].spawned = true;
      }
    std::stringstream stvp;
    stvp << "name=" << name
	 << "#polygon= format=ellipse ,label=stvp_"
	 << Stovepipes[i].ID 
	 << ", x=" << Stovepipes[i].x+camp_x-Stovepipes[i].camp_x
	 << ", y=" << Stovepipes[i].y+camp_y-Stovepipes[i].camp_y
	 << ", degs="  << degs
	 << ", pts="  << Stovepipes[i].verteces
	 << ", major="  << Stovepipes[i].major_axis
	 << ", minor="  << Stovepipes[i].minor_axis;
    publish("OBSTACLE_ALERT",stvp.str());
  
  }

  //-----------------------------------------------------------------
  // Procedure: update_stovepipe_list
  //   Purpose: Maintain a list of known stovepipes, and the
  //            times they were last written to. In the dynamic scenario, they are updated
  //            when camp updates are received 

  void LamssMissionManager::update_stovepipes(LAMSS_STOVEPIPE stovepipe_msg)
  {

    CStovepipe Empty;
  
    // Current camp location
    double camp_x = camp_x_last + camp_x_spd_filt.getAvg()*
      (stovepipe_msg._time() - last_tracking_msg);
    double camp_y = camp_y_last + camp_y_spd_filt.getAvg()*
      (stovepipe_msg._time() - last_tracking_msg);
    for(int i=0; i<Stovepipes.size(); i++) 
      {
	// if negative ID kill it
	if (Stovepipes[i].ID == -stovepipe_msg.stovepipe_id())
	  {
	    for (int j=i; j<Stovepipes.size()-1; j++)
	      Stovepipes[j] = Stovepipes[j+1];
	    Stovepipes.pop_back();
	    return;
	  }
        else if (Stovepipes[i].ID == stovepipe_msg.stovepipe_id()) 
	  {
	    // If it exists, Update values
	    Stovepipes[i].utc = stovepipe_msg._time();
	    Stovepipes[i].x = stovepipe_msg.stovepipe_x();
	    Stovepipes[i].y = stovepipe_msg.stovepipe_y();
	    m_Geodesy.UTM2LatLong(Stovepipes[i].x,Stovepipes[i].y,
				  Stovepipes[i].lat,Stovepipes[i].lon);

	    Stovepipes[i].camp_x = camp_x;
	    Stovepipes[i].camp_y = camp_y;
	    Stovepipes[i].depth = stovepipe_msg.depth();
	    Stovepipes[i].heading = stovepipe_msg.heading();
	    Stovepipes[i].major_axis = stovepipe_msg.major_axis();
	    Stovepipes[i].minor_axis = stovepipe_msg.minor_axis();
	    Stovepipes[i].verteces = stovepipe_msg.verteces();
	    glog.is(VERBOSE) &&
	      glog << "Modified Stovepipe ID " << Stovepipes[i].ID
		   << " Stovepipe_x = " << Stovepipes[i].x
		   << " Stovepipe_y = " << Stovepipes[i].y
		   << " heading = " << Stovepipes[i].heading
		   << " major_axis = " << Stovepipes[i].major_axis
		   << " minor_axis = " << Stovepipes[i].minor_axis
		   << " Verteces = " << Stovepipes[i].verteces
		   << " camp_x = " << camp_x
		   << " camp_y = " << camp_y
		   << std::endl;
	    publish_stovepipe(i,camp_x,camp_y);
	    return;
	  }
      }
    // Does not exist, add one
    int last = Stovepipes.size();
    Stovepipes.resize(last+1);
    Stovepipes[last].ID = stovepipe_msg.stovepipe_id();
    Stovepipes[last].utc = stovepipe_msg._time();
    Stovepipes[last].x = stovepipe_msg.stovepipe_x();
    Stovepipes[last].y = stovepipe_msg.stovepipe_y();
    m_Geodesy.UTM2LatLong(Stovepipes[last].x,Stovepipes[last].y,
			  Stovepipes[last].lat,Stovepipes[last].lon);

    Stovepipes[last].camp_x = camp_x;
    Stovepipes[last].camp_y = camp_y;
    Stovepipes[last].depth = stovepipe_msg.depth();
    Stovepipes[last].heading = stovepipe_msg.heading();
    Stovepipes[last].major_axis = stovepipe_msg.major_axis();
    Stovepipes[last].minor_axis = stovepipe_msg.minor_axis();
    Stovepipes[last].verteces = stovepipe_msg.verteces();
    Stovepipes[last].spawned = false;
    glog.is(VERBOSE) && glog << "New Stovepipe ID " << Stovepipes[last].ID
			     << " Stovepipe_x = " << Stovepipes[last].x
			     << " Stovepipe_y = " << Stovepipes[last].y
			     << " Heading = " << Stovepipes[last].heading
			     << " Major_Axis = " << Stovepipes[last].major_axis
			     << " Minor_axis = " << Stovepipes[last].minor_axis
			     << " Verteces = " << Stovepipes[last].verteces
			     << " camp_x = " << camp_x
			     << " camp_y = " << camp_y
			     << std::endl;
    publish_stovepipe(last,camp_x,camp_y);
    return;
  }


  // Receives VehicleTransmitSelection, checks against the current state (previous) and if different switches the modems by publishing ACOMMS_DRIVER_RESET
  void LamssMissionManager::handle_vehicle_transmit_selection(const lamss::icex::protobuf::VehicleTransmitSelection& msg)
  {
    glog.is(VERBOSE) && glog << "Received command to set modem to " << msg.ShortDebugString() << std::endl;    
    if(msg.modem() == vehicle_transmit_.modem())
      {
        glog.is(VERBOSE) && glog << "Modem already set, doing nothing" << std::endl;
      }
    else
      {
        glog.is(VERBOSE) && glog << "Publishing reset to have pAcommsHandler swap modems" << std::endl;
        publish("ACOMMS_DRIVER_RESET", "Reset");


        goby::acomms::protobuf::MACUpdate mac = cfg_.standard_mac();
        if(msg.modem() == lamss::icex::protobuf::VehicleTransmitSelection::TRANSMIT_25KHZ)
	  mac = cfg_.legacy_mac();

        mac.set_src(vehicle_id_);
        mac.set_dest(vehicle_id_);
        mac.set_update_type(goby::acomms::protobuf::MACUpdate::ASSIGN);
        publish_pb("ACOMMS_MAC_CYCLE_UPDATE", mac);
      }

    vehicle_transmit_ = msg;
  }

