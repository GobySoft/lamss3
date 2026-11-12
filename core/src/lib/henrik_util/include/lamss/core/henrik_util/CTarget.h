// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __CTARGET_h__
#define __CTARGET_h__

#include "MBUtils.h"
#include "MOOS/libMOOS/MOOSLib.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"

#include <fstream>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>
#include <vector>

namespace lamss
{
namespace core
{

class CTarget
{
  public:
    bool setTargetValues(int tgt_num, bool tgt_on, std::vector<double> values);
    bool getTargetValues(int& tgt_num, bool& tgt_on, std::vector<double>& values);
    std::string printTargetValues();
    std::string target_id();
    std::string target_state();

    bool target_on;
    int target_number;
    double target_utc;
    double target_x;
    double target_y;
    double target_spd;
    double target_hdg;
    double target_dep;
    double target_spl;
    double target_freq;
    double target_bw;
    double target_strength;
    double target_str;
    std::vector<double> tone_freq;
    std::vector<double> tone_dbl;

    double target_amp() { return pow(10.0, -6.0) * pow(10.0, target_spl / 20.0); };
};

} // namespace core
} // namespace lamss

#endif /* __CTARGET_h__ */
