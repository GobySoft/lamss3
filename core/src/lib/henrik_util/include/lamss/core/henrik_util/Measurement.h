// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __MEASUREMENT_h__
#define __MEASUREMENT_h__

#include "MBUtils.h"
#include "MOOS/libMOOS/MOOSLib.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"

#include <math.h>
#include <string.h>
#include <string>

namespace lamss
{
namespace core
{

class Measurement
{
  public:
    Measurement();
    ~Measurement();

    int node;
    int state;
    int beam;
    double bearing;
    double true_bearing;
    double sigma;
    double Meas_Time;

    double auv_x;
    double auv_y;
};

bool parse_measurement(std::string, Measurement*);

bool parse_brng_stat(std::string status, int& node, int& state, double& bearing, double& sigma,
                     double& xp, double& yp, double& time);

} // namespace core
} // namespace lamss

#endif /* __MEASUREMENT_h__ */
