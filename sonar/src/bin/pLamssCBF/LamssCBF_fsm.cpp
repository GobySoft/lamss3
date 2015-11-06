// Copyright 2015 
// Henrik Schmidt
// Toby Schneider 
// Massachusetts Institute of Technology 

#include "LamssCBF_fsm.h"

boost::shared_ptr<LamssCBFConfig> lamss::shared::fsm::cfg_;

ProcessingStateEnum lamss::shared::fsm::state_ = PROCESSING_STATE_IDLE;


std::deque<CMOOSMsg> lamss::shared::fsm::publish_queue_;
boost::mutex lamss::shared::fsm::moos_mutex_;

boost::condition_variable lamss::shared::fsm::processing_cond_;  

int lamss::shared::fsm::vehicle_id_ = 0;

double lamss::shared::fsm::current_time_ = 0;
bool lamss::shared::fsm::mission_active_ = false;
int lamss::shared::fsm::track_state_ = 0;

bool lamss::shared::fsm::processing_enable_ = false;
bool lamss::shared::fsm::processing_idle_ = false;
bool lamss::shared::fsm::processing_active_ = false;
bool lamss::shared::fsm::broadband_active_ = false;
double lamss::shared::fsm::broadband_start_ = 0;
bool lamss::shared::fsm::tones_active_ = false;
double lamss::shared::fsm::tones_start_ = 0;

/*
LAMSS_DEPLOY lamss::shared::fsm::last_deploy_cmd_;
LAMSS_PROSECUTE lamss::shared::fsm::last_prosecute_cmd_;
*/
