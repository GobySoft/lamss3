// Copyright 2014 
// Henrik Schmidt
// Toby Schneider 
// Massachusetts Institute of Technology 

#ifndef LAMSSMissionManagerFSM20150929H
#define LAMSSMissionManagerFSM20150929H

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

#include "goby/util/debug_logger.h"
#include "goby/moos/moos_protobuf_helpers.h"
#include "goby/middleware/protobuf/frontseat.pb.h"
#include "goby/middleware/frontseat/bluefin/bluefin.pb.h"
#include "goby/acomms/protobuf/amac.pb.h"

#include "MOOS/libMOOS/Comms/MOOSMsg.h"
#include "MBUtils.h"

#include "LamssMissionManager_config.pb.h"
#include "lamss/lib_lamss_protobuf/lamss_status.pb.h"
#include "lamss/lib_lamss_protobuf/lamss_deploy.pb.h" // DCCL message
#include "lamss/lib_lamss_protobuf/lamss_prosecute.pb.h" // DCCL message
#include "lamss/lib_lamss_protobuf/lamss_stovepipe.pb.h"

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
      // Return events
      struct EvReturnToBase : boost::statechart::event< EvReturnToBase > { };

      // Abort Homing events
      struct EvUpdateHeading : boost::statechart::event< EvUpdateHeading > { };
      struct EvResumeAbort : boost::statechart::event< EvResumeAbort > { };

      // Landing and takeoff events
      struct EvStartLanding : boost::statechart::event< EvStartLanding > { };
      struct EvTouchdown : boost::statechart::event< EvTouchdown > { };
      struct EvBottoming : boost::statechart::event< EvBottoming > { };
      struct EvTakeOff : boost::statechart::event< EvTakeOff > { };

	  
      struct Idle;
      struct Mission;
      struct Diving;
      struct Surfacing;

      // Deploy States
      struct Deploy;
      struct Survey;
      struct Return;
      struct Landing;

      // Prosecute States 
      struct Prosecute;
      struct Target;
      struct Homing;

      // Abort State
      struct Abort;

      // Drifting State
      struct Drifting;

      // Landing States
      struct Descent;
      struct Touchdown;
      struct Bottoming;
      struct LiftOff;


      extern std::deque<CMOOSMsg> publish_queue_;
      
      extern Mission_StateEnum state_;

      extern boost::mutex moos_mutex_;

      extern boost::condition_variable command_received_cond_;
      extern boost::condition_variable touchdown_altitude_cond_;
      extern boost::condition_variable touchdown_completed_cond_;
      extern boost::condition_variable operation_altitude_cond_;

      //            extern boost::condition_variable blackout_cond_;
      //            extern boost::condition_variable operation_radius_cond_;
      //            extern boost::condition_variable operation_box_cond_;
      
            
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
      extern bool landing_state_active_;
      extern bool descent_state_active_;
      extern bool touchdown_state_active_;
      extern bool bottoming_state_active_;
      extern bool liftoff_state_active_;
	  
      extern bool touchdown_altitude_reached_;
      extern bool touchdown_location_reached_;
      extern bool touchdown_completed_;
      extern bool operation_altitude_;

      extern double current_time_;
      extern double last_acomms_;
      extern double last_gps_;
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
	  goby::glog.is(goby::util::logger::VERBOSE) && goby::glog << group("fsm") <<  state_str_ << std::endl;
	  state_ = state;
                            
	  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "MISSION_STATE", state_str_));   
	}
	virtual ~StateNotify()
	{
	  boost::mutex::scoped_lock lock(moos_mutex_);
	  goby::glog.is(goby::util::logger::VERBOSE) && goby::glog 
	    << group("fsm") << "~" << state_str_ << std::endl;
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
				      StateNotify<MISSION_STATE_ALLSTOP>
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
      struct Deploy : boost::statechart::simple_state<Deploy, Diving, Survey >,
				      StateNotify<MISSION_STATE_DEPLOY> 
      {
	Deploy() 
	{

	  boost::mutex::scoped_lock lock(moos_mutex_);		  
	  deploy_mode_active_ = true;
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "Entering Deploy Mode..."
		       << std::endl;
		  
	  goby::glog.is(goby::util::logger::VERBOSE) && 
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

      // /Mission/Diving/Deploy/Survey
      struct Survey : boost::statechart::simple_state<Survey, Deploy >
      {
	Survey() 
	{
	  boost::mutex::scoped_lock lock(moos_mutex_);
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "Entering Survey Deploy Mode..."
		       << std::endl;
		  
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "success..." 
		       << std::endl;
	}
	~Survey() 
	{
	}

	typedef boost::mpl::list<
	  boost::statechart::transition< EvReturnToBase, Return  >,
	  boost::statechart::transition< EvStartLanding, Landing  >
	  > reactions;
	      
      };
	    
      // /Mission/Diving/Deploy/Return
      struct Return : boost::statechart::simple_state<Return, Deploy >
      {
	Return() 
	{
	  boost::mutex::scoped_lock lock(moos_mutex_);
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "Entering Return Deploy Mode..."
		       << std::endl;
		  
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "success..." 
		       << std::endl;
	}
	~Return() 
	{
	}
	      
      };
	  
      // /Mission/Diving/Deploy/Landing
      struct Landing : boost::statechart::simple_state<Landing, Deploy,
						       Descent >
      {
	Landing() 
	{
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "Entering Landing Deploy Mode..."
		       << std::endl;
		  
	  boost::mutex::scoped_lock lock(moos_mutex_);
	  landing_state_active_ = true;
		
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "success..." 
		       << std::endl;
	}
	~Landing() 
	{
	  boost::mutex::scoped_lock lock(moos_mutex_);
	  landing_state_active_ = false;
	}

      };
	    
      // /Mission/Diving/Deploy/Loiter/Descent
      struct Descent : boost::statechart::simple_state<Descent, Landing >
      {
	Descent() 
	{
	  boost::mutex::scoped_lock lock(moos_mutex_);
	  descent_state_active_ = true;
	  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,"LANDING_STATE",
					    "DESCENT"));
		  
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "Entering Landing:Descent Mode..."
		       << std::endl;
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "success..." 
		       << std::endl;
	}
	~Descent() 
	{
	  boost::mutex::scoped_lock lock(moos_mutex_);
	  descent_state_active_ = false;
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "Touchdown pending depth and location ..."
		       << std::endl;
	  // make sure we reached touchdown altitude
	  while(!touchdown_altitude_reached_ || !touchdown_location_reached_)
	    touchdown_altitude_cond_.wait(lock);
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "success..." 
		       << std::endl;
	}

	typedef boost::mpl::list<
	  boost::statechart::transition< EvTouchdown, Touchdown >
	  > reactions;
		  
      };
	    
      // /Mission/Diving/Deploy/Touchdown
      struct Touchdown : boost::statechart::simple_state<Touchdown, Landing >
      {
	Touchdown() 
	{
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "Entering Deploy:Touchdown Mode..."
		       << std::endl;
	      
	  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,
					    "LANDING_STATE",
					    "TOUCHDOWN"));
		  
	  boost::mutex::scoped_lock lock(moos_mutex_);
	  touchdown_state_active_ = true;
	  touchdown_completed_ = false;
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "success..." 
		       << std::endl;
	}
	~Touchdown() 
	{
	  boost::mutex::scoped_lock lock(moos_mutex_);
	  // make sure we reached touchdown altitude
	  while(!touchdown_completed_)
	    touchdown_completed_cond_.wait(lock);
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "completed ..." 
		       << std::endl;
	  touchdown_state_active_ = false;
	}
	    
	typedef boost::mpl::list<
	  boost::statechart::transition< EvBottoming, Bottoming >
	  > reactions;
		  
      };
	    
      // Mission/Diving/Deploy/Loiter/Bottoming
      struct Bottoming: boost::statechart::simple_state<Bottoming, Landing >
      {
	Bottoming()
	{
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "Entering Bottoming state ..."
		       << std::endl;
	  boost::mutex::scoped_lock lock(moos_mutex_);
	  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,
					    "LANDING_STATE",
					    "BOTTOMING"));
	  bottoming_state_active_ = true;
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "success..." 
		       << std::endl;
	      
	}
	~Bottoming() 
	{ 
	  boost::mutex::scoped_lock lock(moos_mutex_);
	  bottoming_state_active_ = false;
	}
	    
	typedef boost::mpl::list<
	  boost::statechart::transition< EvTakeOff, LiftOff >
	  > reactions;
	    
      };
	  
      // /Mission/Diving/Deploy/Loiter/LiftOff
      struct LiftOff : boost::statechart::simple_state<LiftOff, Landing >
      {
	LiftOff() 
	{
	    
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "LiftOff initiated, Waiting for kite and  anchor recovery..."
		       << std::endl;
	    
	    
	  boost::mutex::scoped_lock lock(moos_mutex_);

	  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,
					    "LANDING_STATE",
					    "LIFTOFF"));
	  liftoff_state_active_ = true;
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "success..." 
		       << std::endl;
	}
	~LiftOff() 
	{
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "Waiting for liftoff completion" 
		       << std::endl;
	  boost::mutex::scoped_lock lock(moos_mutex_);
	  // wait for vehicle to get off the bottom
	  while(!operation_altitude_)
	    operation_altitude_cond_.wait(lock);
	  liftoff_state_active_ = false;
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "success..." 
		       << std::endl;
	    
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

	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "Entering Prosecute Mode..."
		       << std::endl;
		  
	  goby::glog.is(goby::util::logger::VERBOSE) && 
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
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "Entering Target Prosecute Mode..."
		       << std::endl;
		  
	  goby::glog.is(goby::util::logger::VERBOSE) && 
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
	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "Entering Homing Prosecute Mode..."
		       << std::endl;
		  
	  goby::glog.is(goby::util::logger::VERBOSE) && 
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

	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "Entering Abort Mode..."
		       << "abort_duration=" << abort_duration_ 
		       << std::endl;
		  
	  goby::glog.is(goby::util::logger::VERBOSE) && 
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

	  goby::glog.is(goby::util::logger::VERBOSE) && 
	    goby::glog << group("fsm") 
		       << "Entering Drifting Mode..."
		       << std::endl;
		  
	  goby::glog.is(goby::util::logger::VERBOSE) && 
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

