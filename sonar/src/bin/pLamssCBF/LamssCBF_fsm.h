// Copyright 2015 
// Henrik Schmidt
// Massachusetts Institute of Technology 

#ifndef LAMSSCBF_FSM20151106H
#define LAMSSBBF_FSM20151106H

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
//#include "goby/moos/protobuf/frontseat.pb.h"
//#include "goby/moos/frontseat/bluefin/bluefin.pb.h"
#include "goby/acomms/protobuf/amac.pb.h"

#include "MOOS/libMOOS/Comms/MOOSMsg.h"

#include "LamssCBF_config.pb.h"
#include "BeamformingVars.pb.h"

//#include "lamss/lib_lamss_protobuf/lamss_status.pb.h"
//#include "lamss/lib_lamss_protobuf/lamss_deploy.pb.h" // DCCL message
//#include "lamss/lib_lamss_protobuf/lamss_prosecute.pb.h" // DCCL message
//#include "lamss/lib_lamss_protobuf/lamss_contact.pb.h" // DCCL message
//#include "lamss/lib_lamss_protobuf/lamss_track.pb.h" // DCCL message

namespace lamss
{
  namespace shared
  {
    namespace fsm
    {
      
      // events
      
      struct EvProcessingOff : boost::statechart::event< EvProcessingOff > { };
      struct EvProcessingReset : boost::statechart::event< EvProcessingReset > { };
      struct EvToBroadband : boost::statechart::event< EvToBroadband > { };
      struct EvToTones : boost::statechart::event< EvToTones > { };
      
      struct Idle;
      // Processing States
      struct Processing;
      struct Broadband;
      struct Tones;
      
      extern std::deque<CMOOSMsg> publish_queue_;
      
      extern ProcessingStateEnum state_;
      
      extern boost::mutex moos_mutex_;
      
      extern boost::condition_variable processing_cond_;
      
      /*
      extern LAMSS_DEPLOY last_deploy_cmd_;    
      extern LAMSS_PROSECUTE last_prosecute_cmd_;    
      */

      extern double current_time_;
      extern bool processing_active_;
      
      extern bool processing_enable_;
      extern bool processing_idle_;
      extern bool processing_active_;
      // Detection/Search state vars
      extern bool broadband_active_;
      extern double broadband_start_;
      // Tones State vars
      extern bool tones_active_;
      extern double tones_start_;

      // Mission state vars
      /*
      extern bool deploy_received_;
      extern bool deploy_mode_active_;
      extern bool prosecute_received_;
      extern bool prosecute_mode_active_;
      */

      extern boost::shared_ptr<LamssCBFConfig> cfg_;
      
      // publish to the MOOSDB and the glog each time we change states
      template<ProcessingStateEnum state>
	struct StateNotify
	{
	StateNotify() :
	  prefix_(std::string("PROCESSING_STATE_").length()),
	    state_str_(ProcessingStateEnum_Name(state).substr(prefix_))
	  {
	    boost::mutex::scoped_lock lock(moos_mutex_);
	    goby::glog.is(goby::common::logger::VERBOSE) && 
	      goby::glog << group("fsm") <<  
	      state_str_ << std::endl;
	    state_ = state;
            
	    publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "PROCESSING_STATE", 
					      state_str_));   
	  }
	  virtual ~StateNotify()
	  {
	    goby::glog.is(goby::common::logger::VERBOSE) && 
	      goby::glog << group("fsm") << "~" << 
	      state_str_ << std::endl;
	  }
	  
	  const std::string::size_type prefix_;
	  const std::string state_str_;
	};
      
      
      struct LamssCBFFSM : boost::statechart::asynchronous_state_machine<LamssCBFFSM, Idle >
	{
	LamssCBFFSM(my_context ctx) : my_base( ctx )
	  {
	  }
	};
      
      // Idle
      struct Idle: boost::statechart::simple_state<Idle, LamssCBFFSM>,
	StateNotify<PROCESSING_STATE_IDLE>
	  {
	    Idle()
	      {		   
		boost::mutex::scoped_lock lock(moos_mutex_);
		// wait for initial deploy command
		processing_idle_ = true; 
		
		while(!processing_enable_)
		  processing_cond_.wait(lock);
		
	      }
	    ~Idle() 
	      { 
		boost::mutex::scoped_lock lock(moos_mutex_);
		processing_idle_ = false; 
	      }
	    
	    typedef boost::mpl::list<
	      boost::statechart::transition< EvToBroadband, Broadband >,
	      boost::statechart::transition< EvToTones, Tones >
	      > reactions;
	    
	    
	  };
      
      // Processing
      struct Processing: boost::statechart::simple_state<Processing, 
	LamssCBFFSM , Broadband >,
	StateNotify<PROCESSING_STATE_PROCESSING> 
	  {
	    Processing()
	      {
		boost::mutex::scoped_lock lock(moos_mutex_);
		processing_active_ = true;
	      }
	    
	    ~Processing() 
	      { 
		boost::mutex::scoped_lock lock(moos_mutex_);
		processing_active_ = false;
	      }
	    
	    typedef boost::mpl::list<
	      boost::statechart::transition< EvProcessingOff, Idle  >,
	      boost::statechart::transition< EvProcessingReset, Idle >
	      > reactions;
	    
	  };
      
      // /Processing/Broadband
      struct Broadband: boost::statechart::simple_state<Broadband, Processing>,
	StateNotify<PROCESSING_STATE_BROADBAND>
	  {
	    Broadband() 
	      {
		broadband_active_ = true;
		broadband_start_ = current_time_;
		publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,"IN_RANGE","FALSE"));
	      }
	    ~Broadband()
	      {
		broadband_active_ = false;
	      }
	    
	  };
      
      // /Processing/Tones
      struct Tones: boost::statechart::simple_state<Tones, Processing>,
	StateNotify<PROCESSING_STATE_TONES>
	  {
	    Tones() 
	      {
		boost::mutex::scoped_lock lock(moos_mutex_);
		tones_active_ = true;
		tones_start_ = current_time_;
	      }
	    ~Tones()
	      {
		boost::mutex::scoped_lock lock(moos_mutex_);
		tones_active_ = false;
	      }
	  };
      
    }
  }
}


#endif

