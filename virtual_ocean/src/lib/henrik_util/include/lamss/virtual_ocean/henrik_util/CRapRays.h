// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __CRAPRAYS_h__
#define __CRAPRAYS_h__

#include "MOOS/libMOOS/MOOSLib.h"
// #include "AngleUtils.h"

#include <math.h>
#include <vector>

namespace lamss
{
namespace virtual_ocean
{

class CRapRays
{
  public:
    bool setEnv(double c_surface, double c_gradient, double water_depth, double z_rap_max);
    bool rapRay(double x0, double y0, double z0, double x, double y, double z, double& theta0,
                double& p_tl, double& tau, bool& turn);
    bool rapDepth(double x, double y, double z, double x0, double y0, double& z_rap);
    bool rapRange(double x0, double y0, double z0, double z, double th0_deg, double& r_rap);
    bool maxRapRange(double sonar_depth, double target_depth, double& range);
    bool rapDipRange(double z0, double z, double tau, double& th0_deg, double& r_rap);
    bool rapGeoTrack(double x0, double y0, double hdg0, double spd0, double brg, double brg_rate,
                     double r, double r_rate, double& x, double& y, double& hdg, double& spd);
    bool rapFootprint(double elev_deg, double my_depth, double rap_depth, double beam_width,
                      double& range_center, double& range_3db_inner, double& range_3db_outer,
                      double& tau_center, double& tau_inner, double& tau_outer, double& amp_center,
                      double& amp_inner, double& amp_outer);

  protected:
    double logTerm(double c, double a);

    double c_0;
    double c_grad;
    double w_depth;
    double z_max;
};

} // namespace virtual_ocean
} // namespace lamss

#endif /* __CRAPRAYS_h__ */
