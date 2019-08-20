// Copyright 2014 
// Henrik Schmidt
// Toby Schneider 
// Massachusetts Institute of Technology 

//#include "goby/util/debug_logger.h"
//#include "goby/time/legacy.h"
//#include "goby/util/binary.h"

#include "LamssMissionManager_fsm.h"

//using goby::glog;
//using namespace goby::util::logger;
//using goby::common::goby_time;

boost::shared_ptr<LamssMissionManagerConfig> lamss::shared::fsm::cfg_;

Mission_StateEnum lamss::shared::fsm::state_ = MISSION_STATE_ALLSTOP;


std::deque<CMOOSMsg> lamss::shared::fsm::publish_queue_;
boost::mutex lamss::shared::fsm::moos_mutex_;

boost::condition_variable lamss::shared::fsm::command_received_cond_;  
//boost::condition_variable lamss::shared::fsm::blackout_cond_;  
//boost::condition_variable lamss::shared::fsm::operation_radius_cond_;  
//boost::condition_variable lamss::shared::fsm::operation_box_cond_;  

int lamss::shared::fsm::vehicle_id_ = 0;
std::string lamss::shared::fsm::vehicle_name_ = "ownship";


bool lamss::shared::fsm::command_received_ = false;
bool lamss::shared::fsm::mission_active_ = false;
bool lamss::shared::fsm::diving_active_ = false;
bool lamss::shared::fsm::deploy_mode_active_ = false;
bool lamss::shared::fsm::prosecute_mode_active_ = false;
bool lamss::shared::fsm::surfacing_mode_active_ = false;
bool lamss::shared::fsm::abort_mode_active_ = false;
bool lamss::shared::fsm::drifting_mode_active_ = false;
bool lamss::shared::fsm::homing_mode_active_ = false;

double lamss::shared::fsm::current_time_ = 0;
double lamss::shared::fsm::last_acomms_ = 0;
double lamss::shared::fsm::abort_time_ = 0;
double lamss::shared::fsm::abort_duration_ = 0;

LAMSS_TRANSIT lamss::shared::fsm::last_transit_route_;
LAMSS_DEPLOY lamss::shared::fsm::last_deploy_cmd_;
LAMSS_PROSECUTE lamss::shared::fsm::last_prosecute_cmd_;

