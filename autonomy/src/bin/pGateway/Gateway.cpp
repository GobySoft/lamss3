/*****************************************************************/
/*    NAME: Henrik Schmidt                                       */
/*    ORGN: MIT Cambridge MA                                     */
/*    FILE: Gateway.cpp                                 */
/*    DATE: Aug. 2, 2008                                         */
/*                                                               */
/* This program is free software; you can redistribute it and/or */
/* modify it under the terms of the GNU General Public License   */
/* as published by the Free Software Foundation; either version  */
/* 2 of the License, or (at your option) any later version.      */
/*                                                               */
/* This program is distributed in the hope that it will be       */
/* useful, but WITHOUT ANY WARRANTY; without even the implied    */
/* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       */
/* PURPOSE. See the GNU General Public License for more details. */
/*                                                               */
/* You should have received a copy of the GNU General Public     */
/* License along with this program; if not, write to the Free    */
/* Software Foundation, Inc., 59 Temple Place - Suite 330,       */
/* Boston, MA 02111-1307, USA.                                   */
/*****************************************************************/

#include "Gateway.h"

#include <iostream>
#include <math.h>
#include <sstream>

using namespace std;

CGateway::CGateway()
{
    deploy_x = 0;
    deploy_y = 0;
}

CGateway::~CGateway() {}

bool CGateway::OnStartUp()
{
    //Initialize m_Geodesy from lat lon origin in .moos file
    string sVal;
    double dfLatOrigin;
    double dfLongOrigin;

    if (m_MissionReader.GetValue("LatOrigin", sVal))
    {
        dfLatOrigin = atof(sVal.c_str());
        //MOOSTrace("  LatOrigin  = %10.5f deg.\n",dfLatOrigin);
    }
    else
    {
        MOOSTrace("LatOrigin not specified in mission file - FAIL\n");
        return false;
    }

    if (m_MissionReader.GetValue("LongOrigin", sVal))
    {
        dfLongOrigin = atof(sVal.c_str());
        //MOOSTrace("  LongOrigin = %10.5f deg.\n",dfLongOrigin);
    }
    else
    {
        MOOSTrace("LongOrigin not specified in mission file - FAIL\n");
        return false;
    }
    if (!m_Geodesy.Initialise(dfLatOrigin, dfLongOrigin))
    {
        MOOSTrace("Geodesy Init inside pTargetSim failed - FAIL\n");
        return false;
    }
    //Done initializaing m_Geodesy

    double d_x;
    if (m_MissionReader.GetConfigurationParam("deploy_x", d_x))
    {
        deploy_x = d_x;
        // m_Geodesy.UTM2LatLong(deploy_x,deploy_y,deploy_lat,deploy_lon);
        m_Geodesy.UTM2LatLong(deploy_x, deploy_y, latitude, longitude);
        deploy_lat = latitude;
        deploy_lon = longitude;
    }

    double d_y;
    if (m_MissionReader.GetConfigurationParam("deploy_y", d_y))
    {
        deploy_y = d_y;
        // m_Geodesy.UTM2LatLong(deploy_x,deploy_y,deploy_lat,deploy_lon);
        m_Geodesy.UTM2LatLong(deploy_x, deploy_y, latitude, longitude);
        deploy_lat = latitude;
        deploy_lon = longitude;
    }

    if (m_MissionReader.GetConfigurationParam("deploy_longitude", d_x))
    {
        deploy_lon = d_x;
    }

    if (m_MissionReader.GetConfigurationParam("deploy_latitude", d_y))
    {
        deploy_lat = d_y;
    }

    double d_depth;

    if (m_MissionReader.GetConfigurationParam("deploy_depth", d_depth))
    {
        deploy_depth = d_depth;
    }
    else
    {
        MOOSTrace("pGateway:: deploy depth not set - FAIL\n");
        return false;
    }

    start_time = MOOSTime();

    if (m_Geodesy.LatLong2LocalUTM(deploy_lat, deploy_lon, deploy_y, deploy_x))
    {
        MOOSTrace("Gateway Deploy location: Lat = %10.6f, Long =  %10.6f\n", deploy_lat,
                  deploy_lon);
        MOOSTrace("Gateway Deploy location: x = %f, Long =  %f\n", deploy_x, deploy_y);
    }
    else
    {
        MOOSTrace("Error converting Latitude/Longitude to local coordinates\n");
        return (false);
    }
    return (true);
}

bool CGateway::OnConnectToServer()
{
    m_Comms.Register("VEHICLE_NAME");
    return (true);
}

bool CGateway::Iterate()
{
    double curr_time = MOOSTime();
    m_Comms.Notify("NAV_X", deploy_x);
    m_Comms.Notify("NAV_Y", deploy_y);
    double d_hdg = 0.;
    double d_spd = 0;
    m_Comms.Notify("NAV_HEADING", d_hdg);
    m_Comms.Notify("NAV_SPEED", d_spd);
    m_Comms.Notify("NAV_DEPTH", deploy_depth);
    m_Comms.Notify("NAV_HEADING", d_hdg);
    m_Comms.Notify("NAV_LAT", deploy_lat);
    m_Comms.Notify("NAV_LONG", deploy_lon);

    // Create NODE_REPORT
    string output = "NAME=" + ship_name + ",TYPE=ship,";

    output += "UTC_TIME=" + doubleToString(curr_time, 1);
    output += ",";

    output += "X=" + doubleToString(deploy_x, 2);
    output += ",";

    output += "Y=" + doubleToString(deploy_y, 2);
    output += ",";

    output += "LAT=" + doubleToString(deploy_lat, 6);
    output += ",";

    output += "LON=" + doubleToString(deploy_lon, 6);
    output += ",";

    output += "SPD=" + doubleToString(d_spd, 2);
    output += ",";

    output += "HDG=" + doubleToString(d_hdg, 2);
    output += ",";

    output += "DEPTH=" + doubleToString(deploy_depth, 2);

    m_Comms.Notify("NODE_REPORT_NMEA_SHIP", output);
    return (true);
}

//-------------------------------------------------------------
// Procedure: OnNewMail

bool CGateway::OnNewMail(MOOSMSG_LIST& NewMail)

{
    CMOOSMsg Msg;

    double dfLatOrigin, dfLongOrigin;
    bool new_origin = false;

    MOOSMSG_LIST::reverse_iterator p;
    for (p = NewMail.rbegin(); p != NewMail.rend(); p++)
    {
        CMOOSMsg& Msg = *p;

        if (Msg.m_sKey == "LAT_ORIGIN")
        {
            dfLatOrigin = Msg.GetDouble();
            ;
            new_origin = true;
        }

        if (Msg.m_sKey == "LONG_ORIGIN")
        {
            dfLongOrigin = Msg.GetDouble();
            new_origin = true;
        }

        if (Msg.m_sKey == "VEHICLE_NAME")
        {
            ship_name = Msg.m_sVal;
        }
    }

    if (new_origin)
    {
        if (!m_Geodesy.Initialise(dfLatOrigin, dfLongOrigin))
            MOOSTrace("Geodesy Init failed - FAIL\n");
    }

    return (true);
}
