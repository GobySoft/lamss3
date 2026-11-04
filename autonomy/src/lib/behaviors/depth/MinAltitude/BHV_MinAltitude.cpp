// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
#pragma warning(disable : 4786)
#pragma warning(disable : 4503)
#endif
#include "BHV_MinAltitude.h"
#include "BuildUtils.h"
#include "MBUtils.h"
#include "OF_Coupler.h"
#include "ZAIC_HEQ.h"
#include "ZAIC_PEAK.h"
#include <iostream>
#include <math.h>
#include <stdlib.h>

using namespace std;

namespace lamss
{
namespace autonomy
{

//-----------------------------------------------------------
// Procedure: Constructor

BHV_MinAltitude::BHV_MinAltitude(IvPDomain gdomain) : IvPBehavior(gdomain)
{
    this->setParam("descriptor", "(d)bhv_min_altitude");

    // Behavior Parameter Default Values:
    max_depth_local = 0; // meters
    min_altitude = 0;
    // default is normal downlooking DVL
    m_inverted_dvl = false;

    // Behavior State Variable Initial Values:

    m_domain = subDomain(m_domain, "depth");

    // Declare the variables we will need from the info_buffer
    addInfoVars("NAV_DEPTH");
    addInfoVars("NAV_ALTITUDE");
}

//-----------------------------------------------------------
// Procedure: onIdleState
//      Note: This function overrides the onIdleState() virtual
//            function defined for the IvPBehavior superclass
//            This function will be executed by the helm each
//            time the behavior FAILS to meet its run conditions.

void BHV_MinAltitude::onIdleState()
{
    // Do your thing here
}

//-----------------------------------------------------------
// Procedure: setParam

bool BHV_MinAltitude::setParam(string g_param, string g_val)
{
    if (IvPBehavior::setParamCommon(g_param, g_val))
        return (true);

    g_val = stripBlankEnds(g_val);

    if (g_param == "min_altitude")
    {
        double dval = atof(g_val.c_str());
        if ((dval < 0) || (!isNumber(g_val)))
            return (false);
        min_altitude = dval;
        return (true);
    }
    else if (g_param == "inverted_dvl")
        return (setBooleanOnString(m_inverted_dvl, g_val));
    return (false);
}

//-----------------------------------------------------------
// Procedure: onRunState

IvPFunction* BHV_MinAltitude::onRunState()
{
    bool ok, ok1;
    double curr_depth = getBufferDoubleVal("NAV_DEPTH", ok);
    double curr_altitude = getBufferDoubleVal("NAV_ALTITUDE", ok1);
    // use only if valid, i.e. != 0
    if (ok1 && curr_altitude != 0.0)
    {
        if (m_inverted_dvl)
        {
            double curr_ice_depth = curr_depth - curr_altitude;
            double min_depth_local = curr_ice_depth + min_altitude + 1.0;

            ZAIC_HEQ zaic_depth(m_domain, "depth");
            zaic_depth.setSummit(min_depth_local);
            zaic_depth.setBaseWidth(1.0);
            zaic_depth.setMinMaxUtil(0, 100);

            IvPFunction* ipf_depth = zaic_depth.extractOF();

            if (ipf_depth)
                ipf_depth->setPWT(m_priority_wt);

            return (ipf_depth);
        }
        else
        {
            // check here for bottom altitude and change max depth
            // if necessary
            double curr_water_depth = curr_depth + curr_altitude;
            max_depth_local = curr_water_depth - min_altitude;

            ZAIC_PEAK zaic_depth(m_domain, "depth");
            zaic_depth.setSummit(0.0);
            zaic_depth.setPeakWidth(max_depth_local);
            zaic_depth.setBaseWidth(1.0);
            // zaic_depth.setSummitDelta(0.0);  // Henrik's version
            zaic_depth.setSummitDelta(0.1); // A mikerb version Sep 20, 2007
            zaic_depth.setMinMaxUtil(0, 100);

            IvPFunction* ipf_depth = zaic_depth.extractOF();

            if (ipf_depth)
                ipf_depth->setPWT(m_priority_wt);

            return (ipf_depth);
        }
    }
    return (0);
}

} // namespace autonomy
} // namespace lamss
