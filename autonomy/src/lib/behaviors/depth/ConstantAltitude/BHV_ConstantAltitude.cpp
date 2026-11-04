// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
#pragma warning(disable : 4786)
#pragma warning(disable : 4503)
#endif
#include "BHV_ConstantAltitude.h"
#include "BuildUtils.h"
#include "MBUtils.h"
#include "OF_Coupler.h"
#include "ZAIC_PEAK.h"
#include <math.h>
#include <stdlib.h>

using namespace std;

namespace lamss
{
namespace autonomy
{

//-----------------------------------------------------------
// Procedure: Constructor

BHV_ConstantAltitude::BHV_ConstantAltitude(IvPDomain gdomain) : IvPBehavior(gdomain)
{
    this->setParam("descriptor", "(d)bhv_altitude");

    // Behavior Parameter Default Values:
    depth_local = 0; // meters
    altitude = 0;

    // default is normal downlooking DVL
    m_inverted_dvl = false;
    // Behavior State Variable Initial Values:

    m_domain = subDomain(m_domain, "depth");

    m_missing_altitude_critical = true;

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

void BHV_ConstantAltitude::onIdleState()
{
    // Do your thing here
}

//-----------------------------------------------------------
// Procedure: setParam

bool BHV_ConstantAltitude::setParam(string g_param, string g_val)
{
    if (IvPBehavior::setParamCommon(g_param, g_val))
        return (true);

    g_val = stripBlankEnds(g_val);

    if (g_param == "altitude")
    {
        double dval = atof(g_val.c_str());
        if ((dval < 0) || (!isNumber(g_val)))
            return (false);
        altitude = dval;
        return (true);
    }
    else if (g_param == "inverted_dvl")
        return (setBooleanOnString(m_inverted_dvl, g_val));
    else if (g_param == "missing_altitude_critical")
        return (setBooleanOnString(m_missing_altitude_critical, g_val));
    return (false);
}

//-----------------------------------------------------------
// Procedure: onRunState

IvPFunction* BHV_ConstantAltitude::onRunState()
{
    bool ok, ok1;
    double curr_depth = getBufferDoubleVal("NAV_DEPTH", ok);
    if (!ok)
    {
        postEMessage("No ownship NAV_DEPTH info in the helm info_buffer");
        postMessage("MIN_ALT_STATUS", "Missing NAV_DEPTH info");
        return (0);
    }
    double curr_altitude = getBufferDoubleVal("NAV_ALTITUDE", ok1);
    if (!ok1)
    {
        if (m_missing_altitude_critical)
            postEMessage("No ownship NAV_ALTITUDE info in the helm info_buffer");
        else
        {
            postEMessage("No ownship NAV_ALTITUDE info in the helm info_buffer");
            curr_altitude = 0;
        }
        postMessage("CONST_ALT_STATUS", "Missing NAV_ALTITUDE info");
        return (0);
    }
    else if (curr_altitude <= 0.1)
    {
        postMessage("CONST_ALT_STATUS", "Invalid NAV_ALTITUDE info");
        return (0);
    }

    // check here for bottom altitude and change max depth if necessary
    if (m_inverted_dvl)
    {
        // For inverted DVL (ICEX) it is assumed that NAV_ALTITUDE is
        // positive distance to bottom ice interface
        double curr_ice_depth = curr_depth - curr_altitude;
        depth_local = curr_ice_depth + altitude;
    }
    else
    {
        double curr_water_depth = curr_depth + curr_altitude;
        depth_local = curr_water_depth - altitude;
    }
    double peak_width = 0.1 * altitude;
    ZAIC_PEAK zaic_depth(m_domain, "depth");
    zaic_depth.setSummit(depth_local);
    zaic_depth.setPeakWidth(peak_width);
    zaic_depth.setBaseWidth(depth_local - peak_width);
    zaic_depth.setSummitDelta(90.0);
    zaic_depth.setMinMaxUtil(0, 100);

    IvPFunction* ipf_depth = zaic_depth.extractOF();

    ipf_depth->setPWT(m_priority_wt);

    return (ipf_depth);
}

} // namespace autonomy
} // namespace lamss
