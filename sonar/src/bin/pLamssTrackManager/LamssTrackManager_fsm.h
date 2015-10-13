// Copyright 2015 
// Henrik Schmidt
// Massachusetts Institute of Technology 

#ifndef LAMSSTrackManagerFSM20151006H
#define LAMSSTrackManagerFSM20151006H

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

#include "LamssTrackManager_config.pb.h"
#include "lamss/lib_lamss_protobuf/lamss_status.pb.h"
#include "lamss/lib_lamss_protobuf/lamss_deploy.pb.h" // DCCL message
#include "lamss/lib_lamss_protobuf/lamss_prosecute.pb.h" // DCCL message
#include "lamss/lib_lamss_protobuf/lamss_contact.pb.h" // DCCL message

namespace lamss
{
    namespace shared
    {
        namespace fsm
        {
            
            // events
    
            struct EvDcltOff : boost::statechart::event< EvDcltOff > { };
            struct EvDcltReset : boost::statechart::event< EvDcltReset > { };
	    struct EvDcltStart : boost::statechart::event< EvDcltStart > { };
	    struct EvCueInRange : boost::statechart::event< EvCueInRange > { };
	    struct EvSearchOrbit : boost::statechart::event< EvSearchOrbit > { };
	    struct EvToLocalization : boost::statechart::event< EvToLocalization > { };
	    // Tracking Sequence Events
            struct EvToTracking : boost::statechart::event< EvToTracking > { };
 	    // Prosecute events
            struct EvToClassifying : boost::statechart::event< EvToClassifying > { };

            struct Idle;
	    // DCLT States
	    struct Dclt;
            struct Detection;
	    // detection search substates
	    struct Search;
	    struct CueInRange;
	    struct SearchOrbit;

            struct Localization;
            struct Tracking;
            struct Classifying;

            extern std::deque<CMOOSMsg> publish_queue_;
      
            extern BearingTrackEnum state_;

            extern boost::mutex moos_mutex_;

	    extern LAMSS_DEPLOY last_deploy_cmd_;    
	    extern LAMSS_PROSECUTE last_prosecute_cmd_;    
            
            extern int vehicle_id_;

	    extern double current_time_;
	    extern bool mission_active_;
	    extern int  track_state_;

	    extern bool dclt_idle_;
	    extern bool dclt_active_;
	    // Detection/Search state vars
	    extern bool detection_active_;
	    extern double detection_start_;
	    extern bool cue_in_range_;
	    extern bool cue_too_close_;
	    // Localization State vars
	    extern bool localization_active_;
	    extern double localization_start_;
	    // Tracking State vars
	    extern bool tracking_active_;
	    extern double tracking_start_;
	    extern double tracking_duration_;
	    extern int bearing_track_count_;
	    extern int geo_track_count_;
	    extern int track_report_count_;
	    // Classifying State vars
	    extern bool classifying_active_;
	    extern double classifying_start_;
	    // Mission state vars
	    extern bool deploy_received_;
	    extern bool deploy_mode_active_;
	    extern bool prosecute_received_;
	    extern bool prosecute_mode_active_;

            extern boost::shared_ptr<LamssTrackManagerConfig> cfg_;
            
            // publish to the MOOSDB and the glog each time we change states
            template<BearingTrackEnum state>
                struct StateNotify
                {
                StateNotify() :
                    prefix_(std::string("TRACKING_").length()),
                        state_str_(BearingTrackEnum_Name(state).substr(prefix_))
                        {
                            boost::mutex::scoped_lock lock(moos_mutex_);
                            goby::glog.is(goby::common::logger::VERBOSE) && 
			      goby::glog << group("fsm") <<  
			      state_str_ << std::endl;
                            state_ = state;
                            
                            publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "TRACKING", 
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
      
      
	    struct LamssTrackManagerFSM : boost::statechart::asynchronous_state_machine<LamssTrackManagerFSM, Idle >
	      {
              LamssTrackManagerFSM(my_context ctx) : my_base( ctx )
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
                }
	      };
	    
            // Idle
            struct Idle: boost::statechart::simple_state<Idle, LamssTrackManagerFSM>,
	      StateNotify<TRACKING_IDLE>
		{
		  Idle()
		  {		   
		    boost::mutex::scoped_lock lock(moos_mutex_);
		    dclt_idle_ = true; 
		  }
		  ~Idle() 
		    { 
		    boost::mutex::scoped_lock lock(moos_mutex_);
		    dclt_idle_ = false; 
		    }
		  
		  typedef boost::mpl::list<
		    boost::statechart::transition< EvDcltStart, Detection >
		    > reactions;
		  
		  
		};
	    
            // Dclt
            struct Dclt: boost::statechart::simple_state<Dclt, 
	      LamssTrackManagerFSM , Detection >,
                StateNotify<TRACKING_DCLT> 
		  {
		  Dclt()
		    {
		      boost::mutex::scoped_lock lock(moos_mutex_);
		      dclt_active_ = true;
		    }
		      
		  ~Dclt() 
		    { 
		      boost::mutex::scoped_lock lock(moos_mutex_);
		      dclt_active_ = false;
		    }
		    
		    typedef boost::mpl::list<
		      boost::statechart::transition< EvDcltOff, Idle  >,
		      boost::statechart::transition< EvDcltReset, Detection >
		      > reactions;
       
		  };

            // /Dclt/Detection
            struct Detection: boost::statechart::simple_state<Detection, Dclt, Search>,
	      StateNotify<TRACKING_NO_TRACK>
		{
		  Detection() 
		    {
		      detection_active_ = true;
		      detection_start_ = current_time_;
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,"IN_RANGE","FALSE"));
		    }
		  ~Detection()
		    {
		      detection_active_ = false;
		    }
		  
		  typedef boost::mpl::list<
                    boost::statechart::transition< EvToLocalization, Localization >
                    > reactions;
		};

            // /Dclt/Detection/Search
            struct Search: boost::statechart::simple_state<Search, Detection>
		{
		  Search() 
		    {
		      boost::mutex::scoped_lock lock(moos_mutex_);
		      cue_in_range_ = false;
		    }
		  ~Search()
		    {
		      boost::mutex::scoped_lock lock(moos_mutex_);
		    }
		  typedef boost::mpl::list<
		    boost::statechart::transition< EvCueInRange, CueInRange >
                    > reactions;
		};
		 
            // /Dclt/Detection/CueInRange
            struct CueInRange: boost::statechart::simple_state<CueInRange, Detection>
		{
		  CueInRange() 
		    {
		      boost::mutex::scoped_lock lock(moos_mutex_);
		      cue_in_range_ = true;
		      goby::glog.is(goby::common::logger::VERBOSE) && goby::glog 
			<< group("fsm") << "Target Cue is inside range" << std::endl;
		    }
		  ~CueInRange()
		    {
		      boost::mutex::scoped_lock lock(moos_mutex_);
		      cue_in_range_ = false;
		    }
		  typedef boost::mpl::list<
		    boost::statechart::transition< EvSearchOrbit, SearchOrbit >
                    > reactions;
		};
		 
            // /Dclt/Detection/SearchOrbit
            struct SearchOrbit: boost::statechart::simple_state<SearchOrbit, Detection>
		{
		  SearchOrbit() 
		    {
		      boost::mutex::scoped_lock lock(moos_mutex_);
		      cue_too_close_ = true;
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,"IN_RANGE","TRUE"));
		    }
		  ~SearchOrbit()
		    {
		      boost::mutex::scoped_lock lock(moos_mutex_);
		      cue_too_close_ = false;
		      publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY,"IN_RANGE","FALSE"));
		    }
		};

            // /Dclt/Localization
            struct Localization: boost::statechart::simple_state<Localization, Dclt>,
	      StateNotify<TRACKING_AMBIGUOUS>
		{
		  Localization() 
		    {
		      boost::mutex::scoped_lock lock(moos_mutex_);
		      localization_active_ = true;
		      localization_start_ = current_time_;
		    }
		  ~Localization()
		    {
		      boost::mutex::scoped_lock lock(moos_mutex_);
		      localization_active_ = false;
		    }
		  
		  typedef boost::mpl::list<
                    boost::statechart::transition< EvToTracking, Tracking >
                    > reactions;
		};

            // /Dclt/Tracking
            struct Tracking : boost::statechart::simple_state<Tracking, Dclt >,
                StateNotify<TRACKING_TRACKING> 
            {
	      Tracking() 
                {

		  boost::mutex::scoped_lock lock(moos_mutex_);		  
		  tracking_active_ = true;
		  tracking_start_ = current_time_;
		  geo_track_count_= 0;
		  track_report_count_= 0;
                }
	      ~Tracking() 
		{
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  tracking_active_ = false;
		}
	      
	      typedef boost::mpl::list<
		boost::statechart::transition< EvToClassifying, Classifying >
		> reactions;
            };
	    

            // /Dclt/Classifying
            struct Classifying : boost::statechart::simple_state<Classifying, Dclt >,
                StateNotify<TRACKING_CLASSIFYING> 
            {
	      Classifying() 
                {
		  
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  classifying_active_ = true;
		  classifying_start_ = current_time_;
                }
	      ~Classifying() 
		{
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  classifying_active_ = false;
		}
	      
            };
	    

        }
    }
}


#endif

