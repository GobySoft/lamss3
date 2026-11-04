// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BHV_ZIGZAG_HEADER
#define BHV_ZIGZAG_HEADER

#include "goby/moos/moos_geodesy.h"

#include "IvPBehavior.h"
#include <string>

class IvPDomain;
namespace lamss
{
namespace autonomy
{

class BHV_ZigZag : public IvPBehavior
{
  public:
    BHV_ZigZag(IvPDomain);
    ~BHV_ZigZag(){};

    bool setParam(std::string, std::string);
    IvPFunction* onRunState();
    void onIdleState();

  protected:
    double currentCompensatedHeading(double desired_current, double water_speed);

  private: // Behavior Parameters
    goby::moos::CMOOSGeodesy m_Geodesy;

    // Dynamic UTM
    double lat_origin_;
    double lon_origin_;
    bool got_lat_origin;
    bool got_lon_origin;
    bool geo_init;

    std::string zigzag_function;
    double period, distance_traveled, heading, amplitude, start_x, start_y;
    double head_x, head_y, new_heading, speed;
    double start_lat, start_lon;

    double east_current_spd;
    double north_current_spd;
    bool m_current_compensator;
    bool first_iteration;
};

} // namespace autonomy
} // namespace lamss

#endif
