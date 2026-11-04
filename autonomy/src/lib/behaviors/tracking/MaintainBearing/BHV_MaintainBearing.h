// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BHV_MAINTAINBEARING_HEADER
#define BHV_MAINTAINBEARING_HEADER

#include <string>

#include "IvPBehavior.h"
#include "lamss/core/henrik_util/CVectorUtils.h"

//using namespace std;

class IvPDomain;

namespace lamss
{
namespace autonomy
{

class BHV_MaintainBearing : public IvPBehavior
{
  public:
    BHV_MaintainBearing(IvPDomain);
    ~BHV_MaintainBearing(){};

    IvPFunction* onRunState();
    void onIdleState();
    bool setParam(std::string, std::string);
    double getRelevance();

  protected:
    int decode(std::string);

    bool new_run;
    bool alternate;
    double curr_time;
    double last_switch;
    double alternate_interval;
    double bearing_rate_threshold;
    std::vector<double> bearings;
    std::vector<double> times;

    bool starboard;
    double over_shoot;
    double desired_angle, true_bearing;
    int width, pwidth;
    int bearing_state;
    std::string heading_variable;
    std::string bearing_variable;
    std::string bearing_type;
    bool absolute_bearing;
    std::string bearing_mode;

    lamss::core::CmyVectorUtils vec;
};

} // namespace autonomy
} // namespace lamss

#endif
