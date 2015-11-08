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
	    struct EvNewMission : boost::statechart::event< EvNewMission > { };
	    // Deploy events
            struct EvStartDeploy : boost::statechart::event< EvStartDeploy > { };
 	    // Prosecute events
            struct EvStartProsecute : boost::statechart::event< EvStartProsecute > { };
	    // Abort event
            struct EvStartAbort : boost::statechart::event< EvStartAbort > { };
	    // Drifting event
            struct EvStartDrifting : boost::statechart::event< EvStartDrifting > { };
	    // Surfacing event
            struct EvGotoSurface : boost::statechart::event< EvGotoSurface > { };
            struct EvEnableDiving : boost::statechart::event< EvEnableDiving > { };

	    // Homing events
            struct EvUpdateHeading : boost::statechart::event< EvUpdateHeading > { };
            struct EvResumeAbort : boost::statechart::event< EvResumeAbort > { };

            struct Idle;
            struct Mission;
            struct Diving;
            struct Surfacing;

	    // Deploy States
            struct Deploy;

	    // Prosecute States 
            struct Prosecute;
            struct Target;
            struct Homing;

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
      
	    extern LAMSS_TRANSIT last_transit_route_;    
	    extern LAMSS_DEPLOY last_deploy_cmd_;    
	    extern LAMSS_PROSECUTE last_prosecute_cmd_;    
            
            extern int vehicle_id_;
	    extern std::string vehicle_name_;

	    extern bool command_received_;
	    extern bool mission_active_;
	    extern bool diving_active_;
	    extern bool deploy_mode_active_;
	    extern bool prosecute_mode_active_;
	    extern bool surfacing_mode_active_;
	    extern bool abort_mode_active_;
	    extern bool drifting_mode_active_;

            extern double current_time_;
            extern double last_acomms_;
	    extern double abort_time_;
	    extern double abort_duration_;

	    extern bool homing_mode_active_;

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
                        
		  }
		  ~Idle() 
		    { 
		    }
		  
		  typedef boost::mpl::list<
		    boost::statechart::transition< EvStartDeploy, Deploy  >,
		    boost::statechart::transition< EvStartProsecute, Prosecute >,
		    boost::statechart::transition< EvStartAbort, Abort >,
		    boost::statechart::transition< EvStartDrifting, Drifting >
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

            // Mission/Diving
            struct Diving: boost::statechart::simple_state<Diving, Mission, Deploy>
		  {
		  Diving()
		    {
		      boost::mutex::scoped_lock lock(moos_mutex_);
		      diving_active_ = true;
		    }
		  ~Diving() 
		    { 
		      boost::mutex::scoped_lock lock(moos_mutex_);
		      diving_active_ = false;
		    }
		  
		  typedef boost::mpl::list<
                    boost::statechart::transition< EvGotoSurface, Surfacing >
                    > reactions;

		  };

            // Mission/Surfacing
            struct Surfacing: boost::statechart::simple_state<Surfacing, Mission>,
                StateNotify<MISSION_STATE_SURFACING> 
		  {
		  Surfacing()
		    {
		      boost::mutex::scoped_lock lock(moos_mutex_);
		      surfacing_mode_active_ = true;
		    }
		  ~Surfacing() 
		    { 
		      boost::mutex::scoped_lock lock(moos_mutex_);
		      surfacing_mode_active_ = false;
		    }
		  
		  typedef boost::mpl::list<
                    boost::statechart::transition< EvEnableDiving, Idle >
                    > reactions;

		  };

            // /Mission/Diving/Deploy
            struct Deploy : boost::statechart::simple_state<Deploy, Diving >,
                StateNotify<MISSION_STATE_DEPLOY> 
            {
	      Deploy() 
                {

		  boost::mutex::scoped_lock lock(moos_mutex_);		  
		  deploy_mode_active_ = true;
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

            // /Mission/Diving/Prosecute
            struct Prosecute : boost::statechart::simple_state<Prosecute, Diving, Target >,
                StateNotify<MISSION_STATE_PROSECUTE> 
            {
	      Prosecute() 
                {
		  
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  prosecute_mode_active_ = true;

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
	    
            // /Mission/Diving/Prosecute/Target
            struct Target : boost::statechart::simple_state<Target, Prosecute >
            {
	      Target() 
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Target Prosecute Mode..."
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
                }
	      ~Target() 
		{
		}
	      
            };
	    
            // /Mission/Diving/Prosecute/Homing
            struct Homing : boost::statechart::simple_state<Homing, Prosecute >
            {
	      Homing() 
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  homing_mode_active_ = true;
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,"HOMING_ACTIVE","true"));
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Homing Prosecute Mode..."
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
                }
	      ~Homing() 
		{
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  homing_mode_active_ = false;
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,"HOMING_ACTIVE","false"));
		}
	      
	      typedef boost::mpl::list<
		boost::statechart::transition< EvResumeAbort, Abort >
		> reactions;
            };
	    
            // /Mission/Diving/Abort
            struct Abort : boost::statechart::simple_state<Abort, Diving >,
                StateNotify<MISSION_STATE_ABORT> 
            {
	      Abort() 
                {

		  boost::mutex::scoped_lock lock(moos_mutex_);		  
		  abort_mode_active_ = true;
		  abort_time_ = current_time_;

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
	      
	      typedef boost::mpl::list<
		boost::statechart::transition< EvUpdateHeading, Homing >
		> reactions;

            };
	    
            // /Mission/Diving/Drifting
            struct Drifting : boost::statechart::simple_state<Drifting, Diving >,
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

