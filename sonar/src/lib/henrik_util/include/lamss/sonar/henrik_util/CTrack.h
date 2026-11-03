// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __CTRACK_h__
#define __CTRACK_h__

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
namespace sonar
{

class CTrack
{
  public:
    bool setTrack(int num, int prio, int templ, int state, int ips, double array_x, double array_y,
                  double bearing, double sigma, double rate, double rate_sigma, double snr,
                  std::vector<double>& features);
    bool getTrack(int& num, int& prio, int& templ, int& state, int& ips, double& array_x,
                  double& array_y, double& bearing, double& sigma, double& rate, double& rate_sigma,
                  double& snr, double& utc, std::vector<double>& features);
    bool postTrack(std::string& moosvar, std::string& svar);
    bool decodeTrack(std::string);
    std::string bearingState(int node, double nav_x, double nav_y);
    bool clrTrack();
    bool bearingRate(double bearing, double sigma, double& rate, double& rate_sigma);
    bool dipAngleRate(double dipangle, double dipangle_sigma, double& dipangle_rate,
                      double& dipangle_rate_sigma);
    bool setDipAngle(double dep, double dipangle, double dipangle_sigma, double dipangle_rate,
                     double dipangle_rate_sigma);
    bool getDipAngle(double& dep, double& dipangle, double& dipangle_sigma, double& dipangle_rate,
                     double& dipangle_rate_sigma);
    bool postDipAngle(std::string& moosvar, std::string& svar);

    int trk_num;

  protected:
    int trk_prio; // 1-10, 10 highest.
    int trk_template;
    int trk_state;  // 0: Invalid, 1: Relative, 2: Bbsolute.
    int trk_ips;    // -1: Port, 1: Starboard, 0: Ambiguous
    double trk_x;   // array x corresponding to track
    double trk_y;   // array y corresponding to track
    double trk_dep; // array depth corresponding to track
    double trk_bearing;
    double trk_sigma;
    double trk_rate;
    double trk_rate_sigma;
    double trk_snr;
    double trk_utc;
    int trk_nfeat;
    std::vector<double> trk_features;
    bool new_trk;
    // dipangle added 100702
    double trk_dip;
    double trk_dip_rate;
    double trk_dip_sigma;
    double trk_dip_rate_sigma;
};

} // namespace sonar
} // namespace lamss

#endif /* __CTRACK_h__ */
