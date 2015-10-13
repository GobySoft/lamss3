// Copyright 2015 
// Henrik Schmidt
// Massachusetts Institute of Technology 

#include <dlfcn.h>

#include <numeric>

#include <boost/thread.hpp>

#include "goby/common/logger.h"
#include "goby/acomms/connect.h"
#include "goby/common/time.h"

#include "LamssTrackManager.h"

#include "goby/moos/moos_protobuf_helpers.h"

using namespace lamss::shared::fsm;
using namespace goby::common::logger;
using namespace goby::common::logger_lock;
namespace gpb = goby::moos::protobuf;
using goby::common::goby_time;
using goby::glog;

const double pi = 3.14159;

LamssTrackManager* LamssTrackManager::inst_ = 0;
void* LamssTrackManager::driver_library_handle_= 0;

int main(int argc, char * argv[])
{
    return goby::moos::run<LamssTrackManager>(argc, argv);
}

LamssTrackManager* LamssTrackManager::get_instance()
{
    if(!inst_)    {
        lamss::shared::fsm::cfg_.reset(new LamssTrackManagerConfig);
        inst_ = new LamssTrackManager(*lamss::shared::fsm::cfg_);
    }
    return inst_;
}

LamssTrackManager::LamssTrackManager(LamssTrackManagerConfig& cfg)
    : GobyMOOSApp(&cfg),
      cfg_(cfg),
      scheduler_(true),
      fsm_(scheduler_.create_processor<LamssTrackManagerFSM>()),
      num_tracks(0),
      collab_num_tracks(0),
      filt_set(true),
      dclt_enable_(false),
      trk_beamno(0),
      track_delay(cfg_.track_delay()),
      prev_highest(-1),
      collaborator_id(-1),
      collaborator_name(""),
      close_range_(false),
      m_buffer_length(cfg_.buffer_length_single()),
      last_slow_loop_t_(0)
{

    goby::glog.set_lock_action(lock);
    goby::glog.add_group("fsm", goby::common::Colors::lt_magenta);
    goby::glog.add_group("helm", goby::common::Colors::lt_green);
    goby::glog.add_group("mac", goby::common::Colors::lt_blue);
    goby::glog.add_group("track", goby::common::Colors::yellow);

    set_ignore_stale(false);

    // set up subscriptions before starting separate thread
    subscribe("LAMSS_DEPLOY_IN", &LamssTrackManager::handle_deploy, this);
    subscribe("LAMSS_PROSECUTE_IN", &LamssTrackManager::handle_prosecute, this);
    subscribe("MISSION_STATE", &LamssTrackManager::handle_mission_state, this);
    subscribe("VEHICLE_ID", &LamssTrackManager::handle_vehicle_id, this);
    subscribe("IFS_DATA_IN", &LamssTrackManager::handle_ifs_data, this);    
    subscribe("COLLABORATOR_ID", 
	      &LamssTrackManager::handle_collaborator_id, this);    
    subscribe("COLLABORATOR_NAME", 
	      &LamssTrackManager::handle_collaborator_name, this);  
    subscribe("BEARING_TRACKS", 
	      &LamssTrackManager::handle_bearing_tracks, this);    
    subscribe("PROSECUTE_HEADING", 
	      &LamssTrackManager::handle_prosecute_heading, this);    
    subscribe("TRACK_STAT", 
	      &LamssTrackManager::handle_track_stat, this);    
    subscribe("BEARING_STAT", 
	      &LamssTrackManager::handle_bearing_stat, this);    
    subscribe("CLOSE_RANGE", 
	      &LamssTrackManager::handle_close_range, this);    
    subscribe("CLASS_DONE", 
	      &LamssTrackManager::handle_class_done, this);    

    heading_filter = new Mavg(cfg_.filter_length());
    // reset collaborator tracks
    collab_new_track.clear();
    collab_Tracks.clear();
  
    Empty.setTrack( 0, 0, 0, 0, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, class_features);
    
    // intialize MOOSGeodesy

    m_Geodesy.Initialise(cfg_.common().lat_origin(),cfg_.common().lon_origin());

    scheduler_.initiate_processor(fsm_);  
  
    fsm_thread_.reset(new boost::thread(boost::bind(&boost::statechart::fifo_scheduler<>::operator(), &scheduler_, 0)));
  
    glog.is(VERBOSE) && glog << cfg_.DebugString() << std::endl;

}

void LamssTrackManager::loop()
{
    double curr_time = MOOSTime();
    // set time for fsm
    current_time_ = curr_time;

    try_publish(); // publish messages queued from state machine

    bool prev_mode = dclt_enable_;
    dclt_enable_ = prosecute_mode_active_ || 
      (deploy_mode_active_ && sonar_on_);
    if (dclt_enable_)
      {
	if (dclt_idle_)
	  {
	    publish("TRACKER_COMMAND","ON");
	    queue_event<EvDcltStart>();
	  }
      }
    else
      {
	publish("TRACKER_COMMAND","OFF");
	if (dclt_active_)
	  queue_event<EvDcltOff>();
      }

  if (dclt_enable_)
    {
      heading = angle360(heading_filter->get_FVal());
      postStateVars();
    }
  //
  // update search mode
  //
  if ( prosecute_mode_active_ && detection_active_ )
    {
      //update time since contact was seen
      double elapsed = current_time_ - detection_start_;;
     
      //update target cue estimates
      double curr_cue_x = cue_x_+ elapsed*cue_speed_* sin(cue_heading_*M_PI/180.);
      double curr_cue_y = cue_y_+ elapsed*cue_speed_* cos(cue_heading_*M_PI/180.);
      double auv_x = last_nav_.local_fix().x();
      double auv_y = last_nav_.local_fix().y();
      //update target range
      double curr_cue_range = sqrt(pow(curr_cue_x-auv_x,2) + pow(curr_cue_y-auv_y,2));
      // Check whether target is reachable
      bool opportunity = checkOpportunity();   

      std::string node_report = trackNodeReport("target_cue",curr_cue_x, 
					      curr_cue_y, 
					      cue_speed_, 
					      cue_heading_, 
					      cue_depth_ );
      publish("NODE_REPORT",node_report);
      publish("MY_TARGET","contact=TARGET_CUE");

      glog.is(DEBUG2) && glog << "Current cue range =" << curr_cue_range
			      << ", Giveup_range =" << cfg_.max_search_range()
			      << std::endl;

      if (!cue_in_range_ && curr_cue_range <= cfg_.max_search_range())
	{
	  queue_event<EvCueInRange>();
	}

      if ( !opportunity )
	{
	  glog.is(DEBUG1) && glog << "Cannot reach target - Deploy Commanded" 
				  << std::endl;
	  send_deploy();
	}
      else if (cue_in_range_)
	{
	  if  (curr_cue_range > cfg_.max_search_range())
	    {
	      glog.is(DEBUG1) && glog << 
		"Target running out of range, giving up - Deploy Commanded" 
				      << std::endl;
	      send_deploy();
	    }
      // No detect yet and close to target. 
      //if((!cue_too_close_ && curr_cue_range <= cfg_.min_search_range()) )
	  if((curr_cue_range <= cfg_.min_search_range()) )
	    {
	      std::string buff = "polygon=radial:" + 
		doubleToString(auv_x,1) +
		"," + doubleToString(auv_y,1) + 
		"," + doubleToString(cfg_.orbit_radius(),1);
	      publish("PROSECUTE_LOITER",buff);
	      queue_event<EvSearchOrbit>();
	    }
	}
      //send info to CloseRange behavior
      std::string buff = "node=" + intToString(vehicle_id_) +
	",state=4" +
	",x=" + doubleToString(curr_cue_x,2) +
	",y=" + doubleToString(curr_cue_y,2) +
	",heading=" + doubleToString(cue_heading_,1) +
	",speed=" + doubleToString(cue_speed_,2) +
	",time=" + doubleToString(current_time_,2);
      m_Comms.Notify("TRACK_STAT",buff); 
    }
  // 
  // Update classify mode
  
  if ( prosecute_mode_active_ && classifying_active_ )
    {
      glog.is(DEBUG1) && glog << "Classifying time: " << curr_time-classifying_start_ 
			      << std::endl;
      if ((curr_time-classifying_start_) >= cfg_.max_classify_time())
	{
	  queue_event<EvDcltOff>();
	  publish ("CLASS_DONE","TRUE");
	}
    }

// process some events at a slower rate
    const int slow_loop_period = 10;
    if(curr_time > last_slow_loop_t_ + slow_loop_period)
        slow_loop(curr_time);


}

void LamssTrackManager::slow_loop(double curr_time)
{
  // Here we should add slow loop actions.

  last_slow_loop_t_ = curr_time;
}

void LamssTrackManager::try_publish()
{
    boost::mutex::scoped_try_lock lk(moos_mutex_);
    if(lk.owns_lock())   
    {
        while(!publish_queue_.empty())
	{
            glog.is(DEBUG2) && glog << "Publishing: " << publish_queue_.front().GetKey() << ": " << publish_queue_.front().GetAsString()  << std::endl;
            publish(publish_queue_.front());
            publish_queue_.pop_front();
	}
    }
}

void LamssTrackManager::handle_ifs_data(const CMOOSMsg& msg)
{
    goby::moos::protobuf::FrontSeatInterfaceData data;
    parse_for_moos(msg.GetString(), &data);

    if(data.has_node_status())
    {
        last_nav_ = data.node_status();
    }
}

void LamssTrackManager::handle_deploy(const CMOOSMsg& msg)
{
  LAMSS_DEPLOY deploy_cmd;
  parse_for_moos(msg.GetString(), &deploy_cmd);
  if (deploy_cmd.destinationplatformid() == vehicle_id_)
    {
      last_deploy_cmd_ = deploy_cmd;
      sonar_on_ = (last_deploy_cmd_.sonar_control() == LAMSS_DEPLOY::SONAR_CONTROL_ON);
    }
}

void LamssTrackManager::handle_prosecute(const CMOOSMsg& msg)
{
  LAMSS_PROSECUTE prosecute_cmd;
  parse_for_moos(msg.GetString(), &prosecute_cmd);
  if (prosecute_cmd.destinationplatformid() == vehicle_id_)
    last_prosecute_cmd_ = prosecute_cmd;

  // set up  search parameters from target cue

  cue_utc_ = last_prosecute_cmd_.target_timestamp();
  cue_x_ = last_prosecute_cmd_.target_x();
  cue_y_ = last_prosecute_cmd_.target_y();
  cue_heading_= last_prosecute_cmd_.target_heading();
  cue_speed_ = last_prosecute_cmd_.target_speed();
  cue_depth_ = last_prosecute_cmd_.target_depth();

  double auv_x =last_nav_.local_fix().x();
  double auv_y =last_nav_.local_fix().y();
  //bearing to target 
  double cue_bearing = atan2(cue_x_ - auv_x,
			 cue_y_ - auv_y)*180.0/M_PI;
	        
  //vector addition
  double res_vector = 180.0/M_PI*
    atan2((sin(cue_bearing*M_PI/180.0)+sin(cue_heading_*M_PI/180.0)),
	  (cos(cue_bearing*M_PI/180.0)+cos(cue_heading_*M_PI/180.0)));
  
  res_vector= angle360(res_vector);
  
  double new_heading;
  double h_diff = fabs(cue_bearing-res_vector);
  if(h_diff <= 45.0)    
    {
      //pass behind target   
      if((sin(cue_bearing*M_PI/180)*cos(cue_bearing*M_PI/180)) < 0.0)
	new_heading = angle360(cue_bearing - h_diff);
      else
	new_heading = angle360(cue_bearing + h_diff);
    }
  else
    //pass in front
    new_heading = res_vector;

  std::string buff = "heading=" + doubleToString(new_heading,1);
  publish("INIT_TRACK_HEADING",buff);   
  
  buff = "heading=" + doubleToString(cue_heading_,1);
  publish("PROSECUTE_HEADING",buff);
  
}


void LamssTrackManager::handle_vehicle_id(const CMOOSMsg& msg)
{
    boost::mutex::scoped_lock lock(moos_mutex_);
    vehicle_id_ = msg.GetDouble();
}

void LamssTrackManager::handle_collaborator_id(const CMOOSMsg& msg)
{
    collaborator_id = msg.GetDouble();
}

void LamssTrackManager::handle_collaborator_name(const CMOOSMsg& msg)
{
    collaborator_name = msg.GetString();
    subscribe(collaborator_name+"_BEARING_TRACKS", 
	      &LamssTrackManager::handle_collaborator_bearing_tracks, this);  
}

void LamssTrackManager::handle_mission_state(const CMOOSMsg& msg)
{
  std::string mission_state = msg.GetString();
  deploy_mode_active_ = MOOSStrCmp(mission_state,"DEPLOY"); 
  prosecute_mode_active_ = MOOSStrCmp(mission_state,"PROSECUTE");
  mission_active_ = deploy_mode_active_ || prosecute_mode_active_;
  if (mission_active_)
    reset_tracking();
}

void LamssTrackManager::handle_ael_heading(const CMOOSMsg& msg)
	      {
		raw_heading = msg.GetDouble();
		raw_heading = angle360(raw_heading);
		heading = raw_heading;
		if (filt_set)
		  {
		    heading_filter->set_RawVal(raw_heading);
		    heading = angle360(heading_filter->get_FVal());     //HS 2007/07/12
		  }
	      }

void LamssTrackManager::handle_bearing_tracks(const CMOOSMsg& msg)
{
  std::string sval = msg.GetString();
  CTrack TrackBuf;
  TrackBuf.decodeTrack(sval);
  int in_trk= TrackBuf.trk_num;
  if ( in_trk > num_tracks )
    {
      glog.is(DEBUG2) && glog 
	<< "Non-existing track number " << in_trk
	<< ". Adding new track" << std::endl;
      for (int i=num_tracks; i< in_trk; i++)
	{
	  Tracks.push_back(Empty);
	  new_track.push_back(false);
	}
      num_tracks = in_trk;
    }
  Tracks[in_trk-1]=TrackBuf;
  new_track[in_trk-1] = true;
  std::string moosvar;
  Tracks[in_trk-1].postTrack(moosvar,sval);
  // goby::glog << group("track") 
    glog.is(VERBOSE) && glog 
	     << "New track: " << sval << std::endl;
}

void LamssTrackManager::handle_collaborator_bearing_tracks(const CMOOSMsg& msg)
{
  std::string sval = msg.GetString();
  CTrack TrackBuf;
  TrackBuf.decodeTrack(sval);
  int in_trk= TrackBuf.trk_num;
  if (in_trk > 0)
    {
      if ( in_trk > collab_num_tracks)
	{
	  glog.is(DEBUG2) && glog 
	    << "Non-existing collaborator track number " << in_trk
	    << ". Adding new track" << std::endl;
	  for (int i=collab_num_tracks; i< in_trk; i++)
	    {
	      collab_Tracks.push_back(Empty);
	      collab_new_track.push_back(false);
	    }
	  collab_num_tracks = in_trk;
	}
      collab_Tracks[in_trk-1]=TrackBuf;
      collab_new_track[in_trk-1] = true;
      std::string moosvar;
      collab_Tracks[in_trk-1].postTrack(moosvar,sval);
      // goby::glog << group("track") 
      glog.is(DEBUG2) && glog 
	<< "New collaborator track: " << sval << std::endl;
      postStateCollab();
    }	      
}

void LamssTrackManager::handle_prosecute_heading(const CMOOSMsg& msg)
{
  MOOSValFromString(prosecute_heading,msg.GetString(),"heading");
}

void LamssTrackManager::send_deploy()
{
  std::string serialized;
  if (!MOOSStrCmp(cfg_.deploy_location(),"station"))
    {
      last_deploy_cmd_.set_deploy_x(last_nav_.local_fix().x());
      last_deploy_cmd_.set_deploy_y(last_nav_.local_fix().y());
    }
  serialize_for_moos(&serialized,last_deploy_cmd_);    
  publish("LAMSS_DEPLOY_IN", serialized);
}

void LamssTrackManager::handle_track_stat(const CMOOSMsg& msg)
{
  std::string track_stat =  msg.GetString();
  
  MOOSChomp(track_stat,"node=");
  std::string tracknode = MOOSChomp(track_stat,",");
  int track_node = atoi(tracknode.c_str());
  
  MOOSChomp(track_stat,"state=");
  std::string trackstate = MOOSChomp(track_stat,",");
  
  if(trackstate == "2" && (track_node == vehicle_id_) )
    {
      MOOSChomp(track_stat,"x=");
      std::string Tstat_x = MOOSChomp(track_stat,",");
      double track_x = atof(Tstat_x.c_str());
      MOOSChomp(track_stat,"y=");
      std::string Tstat_y = MOOSChomp(track_stat,",");
      double track_y = atof(Tstat_y.c_str());
      MOOSChomp(track_stat,"heading=");
      std::string Tstat_course = MOOSChomp(track_stat,",");
      double track_hdg = atof(Tstat_course.c_str());
      MOOSChomp(track_stat,"speed=");
      std::string Tstat_speed = MOOSChomp(track_stat,",");
      double track_spd = atof(Tstat_speed.c_str());
      MOOSChomp(track_stat,"time=");
      std::string Tstat_utc = MOOSChomp(track_stat,",");
      double track_utc = atof(Tstat_utc.c_str());
      
      geo_track_count_++;
      if (geo_track_count_ == 1)
	clearTrackBuffer();
      else
	tracking_duration_ = current_time_ - tracking_start_;
      glog.is(DEBUG1) && glog 
	<< "Geo Track Count =" << geo_track_count_ 
	<<", Tracking duration =" << tracking_duration_ << std::endl;
      
      // Update track buffer and compute uncertainty
      int buf_len = updateTrackBuffer(track_utc,track_x,track_y,
				      track_spd,track_hdg);
      glog.is(DEBUG2) && glog 
	<< "Current buffer length =" << buf_len << std::endl;
      glog.is(DEBUG2) && glog 
	<< "Max buffer length =" << m_buffer_length << std::endl;
      double xy_std = computeSigma();
      glog.is(DEBUG2) && glog 
	<< "xy_std =" << xy_std << ", xy_sigma =" << m_xy_sigma << std::endl;
      bool good_track = (xy_std < m_xy_sigma) && (buf_len >= m_buffer_length);
      if (good_track)
	{
	  if (track_report_count_ == 0)
	    m_report_start = current_time_;
	  
	  glog.is(DEBUG2) && glog 
	    << "Time since first report " << current_time_-m_report_start 
	    << std::endl;
      track_report_count_++;
      
      std::string track_report = msg.GetString();
      double track_lat,track_lon;
      
      if (m_Geodesy.UTM2LatLong(track_x,track_y,track_lat,track_lon))
	{
	  track_report += ",track_lat=" + doubleToString(track_lat) 
	    + ",track_lon=" + doubleToString(track_lon);
	}
      publish("TRACK_REPORT",track_report);
      glog.is(DEBUG1) && glog 
	<< "Good tracks reported: " << track_report_count_ << std::endl;
      // Post node report for adaptive behaviors
      std::string node_report = trackNodeReport("priority_trk",track_x, track_y, 
						track_spd, 
						track_hdg, 0.0 );
      publish("NODE_REPORT",node_report);
      publish("PRIORITY_TRK","contact=priority_trk");
      
      if (current_time_ - m_report_start >= m_track_report_time)
	{
	  if (last_prosecute_cmd_.prosecute_mission() == 
	      LAMSS_PROSECUTE::PROSECUTE_MISSION_TRACK_ONLY )
	    {
	      // Tracking only, send deploy
	      if (prosecute_mode_active_)
		{
		  glog.is(DEBUG1) && glog 
		    << "Acceptable tracking solution. DEPLOY Executed"
		    << std::endl;
		  queue_event<EvDcltOff>();
		  send_deploy();
		}
	    }
	  else if (last_prosecute_cmd_.prosecute_mission() == 
		   LAMSS_PROSECUTE::PROSECUTE_MISSION_TRACK_CLASSIFY )
	    {            
	      // Switch to classify
	      queue_event<EvToClassifying>();
	      //m_Comms.Notify("TRACKING","CLASSIFYING");
	    }
	}
	}
      else if ( tracking_duration_ > cfg_.max_tracking_time() )
	{		// Tracking only, send deploy
	  track_report_count_ = 0;
	  if (prosecute_mode_active_)
	    {
	      glog.is(DEBUG1) && glog 
		<< "DEPLOY Executed: Max tracking time exceeded" << std::endl;
	      send_deploy();
	    }
	}
    }
}

void LamssTrackManager::handle_bearing_stat(const CMOOSMsg& msg)
{
  std::string bearing_stat =  msg.GetString();
  int bstat_node;
  int bstate;
  double bearing;
  double brg_sigma;
  double auv_x;
  double auv_y;
  double meas_time;

  parse_brng_stat(bearing_stat,
		  bstat_node,
		  bstate,
		  bearing,
		  brg_sigma,
		  auv_x,
		  auv_y,
		  meas_time);

  if ( bstat_node == vehicle_id_)
    {
      if ( prosecute_mode_active_ && 
	   close_range_ )
	{
	  if( bearing_track_count_%10 == 0 )
	    {
	      if ( bearing_track_count_ > 0 ) 
		{
		  double delta_bearing;
		  if ( (bearing- prev_bearing_) > 180.0)
		    delta_bearing = bearing - prev_bearing_ - 360.0;
		  else
		    {
		      if ( (bearing-prev_bearing_) < -180.0)
			delta_bearing = bearing - prev_bearing_ + 360.0;
		      else
			delta_bearing = bearing - prev_bearing_;
		    }
		  // Bearing rate in degree per minute
		  double bearing_rate = fabs(60*delta_bearing/(meas_time-prev_bearing_time_));
		  if ( (bearing_rate > bearing_rate_max_))
		    {
		      bearing_rate_max_ = bearing_rate;
		      bearing_rate_threshold_ = bearing_rate_max_ * 
			cfg_.bearing_rate_threshold()/100.0;
		      bearing_neg_trend_ = false;
		    }
		  else
		    bearing_neg_trend_ = true;
		  if ((bearing_track_count_ > 60) && 
		      (bearing_rate < bearing_rate_threshold_) && 
		      prosecute_mode_active_)
		    {
		      send_deploy();
		      glog.is(DEBUG1) && glog 
			<< "DEPLOY Executed: Bearing Rate Threshold Reached " 
			<< std::endl;
		    }
		}          
	      prev_bearing_time_=meas_time;
	      prev_bearing_=bearing;
	    }
	  bearing_track_count_++;
	}
      else
	{
	  bearing_rate_max_ = -1E20;
	  bearing_neg_trend_ = false;
	  bearing_rate_threshold_ = 0;
	}
    }
}

bool LamssTrackManager::checkOpportunity()
// This function checks whether target will ever be getting inside the 
// giveup/detection range
{
  double h_ang = (90.0-cue_heading_)*M_PI/180.0;
  if (h_ang <= -M_PI)
    h_ang = h_ang + 2*M_PI;
  else if (h_ang > M_PI)
    h_ang = h_ang - 2*M_PI;
  
  double auv_x =last_nav_.local_fix().x();
  double auv_y =last_nav_.local_fix().y();
  double auv_speed =last_nav_.speed();

  double b_ang =atan2(cue_y_- auv_y,
		      cue_x_- auv_x);
  double t_range=sqrt(pow(cue_x_-auv_x,2)+pow(cue_y_-auv_y,2));

  if (t_range < cfg_.max_search_range())
    return(true);

  double dif_ang = h_ang-b_ang;
  if (dif_ang <= -M_PI)
    dif_ang = dif_ang + 2*M_PI;
  else if (dif_ang > M_PI)
    dif_ang = dif_ang - 2*M_PI;

  double cos_ang = cos(dif_ang);  
  if (cos_ang <  0)
    {
      //	         coming this way - check if close enough
      // time to CPA 
      double time_to_cpa = -cos_ang*t_range/cue_speed_;
      double my_dist = time_to_cpa * auv_speed;
      if (fabs(t_range*sin(dif_ang))- my_dist <= cfg_.max_search_range())
	return(true);
    }
  else
    {
      // Closing range ?
      // Relative target speed
      double closing_speed = auv_speed - cue_speed_ * cos_ang;
      if (closing_speed > 0 && (t_range <= cfg_.max_search_range()))
	return(true);
    }

    return(false);
}

void LamssTrackManager::handle_close_range(const CMOOSMsg& msg)
{
  std::string close_state = msg.GetString();
  if (MOOSStrCmp(close_state,"TRUE"))
    {
      if( !close_range_)
	{
	  close_range_ = true;
	  glog.is(DEBUG2) && glog 
	    << "In tracking range" << std::endl;
	}
    }
  else
    close_range_ = false;
}
      
void LamssTrackManager::handle_class_done(const CMOOSMsg& msg)
{
  std::string class_state = msg.GetString();
  if (prosecute_mode_active_ && class_state =="TRUE")
    {
      send_deploy();
      glog.is(DEBUG1) && glog 
	<< "DEPLOY Executed: Classification done " << std::endl;
    }
  
}

void LamssTrackManager::reset_tracking()
{
  //	m_Comms.Notify   ("TRACKING","NO_TRACK");  
  if (dclt_active_)
    queue_event<EvDcltReset>();
  publish("TRACKER_COMMAND","RESET");
  
  for (int i=0; i<num_tracks; i++)
    { 
      Tracks[i].clrTrack();
    }
  
  for (int i=0; i<collab_num_tracks; i++)
    { 
      collab_Tracks[i].clrTrack();
    }
  
  std::string msg;
  char buff[200];
  sprintf(buff,"node=%d,state=%d,bearing=%.4f,xp=%.3f,yp=%.3f,beamno=%d,sigma=%.3f,time=%.3f",node,NO_TRACK,0.0,0.0,0.0,0,0.0,MOOSTime());
  msg = buff;
  publish("BEARING_STAT",msg);
  
  if (collaborator_id > 0)
    {
      sprintf(buff,"node=%d,state=%d,bearing=%.4f,xp=%.3f,yp=%.3f,beamno=%d,sigma=%.3f,time=%.3f",collaborator_id,NO_TRACK,0.0,0.0,0.0,0,0.0,MOOSTime());
      msg = buff;
      publish(collaborator_name+"_BEARING_STAT" ,msg);
    }
  return;
}

bool LamssTrackManager::postStateVars()
{

  char buff[200];
  std::string msg;
  double current =  MOOSTime();


  int track_count = 0;

  int highest_prio = 0;
  int highest_index = -1;

  // If prosecuting, concentrate on cued template (tgt_track_id)
  for (int i=0; i<num_tracks; i++)
    { 
      Tracks[i].getTrack(trk_num, trk_prio, trk_template, trk_state, 
			 trk_ips, trk_x, trk_y, 
			 trk_bearing, trk_sigma, trk_rate, trk_rate_sigma, 
			 trk_snr, trk_utc, class_features);
      if (current -  trk_utc < track_delay) 
	{
	  bool interest = ( (prosecute_mode_active_ && 
			     ((trk_template == tgt_track_id) || 
			      (trk_template == 0) || 
			      (tgt_track_id == 0))) || 
			    !prosecute_mode_active_);
	  //goby::glog << group("track") 
	  glog.is(DEBUG2) && glog 
	    << "trk_template_=" << trk_template 
	    << ", tgt_track_id =" << tgt_track_id
	    << ", track_=" << trk_num 
	    << ", priority =" << trk_prio << std::endl;
	  if ( interest && ( trk_prio > highest_prio) )
	    {
	      highest_prio = trk_prio;
	      highest_index = i;
	    }
	}
      else
	{
	  Tracks[i].clrTrack();
	  new_track[i] = false;
	  if (i == prev_highest)
	    {
	      char buff[200];
	      sprintf(buff,"node=%d,state=%d,bearing=%.4f,xp=%.3f,yp=%.3f,beamno=%d,sigma=%.3f,time=%.6f",node,NO_TRACK,0.0,0.0,0.0,0,0.0,MOOSTime());  
	      msg = buff;
	      publish("BEARING_STAT",msg);
	    }
	}
    }
  
    if (dclt_active_ && highest_index != prev_highest)
      queue_event<EvDcltReset>();
  //    tracking_published = false;
  
  //goby::glog << group("track") 
    glog.is(DEBUG2) && glog 
      << "Highest=" << highest_index
      << ",priority=" << highest_prio << std::endl;
 
  // For now we do the highest priority track only
  if (highest_index >= 0 && highest_prio >= priority_threshold)
    if (new_track[highest_index] )
      {
	Tracks[highest_index].getTrack(trk_num, trk_prio, trk_template, 
				       trk_state, trk_ips, trk_x, trk_y, 
				       trk_bearing, trk_sigma, trk_rate, 
				       trk_rate_sigma, trk_snr, trk_utc, 
				       class_features);
	// goby::glog << group("track") 
	glog.is(DEBUG2) && glog 
	  << "Highest=" << highest_index
	  << ", trk=" << trk_num 
	  << ", prio=" << trk_prio
	  << ", state=" << trk_state
	  << ", ips=" << trk_ips
	  << ", bearing=" << trk_bearing << std::endl;
	// If ambiguity is broken, go to Tracking	  
	if (trk_state == ABSOLUTE)
	  {
	    if (localization_active_)
	      {
		// Publish broadside heading
		double broadside_heading = angle360(trk_bearing + 90.0);
		if (fabs(broadside_heading - prosecute_heading) <= 90.0 || 
		    fabs(broadside_heading - prosecute_heading) >= 270.0)
		  publish("BROADSIDE_HEADING","heading="+
			  doubleToString(broadside_heading,1));
		else
		  publish("BROADSIDE_HEADING","heading="+
			  doubleToString(angle360(broadside_heading + 180.0)));
		
		// set tracking depth to current depth for non-adaptive tracking
		std::string tracking_depth_update = "depth=" 
		  + doubleToString(last_nav_.global_fix().depth());
		publish("SENSOR_DEPTH_TRACKING",tracking_depth_update);
		queue_event<EvToTracking>();
		// m_Comms.Notify ("TRACKING","TRACKING");
	      }
	  }
	else if (trk_state == RELATIVE)
	  {
	    if (trk_ips == 0)
	      {
		// When detection is made switch to breaking ambiguity
		// set tracking depth to current depth for non-adaptive tracking
		if (detection_active_)
		  {
		    std::string tracking_depth_update = "depth=" 
		      + doubleToString(last_nav_.global_fix().depth());
		    publish("SENSOR_DEPTH_TRACKING",tracking_depth_update);
		    queue_event<EvToLocalization>();
		  }		// m_Comms.Notify ("TRACKING","AMBIGUOUS");
	      } 
	    else
	      {
		double absolute = angle360(heading + trk_ips*trk_bearing);
		if (!tracking_active_)
		  {
		    // Publish broadside heading
		    double broadside_heading = angle360(absolute + 90.0);
		    if (fabs(broadside_heading - heading) <= 90.0 || 
			fabs(broadside_heading - heading) >= 270.0)
		      publish("BROADSIDE_HEADING","heading="+
			      doubleToString(broadside_heading,1));
		    else
		      publish("BROADSIDE_HEADING","heading="+
			      doubleToString(angle360(broadside_heading + 180.0)));
		    
		    queue_event<EvToTracking>();
		    // m_Comms.Notify ("TRACKING","TRACKING");
		  }
		Tracks[highest_index].setTrack(trk_num, trk_prio, trk_template, 
					       ABSOLUTE, trk_ips, trk_x, trk_y, 
					       absolute, trk_sigma, trk_rate, 
					       trk_rate_sigma, trk_snr, class_features);
	      }
	  }
	else
	  {
	    queue_event<EvDcltReset>();
	    // m_Comms.Notify ("TRACKING","NO_TRACK");
	    // tracking_published = false;
	  }
	
	if (current -  trk_utc > track_delay) 
	  new_track[highest_index] = false;
	
	// set BEARING_STATE
	if (highest_index >= 0)
	  msg = Tracks[highest_index].bearingState(node,trk_x, trk_y);
	else
	  {
	    char buff[200];
	    sprintf(buff,"node=%d,state=%d,bearing=%.4f,xp=%.3f,yp=%.3f,beamno=%d,sigma=%.3f,time=%.3f",node,NO_TRACK,0.0,0.0,0.0,0,0.0,MOOSTime());
	    msg = buff;
	  }
	
	publish("BEARING_STAT",msg);
	prev_highest = highest_index;
      }
  
  for (int i=0; i<num_tracks; i++)
    {
      Tracks[i].getTrack(trk_num, trk_prio, trk_template, trk_state, 
			 trk_ips, trk_x, trk_y, 
			 trk_bearing, trk_sigma, trk_rate, trk_rate_sigma, 
			 trk_snr, trk_utc, class_features);
      if (new_track[i]) 
	{
	  if (trk_state == ABSOLUTE)
	    {
	      if (track_count == 0)
		BearingTrack.initReport(node, trk_x,trk_y, last_nav_.speed(), 
					heading, trk_utc);
	      track_count = BearingTrack.reportTrack(trk_num,trk_prio,trk_template,
						     trk_bearing, trk_sigma, 
						     trk_rate, trk_rate_sigma, trk_snr);
	      if (track_count >= 1)
		{
		  msg = BearingTrack.createContactReport(); 
		  publish("CONTACT_REPORT",msg);
		  track_count = 0;
		}
	    }
	  new_track[i] = false;
	} 
    }
  if (track_count > 0)
    {
      msg = BearingTrack.createContactReport(); 
      publish("CONTACT_REPORT",msg);
      track_count = 0;
    }
  
  return(true);
}

bool LamssTrackManager::postStateCollab()
{

  char buff[200];
  std::string msg;
  double current =  MOOSTime();

  int trk_num;
  int trk_prio; // 1-10, 10 highest.
  int trk_template;
  int trk_state; // 0: Invalid, 1: Relative, 2: Bbsolute.
  int trk_ips;   // -1: Port, 1: Starboard, 0: Ambiguous
  double trk_x; // array x corresponding to track
  double trk_y; // array y corresponding to track
  double trk_bearing;
  double trk_sigma;
  double trk_utc;
  double trk_rate;
  double trk_rate_sigma;
  double trk_snr;
  int trk_nfeat;
  int trk_beamno;
  std::vector<double> class_features;

  int track_count = 0;

  int highest_prio = 0;
  int highest_index = -1;

  for (int i=0; i<collab_num_tracks; i++)
    { 
      collab_Tracks[i].getTrack(trk_num, trk_prio, trk_template, trk_state, 
				trk_ips, trk_x, trk_y, 
				trk_bearing, trk_sigma, trk_rate, trk_rate_sigma, 
				trk_snr, trk_utc, class_features);
      if (current -  trk_utc < track_delay) 
	{
	  if (trk_prio > highest_prio)
	    {
	      highest_prio = trk_prio;
	      highest_index = i;
	    }
	}
      else
	{
	  collab_Tracks[i].clrTrack();
	  collab_new_track[i] = false;
	}
      
    }
  
  //goby::glog << group("track") 
  glog.is(DEBUG2) && glog 
    << "Collaborator: Highest=" << highest_index
    << ", priority=" << highest_prio << std::endl;
  
  if (highest_index >= 0 && highest_prio >= priority_threshold)
    if (collab_new_track[highest_index] )
      {
	collab_Tracks[highest_index].getTrack(trk_num, trk_prio, trk_template, 
					      trk_state, trk_ips, trk_x, trk_y, 
					      trk_bearing, trk_sigma, trk_rate, 
					      trk_rate_sigma, 
					      trk_snr, trk_utc, class_features);
	//goby::glog << group("track") 
	glog.is(DEBUG2) && glog 
	     << "Highest=" << highest_index
	     << ", trk=" << trk_num
	     << ", prio=" << trk_prio
	     << ", state=" << trk_state
	     << ", ips=" << trk_ips
	     << ", bearing=" << trk_bearing << std::endl;

	if (current -  trk_utc > track_delay) 
	  collab_new_track[highest_index] = false;

	// set BEARING_STATE
	msg = collab_Tracks[highest_index].bearingState(collaborator_id,trk_x, trk_y);
	publish(collaborator_name+"_BEARING_STAT" ,msg);
      }

  return(true);
}

double LamssTrackManager::angle360(double angle)
{
  double res = angle;
  //  unwrap angles
  while (res >= 360.0)
    res -= 360.0;
  while (res < 0.0)
    res += 360.0;
  
  return(res);
  
}

// >>>>>>>>>>>>>>> vector utility methods <<<<<<<<<<<<
int LamssTrackManager::str2dbl_vec(std::string msg, std::vector<double>& values)
{
  std::string bite;
  
  values.clear();
  int nz = 0;
  while (msg.length() > 0)
    {
      bite = MOOSChomp(msg,",");
      values.push_back(atof(bite.c_str()));
      nz++;
    }
  
  return(nz);
}

bool LamssTrackManager::dbl_vec2str(std::vector<double> numbers, std::string& output)
{
  output = "";
  for(int i=0; i<numbers.size(); i++) 
    {
      if(i > 0)
	output += ",";
      std::string istr = doubleToString(numbers[i], 2); // 2 is the precision 123.45
      output += istr;
    }
  return(true);
}

//***************************************************************************
//*  Mavg::set_RawVal()                                                     *
//*                                                                         *
//*  This method calculates the moving average of a sequence of angles.     *
//*                                                                         *
//*************************************************************************** 

Mavg::Mavg(int length)
{

	len = length;
	num_measurements = 0;
}

Mavg::~Mavg()
{
}

void Mavg::set_RawVal(double val)
{
	int avg_len;
	double csum,ssum,s,c;

	if (num_measurements < len)
	{	  
		s = sin(val*M_PI/180.0);
		c = cos(val*M_PI/180.0);
		svals.push_back(s);
		cvals.push_back(c);
		avg_len = num_measurements + 1;
		num_measurements += 1;
	}
	else
	{
		s = sin(val*M_PI/180);
		c = cos(val*M_PI/180.0);
		svals.push_back(s);
		cvals.push_back(c);
		svals.pop_front();
		cvals.pop_front();
		avg_len = len;      
	}

	csum = 0.0;
	ssum = 0.0;

	for (int kk = 0; kk < avg_len; kk++)
	{   
		csum +=cvals[kk];
		ssum +=svals[kk];
	}

	avg = atan2(ssum,csum)*180.0/M_PI;

//MOOSTrace("ssum = %f, csum = %f, average angle = %f\n",ssum,csum,avg);
}


//***************************************************************************
//*  Mavg::get_FVal()                                                       *
//*                                                                         *
//*  This method returns the value of the moving average                    *
//*                                                                         *
//*************************************************************************** 
double Mavg::get_FVal()  
{
  if (num_measurements > 0)
    return (avg);
  else
    return(0.0);
}


bool CReport::initReport(int node, double x, double y, double spd, double hdg, double utc)
{

  trk_node = node;
  ael_x = x;
  ael_y = y;
  nav_spd = spd;
  nav_hdg = hdg;
  nav_utc = utc;

  num_trk = 0;

  trk_id.clear();
  trk_priority.clear();
  trk_template.clear();
  trk_bearing.clear();
  trk_sigma.clear();
  trk_rate.clear();
  trk_rate_sigma.clear();
  trk_snr.clear();

  return(true);
}
  
int CReport::reportTrack(int id, int prio, int templ, double bearing, double sigma, 
			 double rate, double rate_sigma, double snr)
{

  trk_id.push_back(id);
  trk_priority.push_back(prio);
  trk_template.push_back(templ);
  trk_bearing.push_back(bearing);
  trk_sigma.push_back(sigma);
  trk_rate.push_back(rate);
  trk_rate_sigma.push_back(rate_sigma);
  trk_snr.push_back(snr);

  num_trk++;

  return(num_trk);
}

std::string CReport::createContactReport()
{
  std::string msg = "MessageType=CONTACT_REPORT";
  msg += ",node=" + intToString(trk_node) ;
  msg += ",time=" + doubleToString(nav_utc) ;

  for (int i=0; i<num_trk; i++)
    {
      msg += ",track=" + intToString(trk_id[i]);
      msg += ",priority=" + intToString(trk_priority[i]);
      msg += ",template=" + intToString(trk_template[i]);
      msg += ",ael_x=" + doubleToString(ael_x) ;
      msg += ",ael_y=" + doubleToString(ael_y) ;
      msg += ",bearing=" + doubleToString(trk_bearing[i],2);
      msg += ",bearing_error=" + doubleToString(trk_sigma[i],2);
      msg += ",rate=" + doubleToString(trk_rate[i],3);
      msg += ",rate_error=" + doubleToString(trk_rate_sigma[i],3);
      msg += ",snr=" + doubleToString(trk_snr[i],2);
    }
  return(msg);
}

int LamssTrackManager::updateTrackBuffer(double track_utc,
				      double track_x,
				      double track_y,
				      double track_spd,
				      double track_hdg)
{
  int i;
  
  double rhead = 90.0-track_hdg*M_PI/180.0;
  double track_xdot = track_spd * cos(rhead);
  double track_ydot = track_spd * sin(rhead);
 
  if (m_track_utc.size() >= m_buffer_length)
    {
      for(i=1; i<m_buffer_length; i++) 
	{
	  m_track_utc[i-1] = m_track_utc[i]; 
	  m_track_x[i-1]   = m_track_x[i] ;
	  m_track_y[i-1]   = m_track_y[i] ;
	  m_track_xdot[i-1] = m_track_xdot[i] ;
	  m_track_ydot[i-1] = m_track_ydot[i] ;
	}
      m_track_utc[m_buffer_length] = track_utc;
      m_track_x[m_buffer_length]   = track_x;
      m_track_y[m_buffer_length]   = track_y;
      m_track_xdot[m_buffer_length] = track_xdot;
      m_track_ydot[m_buffer_length] = track_ydot;
    }
  else
    {
      m_track_utc.push_back(track_utc);
      m_track_x.push_back(track_x);
      m_track_y.push_back(track_y);
      m_track_xdot.push_back(track_xdot);
      m_track_ydot.push_back(track_ydot);
    }
  glog.is(DEBUG2) && glog 
    << "Updating track buffer, length = " << m_track_utc.size() << std::endl;
  return(m_track_utc.size());
}

 void LamssTrackManager::clearTrackBuffer()
 {
   glog.is(DEBUG2) && glog 
     << "Clearing track buffer, length = " << m_track_utc.size() << std::endl;
   m_track_utc.clear();
   m_track_x.clear();
   m_track_y.clear();
   m_track_xdot.clear();
   m_track_ydot.clear();
   return ;
 }

double LamssTrackManager::computeSigma()
{
  int n = m_track_utc.size();
  m_utc_mean = 0;
  m_x_mean = 0;
  m_y_mean = 0;
  m_xdot_mean = 0;
  m_ydot_mean = 0;

  for (int i=0; i < n; i++)
    {
      m_utc_mean += m_track_utc[i];
      m_x_mean   += m_track_x[i];
      m_y_mean   += m_track_y[i];
      m_xdot_mean += m_track_xdot[i];
      m_ydot_mean += m_track_ydot[i];
    }
  m_utc_mean = m_utc_mean/n;
  m_x_mean = m_x_mean/n;
  m_y_mean = m_y_mean/n;
  m_xdot_mean = m_xdot_mean/n;
  m_ydot_mean = m_ydot_mean/n;
  m_heading_mean = 90.0 - atan2(m_ydot_mean,m_xdot_mean);
  if (m_heading_mean < 0)
    m_heading_mean += 360;
  m_speed_mean = sqrt(pow(m_xdot_mean,2) + pow(m_ydot_mean,2));

  double xy_std = 0;
 
  for (int i=0; i < n; i++)
    {
      double x = m_x_mean + m_xdot_mean*(m_track_utc[i]-m_utc_mean);
      double y = m_y_mean + m_ydot_mean*(m_track_utc[i]-m_utc_mean);
      xy_std += pow(x-m_track_x[i],2) + pow(y-m_track_y[i],2);
   }
  if (n > 1)
    xy_std = sqrt(xy_std/(n-1));
  else 
    xy_std = 1e20;

  return(xy_std);
}

std::string LamssTrackManager::trackNodeReport(std::string trk_name, double trk_x, double trk_y, double trk_spd, double trk_hdg, double trk_dep )
{

  double trk_lat;
  double trk_lon;

  std::string output="NAME=" + trk_name +",TYPE=track,";

  if (!m_Geodesy.UTM2LatLong(trk_x,trk_y,trk_lat,trk_lon))
    {
      glog.is(DEBUG2) && glog 
	<< "Error in UTM2LatLong conversion" << std::endl;
      return("Error in UTM2LatLong conversion\n");	  
    }

  output += "UTC_TIME=" + doubleToString(current_time_,1);
  output += ",";

  output += "X=" + doubleToString(trk_x,2);
  output += ",";

  output += "Y=" + doubleToString(trk_y,2);
  output += ",";

  output += "LAT=" + doubleToString(trk_lat,6);
  output += ",";

  output += "LON=" + doubleToString(trk_lon,6);
  output += ",";

  output += "SPD=" + doubleToString(trk_spd,2);
  output += ",";

  output += "HDG=" + doubleToString(trk_hdg,2);

  // output += ",";

  // output += "DEPTH=" + doubleToString(trk_dep,2);

  return(output);
}
