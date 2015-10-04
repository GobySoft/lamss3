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
            struct EvStartMission : boost::statechart::event< EvStartMission > { };
            struct EvStartDive : boost::statechart::event< EvStartDive > { };
	    // Deploy events
            struct EvStartDeploy : boost::statechart::event< EvStartDeploy > { };
            struct EvDeployLoiter : boost::statechart::event< EvDeployLoiter > { };
            struct EvDeployRacetrack : boost::statechart::event< EvDeployRacetrack > { };
            struct EvDeployZigzag : boost::statechart::event< EvDeployZigzag > { };
            struct EvDeployLawnmower : boost::statechart::event< EvDeployLawnmower > { };
            struct EvDeployStationkeep : boost::statechart::event< EvDeployStationkeep > { };
            struct EvDeployTrail : boost::statechart::event< EvDeployTrail > { };
            struct EvDeployReturn : boost::statechart::event< EvDeployReturn > { };
            struct EvDeploySurface : boost::statechart::event< EvDeploySurface > { };
            struct EvDeploySpiral : boost::statechart::event< EvDeploySpiral > { };
            struct EvDeploySampletarfield : boost::statechart::event< EvDeploySampletarfield > { };
            struct EvDeployTrackfront : boost::statechart::event< EvDeployTrackfront > { };
            struct EvDeployHelix : boost::statechart::event< EvDeployHelix > { };
            struct EvDeployUnknown : boost::statechart::event< EvDeployUnknown > { };
            struct EvNewMission : boost::statechart::event< EvNewMission > { };
	    // Prosecute events
            struct EvStartProsecute : boost::statechart::event< EvStartProsecute > { };
            struct EvProsecuteAdaptive : boost::statechart::event< EvProsecuteAdaptive > { };
            struct EvProsecuteTmazigzag : boost::statechart::event< EvProsecuteTmazigzag > { };
            struct EvProsecutePolygon : boost::statechart::event< EvProsecutePolygon > { };
            struct EvProsecuteUnknown : boost::statechart::event< EvProsecuteUnknown > { };
	    // Abort event
            struct EvStartAbort : boost::statechart::event< EvStartAbort > { };

            struct Idle;
            struct Mission;
            struct StartMission;
	    // Deploy States
            struct Deploy;
            struct StartDeploy;
            struct Loiter;
            struct Racetrack;
            struct Zigzag;
            struct Lawnmower;
            struct Stationkeep;
            struct Trail;
            struct Spiral;
            struct Trackfront;
            struct Sampletarfield;
            struct Helix;
            struct Return;
	    struct Surface;
	    struct UnknownDeploy;


	    // Prosecute States 
            struct Prosecute;
            struct StartProsecute;
            struct Adaptive;
            struct Tmazigzag;
            struct Polygon;
	    struct UnknownProsecute;

	    // Abort State
            struct Abort;

            extern std::deque<CMOOSMsg> publish_queue_;
      
            extern Lamss_StateEnum state_;

            extern boost::mutex moos_mutex_;

	    //            extern boost::condition_variable blackout_cond_;
	    //            extern boost::condition_variable operation_radius_cond_;
	    //            extern boost::condition_variable operation_box_cond_;
      
	    extern LAMSS_DEPLOY last_deploy_cmd_;    
	    extern LAMSS_PROSECUTE last_prosecute_cmd_;    
            
            extern int vehicle_id_;

	    extern bool mission_active_;
	    extern bool deploy_received_;
	    extern bool deploy_mode_active_;
	    extern bool prosecute_received_;
	    extern bool prosecute_mode_active_;
	    extern bool surfacing_mode_active_;
	    extern bool abort_requested_;
	    extern bool abort_mode_active_;

            extern double last_acomms_;
            extern double prosecute_time_;
	    extern double abort_time_;
	    extern double abort_duration_;

            extern boost::shared_ptr<LamssMissionManagerConfig> cfg_;
            
            // publish to the MOOSDB and the glog each time we change states
            template<Lamss_StateEnum state>
                struct StateNotify
                {
                StateNotify() :
                    prefix_(std::string("LAMSS_STATE_").length()),
                        state_str_(Lamss_StateEnum_Name(state).substr(prefix_))
                        {
                            boost::mutex::scoped_lock lock(moos_mutex_);
                            goby::glog.is(goby::common::logger::VERBOSE) && goby::glog << group("fsm") <<  state_str_ << std::endl;
                            state_ = state;
                            
			    //                            publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "LAMSS_STATE", state_str_));   
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
	      StateNotify<LAMSS_STATE_IDLE>
		{
		  Idle(my_context ctx) : my_base(ctx)
		  {		   
		    boost::mutex::scoped_lock lock(moos_mutex_);
		    mission_active_ = false; 
		    if (abort_requested_ || deploy_received_ || prosecute_received_)
		      post_event(EvStartMission());
		    
		  }
		  ~Idle() 
		    { 
		    }
		  
		  typedef boost::mpl::list<
		    boost::statechart::transition< EvStartMission, Mission >
		    > reactions;
		  
		  
		};
	    
            // Mission
            struct Mission: boost::statechart::simple_state<Mission, LamssMissionManagerFSM, StartMission>,
	      StateNotify<LAMSS_STATE_ACTIVE>
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

            // /Mission/StartMission
            struct StartMission: boost::statechart::state<StartMission, Mission>,
                StateNotify<LAMSS_STATE_STARTING> 
		  {
		  StartMission(my_context ctx) : my_base(ctx)
		    {
		  boost::mutex::scoped_lock lock(moos_mutex_);
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
                ~StartMission() { }

                typedef boost::mpl::list<
		  boost::statechart::transition< EvStartDeploy, Deploy  >,
		  boost::statechart::transition< EvStartProsecute, Prosecute >,
		  boost::statechart::transition< EvStartAbort, Abort >
                    > reactions;
       
            };

            // /Mission/Deploy
            struct Deploy : boost::statechart::simple_state<Deploy, Mission, StartDeploy>,
                StateNotify<LAMSS_STATE_DEPLOY> 
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
		  
		  std::string serialized;
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, 
						    "MISSION_STATE", "DEPLOY"));
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
	    
            // /Mission/Deploy/StartDeploy
            struct StartDeploy: boost::statechart::state<StartDeploy, Deploy>,
                StateNotify<LAMSS_STATE_START_DEPLOY> 
            {
                StartDeploy(my_context ctx) : my_base(ctx)
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  switch (last_deploy_cmd_.deploy_mode())
		    {	  
		    case LAMSS_DEPLOY::DEPLOY_MODE_LOITER:
		    post_event(EvDeployLoiter());
		    break;

		    case LAMSS_DEPLOY::DEPLOY_MODE_RETURN:
		    post_event(EvDeployReturn());
		    break;

		    case LAMSS_DEPLOY::DEPLOY_MODE_RACETRACK:
		    post_event(EvDeployRacetrack());
		    break;

		    case LAMSS_DEPLOY::DEPLOY_MODE_ZIGZAG:
		    post_event(EvDeployZigzag());
		    break;

		    case LAMSS_DEPLOY::DEPLOY_MODE_LAWNMOWER:
		    post_event(EvDeployLawnmower());
		    break;

		    case LAMSS_DEPLOY::DEPLOY_MODE_TRAIL:
		    post_event(EvDeployTrail());
		    break;

		    case LAMSS_DEPLOY::DEPLOY_MODE_STATION_KEEP:
		    post_event(EvDeployStationkeep());
		    break;

		    case LAMSS_DEPLOY::DEPLOY_MODE_SPIRAL:
		    post_event(EvDeploySpiral());
		    break;

		    case LAMSS_DEPLOY::DEPLOY_MODE_TRACK_FRONT:
		    post_event(EvDeployTrackfront());
		    break;

		    case LAMSS_DEPLOY::DEPLOY_MODE_SAMPLETARFIELD:
		    post_event(EvDeploySampletarfield());
		    break;

		    case LAMSS_DEPLOY::DEPLOY_MODE_HELIX:
		    post_event(EvDeployHelix());
		    break;

		    default:
		    post_event(EvDeployUnknown());
		    break;

		    }
                }
                ~StartDeploy() { }

                typedef boost::mpl::list<
		  boost::statechart::transition< EvDeployLoiter, Loiter  >,
		  boost::statechart::transition< EvDeployReturn, Return >,
		  boost::statechart::transition< EvDeployRacetrack, Racetrack >,
		  boost::statechart::transition< EvDeployZigzag, Zigzag >,
		  boost::statechart::transition< EvDeployLawnmower, Lawnmower >,
		  boost::statechart::transition< EvDeployStationkeep, Stationkeep >,
		  boost::statechart::transition< EvDeployTrail, Trail >,
		  boost::statechart::transition< EvDeploySpiral, Spiral >,
		  boost::statechart::transition< EvDeployTrackfront, Trackfront >,
		  boost::statechart::transition< EvDeploySampletarfield, Sampletarfield >,
		  boost::statechart::transition< EvDeployHelix,Helix >,
		  boost::statechart::transition< EvDeployUnknown,UnknownDeploy >
                    > reactions;
       
            };

            // /Mission/Deploy/Loiter
            struct Loiter: boost::statechart::simple_state<Loiter, Deploy>,
                StateNotify<LAMSS_STATE_DEPLOY_LOITER>  
            {
	      Loiter()
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "DEPLOY_MISSION", "LOITER"));
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Deploy Loiter Mode..." 
			       << "Deploy mode=" << last_deploy_cmd_.deploy_mode()
			       << ", Deploy_x = " << last_deploy_cmd_.deploy_x() 
			       << ", Deploy_y = " << last_deploy_cmd_.deploy_y() 
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
		  
                }
	      ~Loiter() 
                {
                }

            };

            // /Mission/Deploy/Return
            struct Return: boost::statechart::simple_state<Return, Deploy>,
                StateNotify<LAMSS_STATE_DEPLOY_RETURN>  
            {
	      Return()
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "DEPLOY_MISSION", "RETURN"));
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Deploy Return Mode..." 
			       << "Deploy mode=" << last_deploy_cmd_.deploy_mode()
			       << ", Deploy_x = " << last_deploy_cmd_.deploy_x() 
			       << ", Deploy_y = " << last_deploy_cmd_.deploy_y() 
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
		  
                }
	      ~Return() 
                {
                }

            };


            // /Mission/Deploy/Racetrack
            struct Racetrack: boost::statechart::simple_state<Racetrack, Deploy>,
                StateNotify<LAMSS_STATE_DEPLOY_RACETRACK>  
            {
	      Racetrack()
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "DEPLOY_MISSION", "RACETRACK"));
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Deploy Racetrack Mode..." 
			       << "Deploy mode=" << last_deploy_cmd_.deploy_mode()
			       << ", Deploy_x = " << last_deploy_cmd_.deploy_x() 
			       << ", Deploy_y = " << last_deploy_cmd_.deploy_y() 
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
		  
                }
	      ~Racetrack() 
                {
                }

            };

            // /Mission/Deploy/Zigzag
            struct Zigzag: boost::statechart::simple_state<Zigzag, Deploy>,
                StateNotify<LAMSS_STATE_DEPLOY_ZIGZAG>  
            {
	      Zigzag()
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "DEPLOY_MISSION", "ZIGZAG"));
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Deploy Zigzag Mode..." 
			       << "Deploy mode=" << last_deploy_cmd_.deploy_mode()
			       << ", Deploy_x = " << last_deploy_cmd_.deploy_x() 
			       << ", Deploy_y = " << last_deploy_cmd_.deploy_y() 
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
		  
                }
	      ~Zigzag() 
                {
                }

            };

            // /Mission/Deploy/Lawnmower
            struct Lawnmower: boost::statechart::simple_state<Lawnmower, Deploy>,
                StateNotify<LAMSS_STATE_DEPLOY_LAWNMOWER>  
            {
	      Lawnmower()
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "DEPLOY_MISSION", "LAWNMOWER"));
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Deploy Lawnmower Mode..." 
			       << "Deploy mode=" << last_deploy_cmd_.deploy_mode()
			       << ", Deploy_x = " << last_deploy_cmd_.deploy_x() 
			       << ", Deploy_y = " << last_deploy_cmd_.deploy_y() 
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
		  
                }
	      ~Lawnmower() 
                {
                }

            };

            // /Mission/Deploy/Trail
            struct Trail: boost::statechart::simple_state<Trail, Deploy>,
                StateNotify<LAMSS_STATE_DEPLOY_TRAIL>  
            {
	      Trail()
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "DEPLOY_MISSION", "TRAIL"));
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Deploy Trail Mode..." 
			       << "Deploy mode=" << last_deploy_cmd_.deploy_mode()
			       << ", Deploy_x = " << last_deploy_cmd_.deploy_x() 
			       << ", Deploy_y = " << last_deploy_cmd_.deploy_y() 
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
		  
                }
	      ~Trail() 
                {
                }

            };

            // /Mission/Deploy/Stationkeep
            struct Stationkeep: boost::statechart::simple_state<Stationkeep, Deploy>,
                StateNotify<LAMSS_STATE_DEPLOY_STATIONKEEP>  
            {
	      Stationkeep()
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "DEPLOY_MISSION", "STATIONKEEP"));
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Deploy Stationkeep Mode..." 
			       << "Deploy mode=" << last_deploy_cmd_.deploy_mode()
			       << ", Deploy_x = " << last_deploy_cmd_.deploy_x() 
			       << ", Deploy_y = " << last_deploy_cmd_.deploy_y() 
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
		  
                }
	      ~Stationkeep() 
                {
                }

            };

            // /Mission/Deploy/Spiral
            struct Spiral: boost::statechart::simple_state<Spiral, Deploy>,
                StateNotify<LAMSS_STATE_DEPLOY_SPIRAL>  
            {
	      Spiral()
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "DEPLOY_MISSION", "SPIRAL"));
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Deploy Spiral Mode..." 
			       << "Deploy mode=" << last_deploy_cmd_.deploy_mode()
			       << ", Deploy_x = " << last_deploy_cmd_.deploy_x() 
			       << ", Deploy_y = " << last_deploy_cmd_.deploy_y() 
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
		  
                }
	      ~Spiral() 
                {
                }

            };

            // /Mission/Deploy/Trackfront
            struct Trackfront: boost::statechart::simple_state<Trackfront, Deploy>,
                StateNotify<LAMSS_STATE_DEPLOY_TRACKFRONT>  
            {
	      Trackfront()
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "DEPLOY_MISSION", "TRACKFRONT"));
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Deploy Trackfront Mode..." 
			       << "Deploy mode=" << last_deploy_cmd_.deploy_mode()
			       << ", Deploy_x = " << last_deploy_cmd_.deploy_x() 
			       << ", Deploy_y = " << last_deploy_cmd_.deploy_y() 
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
		  
                }
	      ~Trackfront() 
                {
                }

            };

            // /Mission/Deploy/Sampletarfield
            struct Sampletarfield: boost::statechart::simple_state<Sampletarfield, Deploy>,
                StateNotify<LAMSS_STATE_DEPLOY_SAMPLETARFIELD>  
            {
	      Sampletarfield()
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "DEPLOY_MISSION", "SAMPLETARFIELD"));
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Deploy Sampletarfield Mode..." 
			       << "Deploy mode=" << last_deploy_cmd_.deploy_mode()
			       << ", Deploy_x = " << last_deploy_cmd_.deploy_x() 
			       << ", Deploy_y = " << last_deploy_cmd_.deploy_y() 
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
		  
                }
	      ~Sampletarfield() 
                {
                }

            };

            // /Mission/Deploy/Helix
            struct Helix: boost::statechart::simple_state<Helix, Deploy>,
                StateNotify<LAMSS_STATE_DEPLOY_HELIX>  
            {
	      Helix()
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "DEPLOY_MISSION", "HELIX"));
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Deploy Helix Mode..." 
			       << "Deploy mode=" << last_deploy_cmd_.deploy_mode()
			       << ", Deploy_x = " << last_deploy_cmd_.deploy_x() 
			       << ", Deploy_y = " << last_deploy_cmd_.deploy_y() 
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
		  
                }
	      ~Helix() 
                {
                }

            };

            // /Mission/Deploy/UnknownDeploy
            struct UnknownDeploy: boost::statechart::simple_state<UnknownDeploy, Deploy>,
                StateNotify<LAMSS_STATE_DEPLOY_UNKNOWN>  
            {
	      UnknownDeploy()
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "DEPLOY_MISSION", "UNKNOWN"));
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Deploy Unknown Mode..." 
			       << "Deploy mode=" << last_deploy_cmd_.deploy_mode()
			       << ", Deploy_x = " << last_deploy_cmd_.deploy_x() 
			       << ", Deploy_y = " << last_deploy_cmd_.deploy_y() 
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
		  
                }
	      ~UnknownDeploy() 
                {
                }

            };

	    // Prosecute states

            // /Mission/Prosecute
            struct Prosecute : boost::statechart::simple_state<Prosecute, Mission, StartProsecute>,
                StateNotify<LAMSS_STATE_PROSECUTE> 
            {
	      Prosecute() 
                {
		  
		  boost::mutex::scoped_lock lock(moos_mutex_);

		  prosecute_mode_active_ = true;

		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Prosecute Mode..."
			       << "deploy_duration=" << cfg_->deploy_duration() 
			       << "prosecute_duration=" << last_prosecute_cmd_.prosecute_duration() 
			       << std::endl;
		  
		  std::string serialized;
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, 
						    "MISSION_STATE", "PROSECUTE"));
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
	    
            // /Mission/Prosecute/StartProsecute
            struct StartProsecute: boost::statechart::state<StartProsecute, Prosecute>,
                StateNotify<LAMSS_STATE_START_PROSECUTE> 
            {
                StartProsecute(my_context ctx) : my_base(ctx)
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  switch (last_prosecute_cmd_.prosecute_type())
		    {	  
		    case LAMSS_PROSECUTE::PROSECUTE_TYPE_ADAPTIVE:
		    post_event(EvProsecuteAdaptive());
		    break;

		    case LAMSS_PROSECUTE::PROSECUTE_TYPE_TMA_ZIGZAG:
		    post_event(EvProsecuteTmazigzag());
		    break;

		    case LAMSS_PROSECUTE::PROSECUTE_TYPE_POLYGON:
		    post_event(EvProsecutePolygon());
		    break;

		    default:
		    post_event(EvProsecuteUnknown());
		    break;

		    }
                }
                ~StartProsecute() { }

                typedef boost::mpl::list<
		  boost::statechart::transition< EvProsecuteAdaptive, Adaptive  >,
		  boost::statechart::transition< EvProsecuteTmazigzag, Tmazigzag >,
		  boost::statechart::transition< EvProsecutePolygon, Polygon >
                    > reactions;
       
            };

            // /Mission/Prosecute/Adaptive
            struct Adaptive: boost::statechart::simple_state<Adaptive, Prosecute>,
                StateNotify<LAMSS_STATE_PROSECUTE_ADAPTIVE>  
            {
	      Adaptive()
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "PROSECUTE_TYPE", "ADAPTIVE"));
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Prosecute Adaptive Mode..." 
			       << "Prosecute mode=" << last_prosecute_cmd_.prosecute_type()
			       << "Target_x = " << last_prosecute_cmd_.target_x() 
			       << "Target_y = " << last_prosecute_cmd_.target_y() 
			       << "Target_hdg = " << last_prosecute_cmd_.target_heading() 
			       << "Target_spd = " << last_prosecute_cmd_.target_speed() 
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
		  
                }
	      ~Adaptive() 
                {
                }

            };

            // /Mission/Prosecute/Tmazigzag
            struct Tmazigzag: boost::statechart::simple_state<Tmazigzag, Prosecute>,
                StateNotify<LAMSS_STATE_PROSECUTE_TMAZIGZAG>  
            {
	      Tmazigzag()
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "PROSECUTE_TYPE", "TMAZIGZAG"));
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Prosecute Tmazigzag Mode..." 
			       << "Prosecute mode=" << last_prosecute_cmd_.prosecute_type()
			       << "Target_x = " << last_prosecute_cmd_.target_x() 
			       << "Target_y = " << last_prosecute_cmd_.target_y() 
			       << "Target_hdg = " << last_prosecute_cmd_.target_heading() 
			       << "Target_spd = " << last_prosecute_cmd_.target_speed() 
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
		  
                }
	      ~Tmazigzag() 
                {
                }

            };

            // /Mission/Prosecute/Polygon
            struct Polygon: boost::statechart::simple_state<Polygon, Prosecute>,
                StateNotify<LAMSS_STATE_PROSECUTE_POLYGON>  
            {
	      Polygon()
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "PROSECUTE_TYPE", "POLYGON"));
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Prosecute Polygon Mode..." 
			       << "Prosecute mode=" << last_prosecute_cmd_.prosecute_type()
			       << "Target_x = " << last_prosecute_cmd_.target_x() 
			       << "Target_y = " << last_prosecute_cmd_.target_y() 
			       << "Target_hdg = " << last_prosecute_cmd_.target_heading() 
			       << "Target_spd = " << last_prosecute_cmd_.target_speed() 
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
		  
                }
	      ~Polygon() 
                {
                }

            };

            // /Mission/Prosecute/UnknownProsecute
            struct UnknownProsecute: boost::statechart::simple_state<UnknownProsecute, Prosecute>,
                StateNotify<LAMSS_STATE_PROSECUTE_UNKNOWN>  
            {
	      UnknownProsecute()
                {
		  boost::mutex::scoped_lock lock(moos_mutex_);
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, "PROSECUTE_MISSION", "UNKNOWNPROSECUTE"));
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering UnknownProsecute Mode..." 
			       << "Prosecute mode=" << last_prosecute_cmd_.prosecute_type()
			       << "Target_x = " << last_prosecute_cmd_.target_x() 
			       << "Target_y = " << last_prosecute_cmd_.target_y() 
			       << "Target_hdg = " << last_prosecute_cmd_.target_heading() 
			       << "Target_spd = " << last_prosecute_cmd_.target_speed() 
			       << std::endl;
		  
		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "success..." 
			       << std::endl;
		  
                }
	      ~UnknownProsecute() 
                {
                }

            };

            // /Mission/Abort
            struct Abort : boost::statechart::simple_state<Abort, Mission >,
                StateNotify<LAMSS_STATE_ABORT> 
            {
	      Abort() 
                {

		  boost::mutex::scoped_lock lock(moos_mutex_);		  
		  abort_mode_active_ = true;

		  goby::glog.is(goby::common::logger::VERBOSE) && 
		    goby::glog << group("fsm") 
			       << "Entering Abort Mode..."
			       << "deploy_duration=" << cfg_->deploy_duration() 
			       << std::endl;
		  
		  std::string serialized;
		  publish_queue_.push_back(CMOOSMsg(MOOS_NOTIFY, 
						    "MISSION_STATE", "ABORT"));
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
	    

        }
    }
}


#endif

