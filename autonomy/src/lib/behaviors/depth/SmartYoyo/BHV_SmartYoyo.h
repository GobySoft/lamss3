// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BHV_SMART_YOYO_HEADER
#define BHV_SMART_YOYO_HEADER

#include "IvPBehavior.h"
#include <string>

class IvPDomain;
namespace lamss
{
namespace autonomy
{

class BHV_SmartYoyo : public IvPBehavior
{
  public:
    BHV_SmartYoyo(IvPDomain);
    ~BHV_SmartYoyo(){};

    bool setParam(std::string, std::string);
    IvPFunction* onRunState();
    void onIdleState();

  protected:
  private: // Behavior Parameters
    std::string depth_function;
    double period, period_segments, distance_traveled;
    double min_depth, max_depth, max_depth_local, new_depth;
    double deploy_speed, deploy_depth;
    double min_altitude;
    bool m_inverted_dvl;

  private: // Behavior State Variables
    bool first_iteration;
    double mark_time, mark_distance, mark_increment, start_time, iter_time;
};

} // namespace autonomy
} // namespace lamss

#endif
