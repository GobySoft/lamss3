// Copyright 2014 
// Henrik Schmidt
// Toby Schneider 
// Massachusetts Institute of Technology 

//#include "goby/common/logger.h"
//#include "goby/common/time.h"
//#include "goby/util/binary.h"

#include "LamssTrackManager_fsm.h"

//using goby::glog;
//using namespace goby::common::logger;
//using goby::common::goby_time;

boost::shared_ptr<LamssTrackManagerConfig> lamss::shared::fsm::cfg_;

BearingTrackEnum lamss::shared::fsm::state_ = TRACKING_IDLE;


std::deque<CMOOSMsg> lamss::shared::fsm::publish_queue_;
boost::mutex lamss::shared::fsm::moos_mutex_;

boost::condition_variable lamss::shared::fsm::dclt_cond_;  
//boost::condition_variable lamss::shared::fsm::blackout_cond_;  
//boost::condition_variable lamss::shared::fsm::operation_radius_cond_;  
//boost::condition_variable lamss::shared::fsm::operation_box_cond_;  

int lamss::shared::fsm::vehicle_id_ = 0;

double lamss::shared::fsm::current_time_ = 0;
bool lamss::shared::fsm::mission_active_ = false;
int lamss::shared::fsm::track_state_ = 0;

bool lamss::shared::fsm::dclt_enable_ = false;
bool lamss::shared::fsm::dclt_idle_ = false;
bool lamss::shared::fsm::dclt_active_ = false;
bool lamss::shared::fsm::detection_active_ = false;
double lamss::shared::fsm::detection_start_ = 0;
bool lamss::shared::fsm::localization_active_ = false;
double lamss::shared::fsm::localization_start_ = 0;
bool lamss::shared::fsm::tracking_active_ = false;
double lamss::shared::fsm::tracking_start_ = 0;
double lamss::shared::fsm::tracking_duration_ = 0;
bool lamss::shared::fsm::tracking_single_active_ = false;
bool lamss::shared::fsm::tracking_multiple_active_ = false;
int lamss::shared::fsm::bearing_track_count_ = 0;
int lamss::shared::fsm::geo_track_count_ = 0;
int lamss::shared::fsm::geo_track_report_count_ = 0;
bool lamss::shared::fsm::classifying_active_ = false;
double lamss::shared::fsm::classifying_start_ = 0;
// pTrackQuality variables
int lamss::shared::fsm::m_buffer_length_ = 20;
double lamss::shared::fsm::m_xy_sigma_ = 100;

bool lamss::shared::fsm::deploy_mode_active_ = false;
bool lamss::shared::fsm::deploy_received_ = false;
bool lamss::shared::fsm::prosecute_mode_active_ = false;
bool lamss::shared::fsm::cue_in_range_ = false;
bool lamss::shared::fsm::cue_too_close_ = false;

LAMSS_DEPLOY lamss::shared::fsm::last_deploy_cmd_;
LAMSS_PROSECUTE lamss::shared::fsm::last_prosecute_cmd_;
LAMSS_PROSECUTE lamss::shared::fsm::auto_prosecute_cmd_;
LAMSS_TRACK lamss::shared::fsm::last_track_report_;

