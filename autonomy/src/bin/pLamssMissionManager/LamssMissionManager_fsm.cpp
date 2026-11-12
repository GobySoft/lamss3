// Copyright 2015-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

//#include "goby/util/debug_logger.h"
//#include "goby/time/legacy.h"
//#include "goby/util/binary.h"

#include "LamssMissionManager_fsm.h"

//using goby::glog;
//using namespace goby::util::logger;
//using goby::common::goby_time;

boost::shared_ptr<LamssMissionManagerConfig> lamss::shared::fsm::cfg_;

Mission_StateEnum lamss::shared::fsm::state_ = lamss::core::MISSION_STATE_ALLSTOP;

std::deque<CMOOSMsg> lamss::shared::fsm::publish_queue_;
boost::mutex lamss::shared::fsm::moos_mutex_;

boost::condition_variable lamss::shared::fsm::command_received_cond_;
boost::condition_variable lamss::shared::fsm::touchdown_altitude_cond_;
boost::condition_variable lamss::shared::fsm::touchdown_completed_cond_;
boost::condition_variable lamss::shared::fsm::operation_altitude_cond_;
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
bool lamss::shared::fsm::landing_state_active_;
bool lamss::shared::fsm::descent_state_active_;
bool lamss::shared::fsm::touchdown_state_active_;
bool lamss::shared::fsm::bottoming_state_active_;
bool lamss::shared::fsm::liftoff_state_active_;
bool lamss::shared::fsm::touchdown_altitude_reached_;
bool lamss::shared::fsm::touchdown_location_reached_;
bool lamss::shared::fsm::touchdown_completed_;
bool lamss::shared::fsm::operation_altitude_ = true;
;

double lamss::shared::fsm::current_time_ = 0;
double lamss::shared::fsm::last_acomms_ = 0;
double lamss::shared::fsm::last_gps_ = 0;
double lamss::shared::fsm::abort_time_ = 0;
double lamss::shared::fsm::abort_duration_ = 0;
