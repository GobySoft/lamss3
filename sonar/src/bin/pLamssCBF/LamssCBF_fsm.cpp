// Copyright 2015-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LamssCBF_fsm.h"

boost::shared_ptr<lamss::sonar::LamssCBFConfig> lamss::sonar::fsm::cfg_;

lamss::sonar::ProcessingStateEnum lamss::sonar::fsm::state_ = PROCESSING_STATE_IDLE;

std::deque<CMOOSMsg> lamss::sonar::fsm::publish_queue_;
boost::mutex lamss::sonar::fsm::moos_mutex_;

boost::condition_variable lamss::sonar::fsm::processing_cond_;

double lamss::sonar::fsm::current_time_ = 0;

bool lamss::sonar::fsm::processing_enable_ = false;
bool lamss::sonar::fsm::processing_idle_ = false;
bool lamss::sonar::fsm::processing_active_ = false;
bool lamss::sonar::fsm::broadband_active_ = false;
double lamss::sonar::fsm::broadband_start_ = 0;
bool lamss::sonar::fsm::tones_active_ = false;
double lamss::sonar::fsm::tones_start_ = 0;

/*
LAMSS_DEPLOY lamss::sonar::fsm::last_deploy_cmd_;
LAMSS_PROSECUTE lamss::sonar::fsm::last_prosecute_cmd_;
*/
