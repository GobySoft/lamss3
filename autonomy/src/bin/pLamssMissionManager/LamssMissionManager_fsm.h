// Copyright 2014 
// Henrik Schmidt
// Toby Schneider 
// Massachusetts Institute of Technology 

#ifndef LAMSSTrackManagerFSM20150929H
#define LAMSSTrackManagerFSM20140929H

#include <boost/statechart/event.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/in_state_reaction.hpp>
#include <boost/statechart/custom_reaction.hpp>
#include <boost/statechart/asynchronous_state_machine.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <boost/mpl/list.hpp>
#include <iostream>

#include "goby/common/logger.h"
#include "goby/moos/moos_protobuf_helpers.h"
#include "goby/moos/protobuf/frontseat.pb.h"
#include "goby/moos/frontseat/bluefin/bluefin.pb.h"
#include "goby/acomms/protobuf/amac.pb.h"

#include "MOOS/libMOOS/Comms/MOOSMsg.h"
#include "MBUtils.h"

#include "LamssMissionManager_config.pb.h"
#include "lamss/lib_lamss_protobuf/lamss_status.pb.h"
#include "lamss/lib_lamss_protobuf/lamss_deploy.pb.h" // DCCL message
#include "lamss/lib_lamss_protobuf/lamss_prosecute.pb.h" // DCCL message

namespace lamss
{
    namespace shared
    {
        namespace fsm
        {
            
            // events
    
            struct EvAbortMission : boost::statechart::event< EvAbortMission > { };
            struct EvStartDiving : boost::statechart::event< EvStartDiving > { };
	    struct EvNewMission : boost::statechart::event< EvNewMission > { };
	    // Deploy events
            struct EvStartDeploy : boost::statechart::event< EvStartDeploy > { };
 	    // Prosecute events
            struct EvStartProsecute : boost::statechart::event< EvStartProsecute > { };
	    // Abort event
            struct EvStartAbort : boost::statechart::event< EvStartAbort > { };
	    // Drifting event
            struct EvStartDrifting : boost::statechart::event< EvStartDrifting > { };

            struct Idle;
            struct Mission;
            struct Diving;
	    // Deploy States
            struct Deploy;

	    // Prosecute States 
            struct Prosecute;

	    // Abort State
            struct Abort;

	    // Drifting State
            struct Drifting;

            extern std::deque<CMOOSMsg> publish_queue_;
      
            extern Mission_StateEnum state_;

            extern boost::mutex moos_mutex_;

            extern boost::condition_variable command_received_cond_;
	    //            extern boost::condition_variable blackout_cond_;
	    //            extern boost::condition_variable operation_radius_cond_;
	    //            extern boost::condition_variable operation_box_cond_;
      
	    extern LAMSS_DEPLOY last_deploy_cmd_;    
	    extern LAMSS_PROSECUTE last_prosecute_cmd_;    
            
            extern int vehicle_id_;
	    extern std::string vehicle_name_;

	    extern bool command_received_;
	    extern bool mission_active_;
	    extern bool mission_requested_;
	    extern bool diving_active_;
	    extern bool diving_requested_;
	    extern bool deploy_received_;
	    extern bool deploy_mode_active_;
	    extern bool prosecute_received_;
	    extern bool prosecute_mode_active_;
	    extern bool surfacing_mode_active_;
	    extern bool surfacing_requested_;
	    extern bool abort_requested_;
	    extern bool abort_mode_active_;
	    extern bool drifting_requested_;
	    extern bool drifting_mode_active_;

            extern double last_acomms_;
	    extern double abort_time_;
	    extern double abort_duration_;

            extern boost::shared_ptr<LamssMissionManagerConfig> cfg_;
            
            // publish to the MOOSDB and the glog each time we change states
            template<Mission_StateEnum state>
                struct StateNotify
                {
                StateNotify() :
                    prefix_(std::string("MISSION_STATE_").length()),
                        state_str_(Mission_StateEnum_Name(state).substr(prefix_))
                        {
                            boost::mutex::scoped_lock lock(moos_mutex_);
                            goby::glog.is(goby::common::logger::VERBOSE) && goby::glog << group("fsm") <<  state_str_ << std::endl;
                            state_ = state;
                            
                            publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "MISSION_STATE", state_str_));   
                        }
                    virtual ~StateNotify()
                        {
                            goby::glog.is(goby::common::logger::VERBOSE) && goby::glog << group("fsm") << "~" << state_str_ << std::endl;
			}

                    const std::string::size_type prefix_;
                    const std::string state_str_;
                };
      
      
	    struct LamssMissionManagerFSM : boost::statechart::asynchronous_state_machine<LamssMissionManagerFSM, Idle >
	      {
              LamssMissionManagerFSM(my_context ctx) : my_base( ctx )
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
                }
	      };
	    
            // Idle
            struct Idle: boost::statechart::state<Idle, LamssMissionManagerFSM>,
	      StateNotify<MISSION_STATE_IDLE>
		{
		  Idle(my_context ctx) : my_base(ctx)
		  {		   
		    boost::mutex::scoped_lock lock(moos_mutex_);
		    mission_active_ = false; 
		    // wait for initial deploy command
		    while(!command_received_)
		      command_received_cond_.wait(lock);
                        
		    // Check for new state transitions
		    diving_requested_ = deploy_received_ || 
		      prosecute_received_ || 
		      abort_requested_ ||
		      drifting_requested_;
		    mission_requested_ = diving_requested_ || surfacing_requested_;

		    if (diving_requested_)
		      post_event(EvStartDiving());
		    
		  }
		  ~Idle() 
		    { 
		    }
		  
		  typedef boost::mpl::list<
		    boost::statechart::transition< EvStartDiving, Diving >
		    > reactions;
		  
		  
		};
	    
            // Mission
            struct Mission: boost::statechart::simple_state<Mission, LamssMissionManagerFSM, Diving>
		{
		  Mission() 
		    {
		      boost::mutex::scoped_lock lock(moos_mutex_);
		      mission_active_ = true;
		    }
		  ~Mission()
		    {
		      boost::mutex::scoped_lock lock(moos_mutex_);
		      mission_active_ = false;
		    }
		  
		  typedef boost::mpl::list<
                    boost::statechart::transition< EvAbortMission, Idle >,
                    boost::statechart::transition< EvNewMission, Idle >
                    > reactions;

		  
		};

            // /Mission/Diving
            struct Diving: boost::statechart::state<Diving, Mission>
		  {
		  Diving(my_context ctx) : my_base(ctx)
		    {
		      boost::mutex::scoped_lock lock(moos_mutex_);
		      diving_active_ = true;
		      if (deploy_received_)
			{
			  post_event(EvStartDeploy());
			  deploy_received_ = false;
			}
		      else if (prosecute_received_)
			{
			  post_event(EvStartProsecute());
			  prosecute_received_ = false;
			}
		      else if (abort_requested_)
			{
			  post_event(EvStartAbort());
			  abort_requested_ = false;
			}
		      else if (drifting_requested_)
			{
			  post_event(EvStartDrifting());
			  drifting_requested_ = false;
			}
		      else
			{
			  goby::glog << group("fsm") 
				     << "Unknown Mission State - Aborting" 
				     << std::endl;
			  post_event(EvAbortMission());
			  deploy_received_ = false;
			  prosecute_received_ = false;
			  abort_requested_ = true;
			}
		      
		    }
		    ~Diving() 
		      { 
			boost::mutex::scoped_lock lock(moos_mutex_);
			diving_active_ = false;
		      }
		    
		    typedef boost::mpl::list<
		      boost::statechart::transition< EvStartDeploy, Deploy  >,
		      boost::statechart::transition< EvStartProsecute, Prosecute >,
		      boost::statechart::transition< EvStartAbort, Abort >,
		      boost::statechart::transition< EvStartDrifting, Drifting >
		      > reactions;
       
		  };

            // /Mission/Deploy
            struct Deploy : boost::statechart::simple_state<Deploy, Mission >,
                StateNotify<MISSION_STATE_DEPLOY> 
            {
	      Deploy() 
                {

		  boost::mutex::scoped_lock lock(moos_mutex_);		  
		  deploy_mode_active_ = true;
#if 0
		  if (!cfg_->acomms())
		    {
		      // Publish what pAcommsHandler would for incoming
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "SONAR_CONTROL", 
							last_deploy_cmd_.sonar_control()));
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "SONAR_PROCESS", 
							last_deploy_cmd_.sonar_process()));
		      std::string comma = ",";
		      std::string hash = "#";
		      std::string clockwise;
		      if (last_deploy_cmd_.clockwise())
			clockwise = "true";
		      else
			clockwise = "false";
		      
		      std::string sval = "point=" +
			doubleToString(last_deploy_cmd_.deploy_x(),1) + comma +
			doubleToString(last_deploy_cmd_.deploy_y(),1);
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "ORBIT_RADIUS", 
							last_deploy_cmd_.loiter_radius()));
		      sval = "polygon=radial:" + 
			doubleToString(last_deploy_cmd_.deploy_x(),1) + comma +
			doubleToString(last_deploy_cmd_.deploy_y(),1) + comma +
			doubleToString(last_deploy_cmd_.loiter_radius(),1) + comma +
			intToString(last_deploy_cmd_.loiter_segments()) + hash +
			"clockwise=" + clockwise + hash +
			"speed=" + doubleToString(last_deploy_cmd_.deploy_speed(),1);
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "SENSOR_DEPLOY",sval));
		      sval = "depth="+doubleToString(last_deploy_cmd_.deploy_upper_depth());
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "SENSOR_DEPTH_DEPLOY",
							sval));
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, 
							"SENSOR_DEPTH_TRACKING", sval));
		      sval = "depth="+doubleToString(last_deploy_cmd_.deploy_lower_depth());
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, 
							"SENSOR_DEPTH_ALTERNATE", sval));
		      sval = "altitude="+doubleToString(last_deploy_cmd_.deploy_altitude());
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "SENSOR_ALTITUDE", 
							sval));
		      sval = "period=" + doubleToString(last_deploy_cmd_.gps_interval()); 
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "GPS_CONFIGURATION", 
							sval));
		      sval = "points=" + 
			doubleToString(last_deploy_cmd_.deploy_x(),1) + comma +
			doubleToString(last_deploy_cmd_.deploy_y(),1) + hash +
			"heading=" +
			doubleToString(last_deploy_cmd_.survey_heading(),1)+hash+
			"track_length=" + 
			doubleToString(last_deploy_cmd_.survey_length(),1) + hash +
			"track_width=" + 
			doubleToString(last_deploy_cmd_.survey_width(),1) + hash +
			"clockwise=" + clockwise + hash +
			"speed=" + doubleToString(last_deploy_cmd_.deploy_speed(),1);
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,
							"SENSOR_RACETRACK",sval));
		      sval = "min_depth=" + 
			doubleToString(last_deploy_cmd_.deploy_upper_depth(),1) + hash +
			"max_depth=" +
			doubleToString(last_deploy_cmd_.deploy_lower_depth(),1) ;
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "SENSOR_YOYO", sval));
		      sval = sval + hash + "period=" +
			doubleToString(last_deploy_cmd_.survey_period(),1) ;
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "SMART_YOYO", sval));
		      sval = "heading=" +
			doubleToString(last_deploy_cmd_.survey_heading(),1) ;
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,"SURVEY_HEADING",sval));
		      sval = "heading=" +
			doubleToString(last_deploy_cmd_.survey_heading(),1) + hash +
			"period=" +
			doubleToString(last_deploy_cmd_.survey_period(),1) + hash +
			"amplitude=" +
			doubleToString(last_deploy_cmd_.survey_width(),1) + hash +
			"speed=" + doubleToString(last_deploy_cmd_.deploy_speed(),1);
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,"ZIGZAG_CONFIG",sval));
		      sval = "points=" + 
			doubleToString(last_deploy_cmd_.deploy_x(),1) + comma +
			doubleToString(last_deploy_cmd_.deploy_y(),1) + comma +
			doubleToString(last_deploy_cmd_.survey_heading(),1) + comma +
			doubleToString(last_deploy_cmd_.survey_length(),1) + comma +
			doubleToString(last_deploy_cmd_.survey_period(),1) + comma +
			doubleToString(last_deploy_cmd_.survey_width(),1);
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,"ZIGZAG_STATUS",sval));
		      sval = "points=format=lawnmower" + comma + 
			"x=" + doubleToString(last_deploy_cmd_.deploy_x(),1) + comma +
			"y=" + doubleToString(last_deploy_cmd_.deploy_y(),1) + comma +
			"height=" + doubleToString(last_deploy_cmd_.survey_length(),1) +comma+
			"width=" + doubleToString(last_deploy_cmd_.survey_width(),1) + comma +
			"lane_width=" + 
			doubleToString(last_deploy_cmd_.survey_period(),1) + comma +
			"rows=east-west,degs=" + 
			doubleToString(last_deploy_cmd_.survey_heading(),1) + hash +
			"speed=" + doubleToString(last_deploy_cmd_.deploy_speed(),1);
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,"SURVEY_CONFIG",sval));
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,"GOTO_SURFACE","FALSE"));
		      switch (last_deploy_cmd_.trail_contact())
			{
			default:
			case LAMSS_DEPLOY::TRAIL_CONTACT_unicorn:
			  sval = "contact=unicorn#";
			  break;  
			case LAMSS_DEPLOY::TRAIL_CONTACT_macrura:
			  sval = "contact=macrura#" ; 
			  break;
			}
		      sval = sval + "trail_range=" +
			doubleToString(last_deploy_cmd_.trail_range(),1) + hash +
			"trail_angle=" +
			doubleToString(last_deploy_cmd_.trail_angle(),1) + hash ;
		      switch (last_deploy_cmd_.trail_angle_type())
			{
			default:
			case LAMSS_DEPLOY::TRAIL_ANGLE_TYPE_relative:
			  sval = sval + "trail_angle_type=absolute";
			  break;  
			case LAMSS_DEPLOY::TRAIL_ANGLE_TYPE_absolute:
			  sval = sval + "trail_angle_type=absolute";
			  break;
			}
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,"TRAIL_CONFIG",sval));
		      sval = "desired_bistatic_angle=" + 
			doubleToString(last_deploy_cmd_.trail_angle()) + hash +
			"normal_speed=" + doubleToString(last_deploy_cmd_.deploy_speed());
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,"BISTATIC_CONFIG",sval));
		      
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,"TOGGLE_UPPER_DEPTH",
							last_deploy_cmd_.deploy_upper_depth()));
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,"TOGGLE_LOWER_DEPTH",
							last_deploy_cmd_.deploy_lower_depth()));
		      sval = "spiral_center_x=" + 
			doubleToString(last_deploy_cmd_.deploy_x(),1) + hash +
			"spiral_center_y=" + 
			doubleToString(last_deploy_cmd_.deploy_y(),1) + hash +
			"spiral_start_radius=" + 
			doubleToString(last_deploy_cmd_.loiter_radius(),1) + hash +
			"spiral_angle=" + 
			doubleToString(last_deploy_cmd_.trail_angle(),1) + hash +
			"speed=" + doubleToString(last_deploy_cmd_.deploy_speed());
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,
							"DEPLOY_SPIRAL_UPDATE",sval));
		      sval = "speed=" + 
			doubleToString(last_deploy_cmd_.deploy_speed(),1) + hash +
			"helix_ctr_depth=" + 
			doubleToString(last_deploy_cmd_.deploy_upper_depth(),1) ;
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,
							"DEPLOY_TRACK_FRONT_UPDATE",sval));
		      sval = "helix_start_x=" + 
			doubleToString(last_deploy_cmd_.deploy_x(),1) + hash +
			"helix_start_y=" + 
			doubleToString(last_deploy_cmd_.deploy_y(),1) + hash +
			"helix_radius=" + 
			doubleToString(last_deploy_cmd_.loiter_radius(),1) + hash +
			"helix_constant=" + 
			doubleToString(last_deploy_cmd_.survey_period(),1) + hash +
			"helix_ctr_depth=" + 
			doubleToString(last_deploy_cmd_.deploy_upper_depth(),1) + hash +
			"helix_hdg=" + 
			doubleToString(last_deploy_cmd_.survey_heading(),1) + hash +
			"clockwise=" + clockwise + hash +
			"speed=" + doubleToString(last_deploy_cmd_.deploy_speed());
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,
							"DEPLOY_HELIX_UPDATE",sval));
		      sval = "tgt_x=" + doubleToString(last_deploy_cmd_.deploy_x(),1) + 
			comma +
			"tgt_y=" + doubleToString(last_deploy_cmd_.deploy_y(),1) ;
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,
							"TGT_TO_CLASS",sval));
		      sval = "tgt_x=" + doubleToString(last_deploy_cmd_.deploy_x(),1) + 
			hash +
			"tgt_y=" + doubleToString(last_deploy_cmd_.deploy_y(),1) + hash +
			"max_radius=" + 
			doubleToString(last_deploy_cmd_.loiter_radius(),1) + hash +
			"min_depth=" + 
			doubleToString(last_deploy_cmd_.deploy_upper_depth(),1) + hash +
			"max_depth=" +
			doubleToString(last_deploy_cmd_.deploy_lower_depth(),1) ;
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,
							"SAMPLE_UPDATE",sval));
		      
		      
		      
		      switch (last_deploy_cmd_.deploy_mode())
			{
			default:
			case LAMSS_DEPLOY::DEPLOY_MODE_LOITER:
			  sval = "LOITER"; 
			  break;
			case LAMSS_DEPLOY::DEPLOY_MODE_RETURN: 
			  sval = "RETURN"; 
			  break;
			case LAMSS_DEPLOY::DEPLOY_MODE_RACETRACK: 
			  sval = "RACETRACK"; 
			  break;
			case LAMSS_DEPLOY::DEPLOY_MODE_ZIGZAG: 
			  sval = "ZIGZAG"; 
			  break;
			case LAMSS_DEPLOY::DEPLOY_MODE_LAWNMOWER: 
			  sval = "LAWNMOWER"; 
			  break;
			case LAMSS_DEPLOY::DEPLOY_MODE_TRAIL:
			  sval = "TRAIL"; 
			  break;
			case LAMSS_DEPLOY::DEPLOY_MODE_STATION_KEEP: 
			  sval = "STATION_KEEP"; 
			  break;
			case LAMSS_DEPLOY::DEPLOY_MODE_SPIRAL:
			  sval = "SPIRAL"; 
			  break;
			case LAMSS_DEPLOY::DEPLOY_MODE_TRACK_FRONT:
			  sval = "TRACK_FRONT"; 
			  break;
			case LAMSS_DEPLOY::DEPLOY_MODE_SAMPLETARFIELD:
			  sval = "SAMPLETARFIELD"; 
			  break;
			case LAMSS_DEPLOY::DEPLOY_MODE_HELIX:
			  sval = "HELIX"; 
			  break;
			case LAMSS_DEPLOY::DEPLOY_MODE_DRIFT:
			  sval = "DRIFT"; 
			  break;
			}
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,"DEPLOY_MISSION",sval));
		      
		      switch (last_deploy_cmd_.depth_mode())
			{
			default:
			case LAMSS_DEPLOY::DEPTH_MODE_SINGLE:
			  sval = "SINGLE"; 
			  break;
			case LAMSS_DEPLOY::DEPTH_MODE_DUAL: 
			  sval = "DUAL"; 
			  break;
			case LAMSS_DEPLOY::DEPTH_MODE_YOYO: 
			  sval = "YOYO"; 
			  break;
			case LAMSS_DEPLOY::DEPTH_MODE_ADAPTIVE_YOYO: 
			  sval = "ADAPTIVE_YOYO"; 
			  break;
			case LAMSS_DEPLOY::DEPTH_MODE_TOGGLE_DEPTH_YOYO: 
			  sval = "TOGGLE_DEPTH_YOYO"; 
			  break;
			case LAMSS_DEPLOY::DEPTH_MODE_ADAPTIVE_SINGLE:
			  sval = "ADAPTIVE_SINGLE"; 
			  break;
			case LAMSS_DEPLOY::DEPTH_MODE_ADAPTIVE_ACOMMS: 
			  sval = "ADAPTIVE_ACOMMS"; 
			  break;
			case LAMSS_DEPLOY::DEPTH_MODE_BOTTOM_FOLLOWING:
			  sval = "BOTTOM_FOLLOWING"; 
			  break;
			case LAMSS_DEPLOY::DEPTH_MODE_HELIX:
			  sval = "HELIX"; 
			  break;
			}
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,"DEPTH_MODE",sval));
		    }
#endif		      
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Deploy Mode..."
			       << "deploy_duration=" << last_deploy_cmd_.deploy_duration() 
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
                }
	      ~Deploy() 
		{
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  deploy_mode_active_ = false;
		}
	      
            };
	    
	    // Prosecute states

            // /Mission/Prosecute
            struct Prosecute : boost::statechart::simple_state<Prosecute, Mission >,
                StateNotify<MISSION_STATE_PROSECUTE> 
            {
	      Prosecute() 
                {
		  
		  boost::mutex::scoped_lock lock(moos_mutex_);

		  prosecute_mode_active_ = true;
#if 0
		  if (!cfg_->acomms())
		    {
		      // Publish what pAcommsHandler would for incoming
		      std::string bearing_mode = "dual";
		      if (last_prosecute_cmd_.array_angle_mode() == 
			  LAMSS_PROSECUTE::ARRAY_ANGLE_MODE_quad)
			bearing_mode = "quad";
		      std::string bhv_update ="bearing_mode=" + bearing_mode +
			"#desired_angle=" + 
			doubleToString(last_prosecute_cmd_.array_angle(),1) ;
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "ARRAY_TRACK_ANGLE", 
							bhv_update));
		      
		      bhv_update = "TGT" + intToString(last_prosecute_cmd_.target_number());
		      
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "TARGET_NUMBER", 
							last_prosecute_cmd_.target_number()));
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "TARGET_ID", 
							bhv_update));
		      std::string comma = ",";
		      std::string sval = "tgt_id=" + 
			intToString(last_prosecute_cmd_.target_number()) + comma +
			"tgt_x=" + 
			doubleToString(last_prosecute_cmd_.target_x(),1) + comma +
			"tgt_y=" + 
			doubleToString(last_prosecute_cmd_.target_y(),1) + comma +
			"tgt_dep=" + 
			doubleToString(last_prosecute_cmd_.target_depth(),1) + comma +
			"tgt_hdg=" + 
			doubleToString(last_prosecute_cmd_.target_heading(),1) + comma +
			"tgt_spd=" + 
			doubleToString(last_prosecute_cmd_.target_speed(),1) + comma +
			"tgt_utc=" + 
			doubleToString(last_prosecute_cmd_.target_timestamp(),1) ;
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,
							"TARGET_CUE",sval));
		      
		      
		      bhv_update = "contact=" + bhv_update;
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "MY_TARGET", 
							bhv_update));
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "PRIORITY_TRK", 
							bhv_update));
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "CURRENT_TARGET", 
							bhv_update));
		      
		      
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "PROSECUTE_MISSION",
							"ADAPTIVE"));
		    }
#endif

		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Prosecute Mode..."
			       << "deploy_duration=" << last_deploy_cmd_.deploy_duration() 
			       << ", prosecute_duration=" << last_prosecute_cmd_.prosecute_duration() 
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
                }
	      ~Prosecute() 
		{
		  boost::mutex::scoped_lock lock(moos_mutex_);

		  prosecute_mode_active_ = false;
		}
	      
            };
	    
            // /Mission/Abort
            struct Abort : boost::statechart::simple_state<Abort, Mission >,
                StateNotify<MISSION_STATE_ABORT> 
            {
	      Abort() 
                {

		  boost::mutex::scoped_lock lock(moos_mutex_);		  
		  abort_mode_active_ = true;

		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Abort Mode..."
			       << "abort_duration=" << abort_duration_ 
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
                }
	      ~Abort() 
		{
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  abort_mode_active_ = false;
		}
	      
            };
	    
            // /Mission/Drifting
            struct Drifting : boost::statechart::simple_state<Drifting, Mission >,
                StateNotify<MISSION_STATE_DRIFTING> 
            {
	      Drifting() 
                {

		  boost::mutex::scoped_lock lock(moos_mutex_);		  
		  drifting_mode_active_ = true;

		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Drifting Mode..."
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
                }
	      ~Drifting() 
		{
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  drifting_mode_active_ = false;
		}
	      
            };
	    

        }
    }
}


#endif

