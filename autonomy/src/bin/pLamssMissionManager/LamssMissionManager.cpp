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

using namespace lamss::dsop::fsm;
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
        lamss::dsop::fsm::cfg_.reset(new lamss::dsop::protobuf::LamssMissionManagerConfig);
        inst_ = new LamssMissionManager(*lamss::dsop::fsm::cfg_);
    }
    return inst_;
}

LamssMissionManager::LamssMissionManager(lamss::dsop::protobuf::LamssMissionManagerConfig& cfg)
    : GobyMOOSApp(&cfg),
      cfg_(cfg),
      scheduler_(true),
      fsm_(scheduler_.create_processor<LamssMissionManagerFSM>()),
      last_slow_loop_t_(0),
      sonar_mode_active_(false),
      // Henriks track management variables
      target_focus_(""),
      sonar_focus_depth_(0),
      valid_track_(false),
      valid_primary_track_(false),
      valid_secondary_track_(false),
      valid_primary_time_(0),
      sonar_xy_track_(""),
      sonar_ro_track_(""),
      sonar_geo_track_(""),
      sonar_range_only_track_(""),
      primary_track_key_(cfg_.primary_track()),
      active_primary_track_(""),
      got_primary_track_(false),
      secondary_track_key_(cfg_.secondary_track()),
      active_secondary_track_(""),
      got_secondary_track_(false),
      got_last_contact_(false),
      last_track_range_(0),
      last_track_rate_(0),
      last_track_bearing_(0),
      last_contact_utc_(0),
      last_contact_range_(0),
      last_contact_rate_(0),
      last_contact_bearing_(0),
      max_contact_range_(0),
      last_contact_x_(0),
      last_contact_y_(0),
      last_contact_heading_(0),
      last_contact_speed_(0),
      forecast_decay_(1200),
      doppler_threshold_lower_(cfg_.doppler_threshold_lower()),
      doppler_threshold_upper_(cfg_.doppler_threshold_upper()),
      transition_mode_(cfg_.transition_mode())
{
    goby::glog.set_lock_action(lock);
    goby::glog.add_group("fsm", goby::common::Colors::lt_magenta);
    goby::glog.add_group("helm", goby::common::Colors::lt_green);
    goby::glog.add_group("mac", goby::common::Colors::lt_blue);
    goby::glog.add_group("gram", goby::common::Colors::yellow);

    // set up subscriptions before starting separate thread
    subscribe("DSOP_SONARCONTROL_IN", &LamssMissionManager::handle_sonar_control, this);
    subscribe("VEHICLE_ID", &LamssMissionManager::handle_vehicle_id, this);
    // Track management
    subscribe("SONAR_PRIMARY_XY_TRACK", &LamssMissionManager::handle_sonar_track_report, this);
    subscribe("SONAR_PRIMARY_RO_TRACK", &LamssMissionManager::handle_sonar_track_report, this);
    subscribe("SONAR_GEO_TRACK", &LamssMissionManager::handle_sonar_track_report, this);
    subscribe("SONAR_RANGE_ONLY_TRACK", &LamssMissionManager::handle_sonar_track_report, this);
    subscribe("PRIMARY", &LamssMissionManager::handle_primary_track_switch, this);
    subscribe("SECONDARY", &LamssMissionManager::handle_secondary_track_switch, this);

    // intialize MOOSGeodesy

    m_Geodesy.Initialise(cfg_.common().lat_origin(),cfg_.common().lon_origin());

    // for mooring, set last_nav_

    if (cfg_.is_mooring())
      {
	last_nav_.mutable_local_fix()->set_x(cfg_.deploy_x());    
	last_nav_.mutable_local_fix()->set_y(cfg_.deploy_y());    
	last_nav_.mutable_local_fix()->set_z(-cfg_.deploy_depth()); 
	
	double lat;
	double lon;
	m_Geodesy.UTM2LatLong(last_nav_.local_fix().x(),last_nav_.local_fix().y(),
			      lat,lon);
	
	last_nav_.mutable_global_fix()->set_lat(lat);    
	last_nav_.mutable_global_fix()->set_lon(lon);    
	last_nav_.mutable_global_fix()->set_depth(cfg_.deploy_depth());    
	
	last_nav_.mutable_pose()->set_heading(cfg_.deploy_heading());

      }

   scheduler_.initiate_processor(fsm_);  
  
    fsm_thread_.reset(new boost::thread(boost::bind(&boost::statechart::fifo_scheduler<>::operator(), &scheduler_, 0)));
  
    glog.is(VERBOSE) && glog << cfg_.DebugString() << std::endl;

}

void LamssMissionManager::loop()
{
    double curr_time = MOOSTime();

    try_publish(); // publish messages queued from state machine

    // process some events at a slower rate
    const int slow_loop_period = 10;
    if(curr_time > last_slow_loop_t_ + slow_loop_period)
        slow_loop(curr_time);

}

void LamssMissionManager::slow_loop(double curr_time)
{
  if(sonar_mode_active_)
    {
      // Validate any arriving tracks
      validateTracks();
      if (valid_track_ && got_last_contact_)
	{
	  if (transition_mode_)
	    {
	      glog.is(DEBUG1) && glog << "transition switching mode"  
				      << ", track range rate =" 
				      << last_track_rate_ << std::endl;
	      if (doppler_mode_active_)
		if (fabs(last_track_rate_) < doppler_threshold_upper_) 
		  {
		    glog.is(DEBUG1) && glog << "EvDopplerToTransition"  << std::endl;
		    queue_event<EvDopplerToTransition>();
		  }
		else
		  {
		    // stay where we are} 
		  }
	      else if (transition_mode_active_)
		if (fabs(last_track_rate_) >= doppler_threshold_upper_) 
		  { 
		    glog.is(DEBUG1) && glog << "EvTransitionToDoppler"  << std::endl;
		    queue_event<EvTransitionToDoppler>();
		  }
		else if (fabs(last_track_rate_) < doppler_threshold_lower_)
		  {
		    glog.is(DEBUG1) && glog << "EvTransitionToCPA"  << std::endl;
		    queue_event<EvTransitionToCPA>();
		  }
		else
		  {
		    // stay where we are} 
		  }
	      else if (cpa_mode_active_)
		if (fabs(last_track_rate_) >= doppler_threshold_lower_) 
		  {
		    glog.is(DEBUG1) && glog << "EvCPAToTransition"  << std::endl;
		    queue_event<EvCPAToTransition>();
		  }
		else
		  {
		    // stay where we are} 
		  }
	    }
	  else
	    {
	      glog.is(DEBUG1) && glog << "hystereses switching mode"  << std::endl;
	      if (doppler_mode_active_ && fabs(last_track_rate_) < doppler_threshold_lower_) 
		{	
		  glog.is(DEBUG1) && glog << "EvDopplerToCPA"  << std::endl;	
		  queue_event<EvDopplerToCPA>();
		}
	      else if (cpa_mode_active_ && fabs(last_track_rate_) >= doppler_threshold_upper_) 
		{
		  glog.is(DEBUG1) && glog << "EvCPAToDoppler"  << std::endl;
		  queue_event<EvCPAToDoppler>();
		}
	    }
	}
    }
  last_slow_loop_t_ = curr_time;
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

void LamssMissionManager::handle_sonar_control(const CMOOSMsg& msg)
{
    DSOP_SONARCONTROL sonar_control;
    parse_for_moos(msg.GetString(), &sonar_control);

    if(sonar_control.sonar_mode() != DSOP_SONARCONTROL::SONAR_MODE_RESET)
        last_sonar_control_ = sonar_control;
    
    update_sonar_mode(sonar_control);
    update_sonar_focus(sonar_control);

}

void LamssMissionManager::update_sonar_mode(DSOP_SONARCONTROL sonar_control)
{
  DSOP_SONARCONTROL::Sonar_ModeEnum mode = sonar_control.sonar_mode();
  bool auto_ping =  sonar_control.auto_ping_sequence();

  switch(mode) 
    {
    case DSOP_SONARCONTROL::SONAR_MODE_OFF:
    case DSOP_SONARCONTROL::SONAR_MODE_TRIM:
      sonar_mode_active_ = false;
      queue_event<EvAutoPingOff>();
      break;
      
    case DSOP_SONARCONTROL::SONAR_MODE_FIXED_FOCUS:
    case DSOP_SONARCONTROL::SONAR_MODE_ADAPTIVE_FOCUS:
    case DSOP_SONARCONTROL::SONAR_MODE_SWEEP:
      sonar_mode_active_ = true;
      if (auto_ping) 
	queue_event<EvAutoPingOn>();
      else
	queue_event<EvAutoPingOff>();
      break;
      
    case DSOP_SONARCONTROL::SONAR_MODE_RESET:
      queue_event<EvAutoPingOff>();
      // Reset track manager
	    resetTrackManager();
            sonar_mode_active_ = false;
            update_sonar_mode(last_sonar_control_);
            break;       
    }    
  
}

void LamssMissionManager::update_sonar_focus(DSOP_SONARCONTROL sonar_control)
{
  DSOP_SONARCONTROL::Adaptive_FocusEnum focus = sonar_control.adaptive_focus();
  switch(focus)
    {
    case DSOP_SONARCONTROL::ADAPTIVE_FOCUS_ACTIVE_TRACK:
      target_focus_ = "active_track";
      return;      
    case DSOP_SONARCONTROL::ADAPTIVE_FOCUS_TARGET_CUE:
      target_focus_ = "target_cue";
      return;      
      
    case DSOP_SONARCONTROL::ADAPTIVE_FOCUS_TGT_1_SIM:
      target_focus_ = "tgt_1_sim";
      return;      
      
    case DSOP_SONARCONTROL::ADAPTIVE_FOCUS_MAX_RAP_RANGE:
    case DSOP_SONARCONTROL::ADAPTIVE_FOCUS_MIN_RAP_RANGE:
      target_focus_ = "";
      return;      
    }
  
}

void LamssMissionManager::handle_sonar_track_report(const CMOOSMsg& msg)
{
  
  // save for later use if switched
  std::string val = msg.GetString();
  std::string key = msg.GetKey();

  if (MOOSStrCmp(key,primary_track_key_)) 
    {
      m_Comms.Notify("SONAR_PRIMARY_TRACK", val );
      active_primary_track_ = val;
      got_primary_track_ = true;
    }
  if (MOOSStrCmp(key,secondary_track_key_)) 
    {
      m_Comms.Notify("SONAR_SECONDARY_TRACK", val);
      active_secondary_track_ = val;
      got_secondary_track_ = true;
    }
  if (MOOSStrCmp(key,"SONAR_PRIMARY_XY_TRACK"))
      sonar_xy_track_ = val;
  else if (MOOSStrCmp(key,"SONAR_PRIMARY_RO_TRACK"))
      sonar_ro_track_ = val;
  else if (MOOSStrCmp(key,"SONAR_GEO_TRACK"))
      sonar_geo_track_ = val;
  else if (MOOSStrCmp(key,"SONAR_RANGE_ONLY_TRACK"))
      sonar_range_only_track_ = val;
 
}

void LamssMissionManager::handle_primary_track_switch(const CMOOSMsg& msg)
{
  glog.is(DEBUG1) && glog  << "Entering handle_primary_track_switch" << std::endl;

  std::string primary_track_prev = primary_track_key_;

  handle_track_switch(msg, 
		      primary_track_key_,
		      active_primary_track_);
  glog.is(DEBUG1) && glog  << "New primary_track_key_ = " 
			   << primary_track_key_  << " Value= "
			   << active_primary_track_ << "\n"
			   << "Previous primary track key: "
			   << primary_track_prev << std::endl;

  if (!MOOSStrCmp(primary_track_key_,primary_track_prev))
    {
      m_Comms.Notify("SONAR_PRIMARY_TRACK", active_primary_track_);
      // Publish active primary track or alternatively secondary track
      valid_primary_track_ = createTrackNodeReport(active_primary_track_);
      if (valid_primary_track_)
	{
	  got_primary_track_ = true;
	  glog.is(DEBUG1) && glog  << "primary_track_key_ = " 
				   << primary_track_key_ << std::endl;
	  glog.is(DEBUG1) && glog  << "Republishing track: "  
				   << active_primary_track_ << std::endl;
	}
      else
	valid_secondary_track_ = createTrackNodeReport(active_secondary_track_);
      
    }
}

void LamssMissionManager::handle_secondary_track_switch(const CMOOSMsg& msg)
{
  std::string secondary_track_prev = secondary_track_key_;

  handle_track_switch(msg, 
		      secondary_track_key_,
		      active_secondary_track_);
  if (!MOOSStrCmp(secondary_track_key_,secondary_track_prev))
    {
      m_Comms.Notify("SONAR_SECONDARY_TRACK", active_secondary_track_);
      // Publish active primary track or alternatively secondary track
      valid_primary_track_ = createTrackNodeReport(active_primary_track_);
      if (!valid_primary_track_)
	valid_secondary_track_ = createTrackNodeReport(active_secondary_track_);

      if (valid_secondary_track_)
	{
	  got_secondary_track_ = true;
	  glog.is(DEBUG1) && glog << " secondary_track = " << secondary_track_key_ << std::endl;
	  glog.is(DEBUG1) && glog  << "Republishing track: "  
				   << active_secondary_track_ << std::endl;
	}
   }
}

bool LamssMissionManager::handle_track_switch(const CMOOSMsg& Msg, 
					   std::string & priority_track,
					   std::string & active_priority_track)
{
  bool valid_priority_track;

  active_priority_track = "";
  
  if (MOOSStrCmp(toupper(Msg.m_sVal),"GEO") || MOOSStrCmp(toupper(Msg.m_sVal),"PA_XY"))
    {
      priority_track = "SONAR_GEO_TRACK";
      active_priority_track = sonar_geo_track_;
    }
  else if (MOOSStrCmp(toupper(Msg.m_sVal),"RANGE") || MOOSStrCmp(toupper(Msg.m_sVal),"PA_RO"))
    {
      priority_track = "SONAR_RANGE_ONLY_TRACK";
      active_priority_track = sonar_range_only_track_;
    }
  else if ( MOOSStrCmp(toupper(Msg.m_sVal),"BB_RO") ||
	    MOOSStrCmp(toupper(Msg.m_sVal),"RO") )
    {
      priority_track = "SONAR_PRIMARY_RO_TRACK";
      active_priority_track = sonar_ro_track_;
    }
  else if ( MOOSStrCmp(toupper(Msg.m_sVal),"BB_XY") ||
	    MOOSStrCmp(toupper(Msg.m_sVal),"XY") )
    {
      priority_track = "SONAR_PRIMARY_XY_TRACK";
      active_priority_track = sonar_xy_track_;
    }
  
  return valid_priority_track;  
}

void LamssMissionManager::validateTracks()
{
  double ping_interval = last_sonar_control_.ping_interval();
  glog.is(DEBUG1) && glog << "Checking for valid tracks " << std::endl;
  if ( got_primary_track_ || got_secondary_track_)
    {
      if ( got_primary_track_ )
	{
	  if (createTrackNodeReport(active_primary_track_))
	    {
	      valid_primary_track_ = true;
	       glog.is(DEBUG1) && glog << " New active primary track: " 
				       << primary_track_key_ << "=" 
				       << active_primary_track_ << std::endl;  
	      valid_primary_time_ = MOOSTime();
	    }
	  else
	    {
	      valid_primary_track_ = false;
	    }
	}
      
      if (got_secondary_track_ &&  
	  (MOOSTime() - valid_primary_time_ > 0.5*ping_interval))
	{
	  // Accept secondary track only if previous is older than half a ping cycle
	  // to avoid later secondary track to override recent primary 
	  if ( createTrackNodeReport(active_secondary_track_))
	    {
	      valid_secondary_track_ = true;
	       glog.is(DEBUG1) && glog  << " New active secondary track: " 
					<< secondary_track_key_ << "=" 
					<< active_secondary_track_ << std::endl;
	    }
	  else 
	    {
	      valid_secondary_track_ = false;
	    }
	} 
      got_primary_track_ = false;
      got_secondary_track_ = false;
      
      valid_track_ = valid_primary_track_ || valid_secondary_track_;
      if (!valid_track_ && got_last_contact_)
	{
	  postStaleTrack();
	  glog.is(DEBUG1) && glog  << " No valid primary or secondary tracks " << std::endl;
	  glog.is(DEBUG1) && glog  << " Publishing stale track " << std::endl;
	  if (MOOSStrCmp(tolower(target_focus_),"active_track"))
	    m_Comms.Notify("PRIORITY_TRK", "contact=stale_track");
	  if (cpa_mode_active_)

	    {	
	      if (transition_mode_)
		{
		  glog.is(DEBUG1) && glog << "Switching to Transition Ping Sequence"  << std::endl;
		  queue_event<EvCPAToTransition>();
		}
	      else
		{
		  glog.is(DEBUG1) && glog << "Switching to Doppler Mode Ping Sequence"  << std::endl;
		  queue_event<EvCPAToDoppler>();
		}
	    }
	}
    }
}

void  LamssMissionManager::postStaleTrack()
{
  double contact_utc = MOOSTime();
  double delta_t = (contact_utc - last_contact_utc_);
  double decay = exp(-delta_t/forecast_decay_);
  double contact_speed = 0;
  if (delta_t <= forecast_decay_)
    contact_speed =decay*last_contact_speed_;
  double contact_x = last_contact_x_ + 
    contact_speed*delta_t*sin(last_contact_heading_*M_PI/180.0);
  double contact_y = last_contact_y_ + 
    contact_speed*delta_t*cos(last_contact_heading_*M_PI/180.0);
  PublishNodeReport("stale_track",
		    contact_utc,
		    contact_x,
		    contact_y,
		    last_contact_heading_,
		    sonar_focus_depth_,
		    contact_speed,
		    0.0);
  
}

bool  LamssMissionManager::createTrackNodeReport(std::string trk_report_msg)
{
  double track_utc;
  double track_x;
  double track_y;
  double track_heading;
  double track_depth;
  double track_speed;
  double track_confidence;
  int track_contacts;
  
  bool ok = MOOSValFromString(track_utc,trk_report_msg,"utc_time");
  ok = ok & MOOSValFromString(track_x,trk_report_msg,"trk_x");
  ok = ok & MOOSValFromString(track_y,trk_report_msg,"trk_y");
  if (!MOOSValFromString(track_depth,trk_report_msg,"trk_depth")) {
    track_depth = 10;
  }
  ok = ok & MOOSValFromString(track_speed,trk_report_msg,"trk_speed");
  ok = ok & MOOSValFromString(track_heading,trk_report_msg,"trk_heading");
  ok = ok & MOOSValFromString(track_confidence,trk_report_msg,"trk_confidence");

  ok = ok & (fabs(track_speed) < cfg_.max_speed());

  valid_track_ = ok;
  
  if (ok)
    {
      last_track_range_ = sqrt(pow(track_x - last_nav_.local_fix().x(),2) 
				     + pow(track_y - last_nav_.local_fix().y(),2));
      double dotx = track_speed*sin(track_heading*M_PI/180);
      double doty = track_speed*cos(track_heading*M_PI/180);
      last_track_rate_ = (dotx*(track_x - last_nav_.local_fix().x()) 
			  + doty*(track_y - last_nav_.local_fix().y()))/last_track_range_;
      last_track_bearing_ = atan2(track_x - last_nav_.local_fix().x(),
				  track_y - last_nav_.local_fix().y())*180/M_PI;
      if (last_track_bearing_ <= -180.0)
	last_track_bearing_ += 360.0;
      if (last_track_bearing_ > 180.0)
	last_track_bearing_ -= 360;
      double dVal=0;
      if (MOOSValFromString(dVal,trk_report_msg,"trk_num_contacts_assoc"))
	track_contacts = int(dVal);
      if (track_contacts > 0)
	{
	  // Mark as last valid contact track
	  last_contact_range_ = last_track_range_;
	  last_contact_rate_ = last_track_rate_;
	  last_contact_bearing_ = last_track_bearing_;
	  if (last_contact_range_ > max_contact_range_)
	    max_contact_range_ = last_contact_range_;
	  last_contact_utc_ = track_utc;
	  last_contact_x_ = track_x;
	  last_contact_y_ = track_y;
	  last_contact_heading_ = track_heading;
	  last_contact_speed_ = track_speed;
	  got_last_contact_ = true;
	  m_Comms.Notify("LAST_CONTACT_RANGE",last_contact_range_);
	  m_Comms.Notify("LAST_CONTACT_RATE",last_contact_rate_);
	}
      
      PublishNodeReport("active_track",
			track_utc,
			track_x,
			track_y,
			track_heading,
			track_depth,
			track_speed,
			track_confidence);
      glog.is(DEBUG1) && glog  << " Valid track received: " 
			       << trk_report_msg << "\n" 
			       << "             range=" << last_contact_range_
			       << ", rate=" << last_contact_rate_
			       << ", bearing=" << last_contact_bearing_
			       << std::endl;
    }
  else
    {
       glog.is(DEBUG1) && glog  << " Track not valid: " 
				<< trk_report_msg << std::endl;
       //      broadside_bearing = last_sonar_control_.focus_bearing();
    }
  return(ok);
}

bool LamssMissionManager::PublishNodeReport(std::string track_name,
					 double track_utc,
					 double track_x,
					 double track_y,
					 double track_hdg,
					 double track_dep,
					 double track_spd,
					 double track_conf)
{
  
   double track_lat;
   double track_lon;
   
   if (!m_Geodesy.UTM2LatLong(track_x,track_y,track_lat,track_lon))
     {
         glog.is(WARN) && glog << "Error in UTM2LatLong conversion " << std::endl;
         return(false);	  
     }
   
   std::string output="NAME=" + track_name +",TYPE=track,";
   
   output += "UTC_TIME=" + doubleToString(track_utc,1);
   output += ",";
   
   output += "X=" + doubleToString(track_x,2);
   output += ",";
   
   output += "Y=" + doubleToString(track_y,2);
   output += ",";
   
   output += "LAT=" + doubleToString(track_lat,6);
   output += ",";
   
   output += "LON=" + doubleToString(track_lon,6);
   output += ",";
   
   output += "SPD=" + doubleToString(track_spd,2);
   output += ",";
   
   output += "HDG=" + doubleToString(track_hdg,2);
   output += ",";
   
   output += "DEPTH=" + doubleToString(track_dep,2);
   output += ",";
   
   output += "CONFID=" + doubleToString(track_conf,2);
   
   m_Comms.Notify ("NODE_REPORT", output);
   
   return(true);
 }

void LamssMissionManager::resetTrackManager()
{
    valid_track_=false;
    valid_primary_track_=false;
    valid_secondary_track_=false;
    valid_primary_time_ = 0;

    got_last_contact_ = false;
    last_track_range_ = 0;
    last_track_rate_ = 0;
    last_track_bearing_ = 0;
    last_contact_range_ = 0;
    last_contact_rate_ = 0;
    last_contact_bearing_ = 0;
    max_contact_range_ = 0;
    last_contact_x_ = 0;
    last_contact_y_ = 0;
    last_contact_heading_ = 0;
    last_contact_speed_ = 0;

    //    range_sweep = false;
    //    counter_increment = 1;

    sonar_geo_track_ = "";
    sonar_range_only_track_ = "";
    sonar_xy_track_ = "";
    sonar_ro_track_ = "";

}

void LamssMissionManager::handle_vehicle_id(const CMOOSMsg& msg)
{
    boost::mutex::scoped_lock lock(moos_mutex_);
    vehicle_id_ = msg.GetDouble();
}

