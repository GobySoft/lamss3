#include "SimGPS.h"
#include <iostream>
#include <sstream>

using namespace std;

CSimGPS::CSimGPS()
{
    MOOSTrace("====================================\n");
    MOOSTrace("\n");
    MOOSTrace("             pSimGPS\n");
    MOOSTrace("\n");
    MOOSTrace("        V 1.0 02/12/2007 \n");
    MOOSTrace("\n");
    MOOSTrace("(c) Copyright, 2007 Henrik Schmidt MIT\n");
    MOOSTrace("====================================\n");
    on_surface = false;
    min_gps_time = 0;
    mark_gps_time = 0;
}

CSimGPS::~CSimGPS() {}

bool CSimGPS::OnStartUp()
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
        MOOSTrace("Geodesy Init inside pSimGPS failed - FAIL\n");
        return false;
    }
    //Done initializaing m_Geodesy

    if (m_MissionReader.GetConfigurationParam("gps_interval", sVal))
    {
        gps_interval = atoi(sVal.c_str());
        MOOSTrace("gps_interval = %d \n", gps_interval);
    }
    else
    {
        MOOSTrace("pSimGPS:: gps_interval is not specified in mission file - FAIL\n");
        return false;
    }

    if (m_MissionReader.GetConfigurationParam("gps_depth", sVal))
    {
        gps_depth = atof(sVal.c_str());
        MOOSTrace("gps_depth = %f \n", gps_depth);
    }
    else
    {
        MOOSTrace("pSimGPS:: gps_depth is not specified in mission file - FAIL\n");
        return false;
    }

    if (m_MissionReader.GetConfigurationParam("min_gps_time", sVal))
    {
        min_gps_time = atof(sVal.c_str());
        MOOSTrace("min_gps_time = %f \n", min_gps_time);
    }

    return (true);
}

bool CSimGPS::OnConnectToServer()
{
    m_Comms.Register("LAT_ORIGIN", 0);
    m_Comms.Register("LONG_ORIGIN", 0);
    m_Comms.Register("TRUE_X", 0);
    m_Comms.Register("TRUE_Y", 0);
    m_Comms.Register("NAV_DEPTH", 0);
    return (true);
}

bool CSimGPS::Iterate()
{
    double curr_time = MOOSTime();
    unsigned int int_time = (unsigned int)MOOSTime();
    bool surfaced = !on_surface && (nav_depth <= gps_depth);
    if (surfaced)
    {
        mark_gps_time = curr_time;
        on_surface = true;
        MOOSTrace("Surfaced, mark_gps_time = %f \n", mark_gps_time);
    }
    else if (on_surface)
    {
        int count_down = int(min_gps_time - (curr_time - mark_gps_time));
        if (count_down % 10 == 0)
            MOOSTrace("On surface, time to dive %d seconds \n", count_down);
        m_Comms.Notify("SECS_TO_DIVE", count_down);
    }
    if (curr_time - mark_gps_time >= min_gps_time)
    {
        if ((int_time % gps_interval == 0) && (nav_depth <= gps_depth))
            gps_update();
    }
    return (true);
}

//-------------------------------------------------------------
// Procedure: OnNewMail

bool CSimGPS::OnNewMail(MOOSMSG_LIST& NewMail)
{
    double dfLatOrigin, dfLongOrigin;
    bool new_origin = false;

    MOOSMSG_LIST::iterator p;

    for (p = NewMail.begin(); p != NewMail.end(); p++)
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

        if (Msg.m_sKey == "TRUE_X")
        {
            nav_x = Msg.m_dfVal;
            MOOSTrace("True x position = %f\n", nav_x);
        }
        //       else
        //       MOOSTrace("Unrecognized command: [%s]\n",Msg.m_sKey.c_str());

        if (Msg.m_sKey == "TRUE_Y")
        {
            nav_y = Msg.m_dfVal;
            MOOSTrace("True y position = %f\n", nav_y);
        }
        //       else
        //       MOOSTrace("Unrecognized command: [%s]\n",Msg.m_sKey.c_str());

        if (Msg.m_sKey == "NAV_DEPTH")
        {
            nav_depth = Msg.m_dfVal;
            if (nav_depth > gps_depth)
                on_surface = false;
        }
        //       else
        //       MOOSTrace("Unrecognized command: [%s]\n",Msg.m_sKey.c_str());
    }

    if (new_origin)
    {
        if (!m_Geodesy.Initialise(dfLatOrigin, dfLongOrigin))
            MOOSTrace("Geodesy Init failed - FAIL\n");
    }

    return (true);
}

void CSimGPS::gps_update()
{
    std::string sMsg;
    unsigned int iTime;

    iTime = (unsigned int)MOOSTime();

    MOOSTrace("GPS Update \n");

    //Convert local coordinates to Lat Long
    double g_latt = 0;
    double g_longt = 0;
    if (m_Geodesy.UTM2LatLong(nav_x, nav_y, g_latt, g_longt))
    {
        gps_lat = g_latt;
        gps_lon = g_longt;
    }

    sMsg = MOOSFormat("Timestamp=%d,Latitude=%f,Longitude=%f", iTime, gps_lat, gps_lon);

    m_Comms.Notify("GPS_SIM_UPDATE_RECEIVED", sMsg);
    m_Comms.Notify("GPS_X", nav_x);
    m_Comms.Notify("GPS_Y", nav_y);
    m_Comms.Notify("GPS_LONG", gps_lon);
    m_Comms.Notify("GPS_LAT", gps_lat);

    MOOSTrace("Sent GPS update\n");

    return;
}
