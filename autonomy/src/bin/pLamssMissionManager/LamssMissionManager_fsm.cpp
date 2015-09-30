// Copyright 2014 
// Henrik Schmidt
// Toby Schneider 
// Massachusetts Institute of Technology 

//#include "goby/common/logger.h"
//#include "goby/common/time.h"
//#include "goby/util/binary.h"

#include "LamssMissionManager_fsm.h"

//using goby::glog;
//using namespace goby::common::logger;
//using goby::common::goby_time;

boost::shared_ptr<lamss::dsop::protobuf::LamssMissionManagerConfig> lamss::dsop::fsm::cfg_;

Tracking_Cycle_StateEnum lamss::dsop::fsm::state_ = TRACKING_STATE_IDLE;


std::deque<CMOOSMsg> lamss::shared::fsm::publish_queue_;
boost::mutex lamss::shared::fsm::moos_mutex_;

boost::condition_variable lamss::shared::fsm::doppler_cond_;  

int lamss::shared::fsm::vehicle_id_ = 0;
bool lamss::shared::fsm::deploy_mode_active_ = false;
bool lamss::shared::fsm::deploy_received_ = false;
bool lamss::shared::fsm::prosecute_mode_active_ = false;
bool lamss::shared::fsm::prosecute_received_ = false;
bool lamss::shared::fsm::surfacing_mode_active_ = false;

double lamss::shared::fsm::last_acomms_ = 0;
double lamss::shared::fsm::prosecute_time_ = 0;

LAMSS_DEPLOY lamss::shared::fsm::last_deploy_;
LAMSS_PROSECUTE lamss::shared::fsm::last_prosecute_;

