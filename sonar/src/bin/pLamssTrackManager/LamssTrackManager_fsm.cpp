// Copyright 2015-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LamssTrackManager_fsm.h"

//using goby::glog;
//using namespace goby::util::logger;
//using goby::common::goby_time;

boost::shared_ptr<lamss::sonar::LamssTrackManagerConfig> lamss::sonar::fsm::cfg_;

lamss::core::BearingTrackEnum lamss::sonar::fsm::state_ = TRACKING_IDLE;

std::deque<CMOOSMsg> lamss::sonar::fsm::publish_queue_;
boost::mutex lamss::sonar::fsm::moos_mutex_;

boost::condition_variable lamss::sonar::fsm::dclt_cond_;
//boost::condition_variable lamss::sonar::fsm::blackout_cond_;
//boost::condition_variable lamss::sonar::fsm::operation_radius_cond_;
//boost::condition_variable lamss::sonar::fsm::operation_box_cond_;

int lamss::sonar::fsm::vehicle_id_ = 0;

double lamss::sonar::fsm::current_time_ = 0;
int lamss::sonar::fsm::track_state_ = 0;

bool lamss::sonar::fsm::dclt_enable_ = false;
bool lamss::sonar::fsm::dclt_idle_ = false;
bool lamss::sonar::fsm::dclt_active_ = false;
bool lamss::sonar::fsm::detection_active_ = false;
double lamss::sonar::fsm::detection_start_ = 0;
bool lamss::sonar::fsm::localization_active_ = false;
double lamss::sonar::fsm::localization_start_ = 0;
bool lamss::sonar::fsm::tracking_active_ = false;
double lamss::sonar::fsm::tracking_start_ = 0;
double lamss::sonar::fsm::tracking_duration_ = 0;
bool lamss::sonar::fsm::tracking_single_active_ = false;
bool lamss::sonar::fsm::tracking_multiple_active_ = false;
int lamss::sonar::fsm::bearing_track_count_ = 0;
int lamss::sonar::fsm::geo_track_count_ = 0;
int lamss::sonar::fsm::geo_track_report_count_ = 0;
bool lamss::sonar::fsm::classifying_active_ = false;
double lamss::sonar::fsm::classifying_start_ = 0;
// pTrackQuality variables
int lamss::sonar::fsm::m_buffer_length_ = 20;
double lamss::sonar::fsm::m_xy_sigma_ = 100;
bool lamss::sonar::fsm::cue_in_range_ = false;
bool lamss::sonar::fsm::cue_too_close_ = false;
bool lamss::sonar::fsm::prosecute_pending_ = false;
