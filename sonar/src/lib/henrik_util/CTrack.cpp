// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "lamss/sonar/henrik_util/CTrack.h"

#include "MBUtils.h"
#include "MOOS/libMOOS/MOOSLib.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"

using namespace std;

namespace lamss
{
namespace sonar
{

//
// CTrack methods and functions
//

bool CTrack::clrTrack()
{
    trk_prio = 0;
    trk_state = 0;
    trk_ips = 0;
    trk_template = 5;
    trk_bearing = 0.0;
    trk_sigma = 0.0;
    trk_rate = 0.0;
    trk_rate_sigma = 0.0;
    trk_snr = 0.0;
    trk_nfeat = 0;
    trk_features.clear();
    new_trk = false;
    // dip added 100702
    trk_dip = 0.0;
    trk_dip_rate = 0.0;
    trk_dip_sigma = 0.0;
    trk_dip_rate_sigma = 0.0;
    return (true);
}

bool CTrack::bearingRate(double bearing, double sigma, double& rate, double& rate_sigma)
{
    double delta = MOOSTime() - trk_utc;
    rate = (bearing - trk_bearing) / delta;
    rate_sigma = 2 * sigma / delta;
    return (true);
}

bool CTrack::dipAngleRate(double dipangle, double dipangle_sigma, double& dipangle_rate,
                          double& dipangle_rate_sigma)
{
    double delta = MOOSTime() - trk_utc;
    dipangle_rate = (dipangle - trk_dip) / delta;
    dipangle_rate_sigma = 2 * dipangle_sigma / delta;
    return (true);
}

bool CTrack::setDipAngle(double dep, double dipangle, double dipangle_sigma, double dipangle_rate,
                         double dipangle_rate_sigma)
{
    trk_dep = dep;
    trk_dip = dipangle;
    trk_dip_rate = dipangle_rate;
    trk_dip_sigma = dipangle_sigma;
    trk_dip_rate_sigma = dipangle_rate_sigma;
    return (true);
}

bool CTrack::getDipAngle(double& dep, double& dipangle, double& dipangle_sigma,
                         double& dipangle_rate, double& dipangle_rate_sigma)
{
    dep = trk_dep;
    dipangle = trk_dip;
    dipangle_rate = trk_dip_rate;
    dipangle_sigma = trk_dip_sigma;
    dipangle_rate_sigma = trk_dip_rate_sigma;
    return (true);
}

bool CTrack::setTrack(int num, int prio, int templ, int state, int ips, double array_x,
                      double array_y, double bearing, double sigma, double rate, double rate_sigma,
                      double snr, vector<double>& features)
{
    trk_num = num;
    ;
    trk_prio = prio; // 1-10, 10 highest.
    trk_template = templ;
    trk_state = state; // 0: Invalid, 1: Relative, 2: Absolute.
    trk_ips = ips;     // -1: Port, 1: Starboard, 0: Ambiguous
    trk_x = array_x;
    trk_y = array_y;
    trk_bearing = bearing;
    trk_sigma = sigma;
    trk_rate = rate;
    trk_rate_sigma = rate_sigma;
    trk_snr = snr;
    trk_nfeat = features.size();
    trk_features.clear();
    for (int i = 0; i < trk_nfeat; i++) trk_features.push_back(features[i]);
    trk_utc = MOOSTime();
    new_trk = true;
    return (true);
}

bool CTrack::getTrack(int& num, int& prio, int& templ, int& state, int& ips, double& array_x,
                      double& array_y, double& bearing, double& sigma, double& rate,
                      double& rate_sigma, double& snr, double& utc, vector<double>& features)
{
    num = trk_num;
    ;
    prio = trk_prio; // 1-10, 10 highest.
    templ = trk_template;
    state = trk_state; // 0: Invalid, 1: Relative, 2: Bbsolute.
    ips = trk_ips;     // -1: port, 1: starboard, 0: Ambiguous.
    array_x = trk_x;
    array_y = trk_y;
    bearing = trk_bearing;
    sigma = trk_sigma;
    rate = trk_rate;
    rate_sigma = trk_rate_sigma;
    snr = trk_snr;
    utc = trk_utc;
    features.clear();

    for (int i = 0; i < trk_nfeat; i++) features.push_back(trk_features[i]);

    return (true);
}

bool CTrack::postTrack(string& moosvar, string& sval)
{
    // This function creates a string with the multi-track format used in
    // PLUS. Also returns suggested variable name: BEARING_TRACKS

    if (isnan(trk_bearing))
        return (false);

    moosvar = "BEARING_TRACKS";

    sval = "track=" + intToString(trk_num);
    sval += ",priority=" + intToString(trk_prio);
    sval += ",template=" + intToString(trk_template);
    sval += ",state=" + intToString(trk_state);
    sval += ",ips=" + intToString(trk_ips);
    sval += ",ael_x=" + doubleToString(trk_x, 1);
    sval += ",ael_y=" + doubleToString(trk_y, 1);
    sval += ",bearing=" + doubleToString(trk_bearing, 1);
    sval += ",bearing_sigma=" + doubleToString(trk_sigma, 2);
    sval += ",rate=" + doubleToString(trk_rate, 3);
    sval += ",rate_sigma=" + doubleToString(trk_rate_sigma, 3);
    sval += ",snr=" + doubleToString(trk_snr, 1);
    sval += ",time=" + doubleToString(trk_utc, 1);
    sval += ",nfeat=" + intToString(trk_nfeat);
    for (int i = 0; i < trk_nfeat; i++) sval += "," + doubleToString(trk_features[i], 2);

    return (true);
}

bool CTrack::postDipAngle(string& moosvar, string& sval)
{
    // This function creates a string with the multi-track format used in
    // PLUS. Also returns suggested variable name: DIP_TRACKS

    moosvar = "ELEVATION_TRACKS";

    sval = "track=" + intToString(trk_num);
    sval += ",state=" + intToString(trk_state);
    sval += ",ael_x=" + doubleToString(trk_x, 1);
    sval += ",ael_y=" + doubleToString(trk_y, 1);
    sval += ",ael_dep=" + doubleToString(trk_dep, 1);
    sval += ",elev=" + doubleToString(-trk_dip, 1);
    sval += ",elev_sigma=" + doubleToString(trk_dip_sigma, 3);
    sval += ",rate=" + doubleToString(-trk_dip_rate, 3);
    sval += ",rate_sigma=" + doubleToString(trk_dip_rate_sigma, 4);
    sval += ",time=" + doubleToString(trk_utc, 1);

    return (true);
}

string CTrack::bearingState(int node, double nav_x, double nav_y)
{
    // This function creates a string with the simple single track format used
    // in PLUSNet and in LAMSS. Usually published to the variable BEARING_STAT
    string msg = "node=" + intToString(node);
    msg += ",state=" + intToString(trk_state);
    msg += ",bearing=" + doubleToString(trk_bearing, 1);
    msg += ",xp=" + doubleToString(nav_x, 1);
    msg += ",yp=" + doubleToString(nav_y, 1);
    msg += ",beamno=0";
    msg += ",sigma=" + doubleToString(trk_sigma, 2);
    msg += ",rate=" + doubleToString(trk_rate, 3);
    msg += ",rate_sigma=" + doubleToString(trk_rate_sigma, 3);
    msg += ",snr=" + doubleToString(trk_snr, 2);
    msg += ",time=" + doubleToString(trk_utc);

    return (msg);
}

bool CTrack::decodeTrack(string val)
{
    string bite;
    //MOOSTrace("string = %s\n",status.c_str());

    MOOSValFromString(bite, val, "track");
    trk_num = atoi(bite.c_str());
    MOOSValFromString(bite, val, "priority");
    trk_prio = atoi(bite.c_str());
    MOOSValFromString(bite, val, "template");
    trk_template = atoi(bite.c_str());
    MOOSValFromString(bite, val, "state");
    trk_state = atoi(bite.c_str());
    MOOSValFromString(bite, val, "ips");
    trk_ips = atoi(bite.c_str());
    MOOSValFromString(trk_x, val, "ael_x");
    MOOSValFromString(trk_y, val, "ael_y");
    MOOSValFromString(trk_bearing, val, "bearing");
    MOOSValFromString(trk_sigma, val, "bearing_sigma");
    MOOSValFromString(trk_rate, val, "rate");
    MOOSValFromString(trk_rate_sigma, val, "rate_sigma");
    MOOSValFromString(trk_snr, val, "snr");
    MOOSValFromString(trk_utc, val, "time");

    //decode number of classification features following
    //decode ips
    MOOSChomp(val, "nfeat=");
    bite = MOOSChomp(val, ",");
    trk_nfeat = atoi(bite.c_str());

    trk_features.clear();
    for (int i = 0; i < trk_nfeat; i++)
    {
        bite = MOOSChomp(val, ",");
        trk_features.push_back(atof(bite.c_str()));
    }

    //  return(true);
    return (!isnan(trk_bearing));
}

} // namespace sonar
} // namespace lamss
