// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#if !defined(AFX_SIMPLEAUVSIM_H__179D7D4B_5915_4664_AEEA_8B81C324A189__INCLUDED_)
#define AFX_SIMPLEAUVSIM_H__179D7D4B_5915_4664_AEEA_8B81C324A189__INCLUDED_

#include "SimParams.h" // Added by ClassView
#include "SixDOFAUV.h" // Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

using namespace NEWMAT;

#include "goby/moos/moos_geodesy.h"
#include <MOOS/libMOOS/Utils/MOOSUtils.h>

#include "AcousticBeacon.h"
#include "AcousticNode.h"
#include "AcousticSignal.h"

#include "SimEnvironment.h" // Added by ClassView
#include <list>
using namespace std;

typedef std::list<CSimEntity*> SIM_ENTITY_LIST;

class CMVSim : public CMOOSApp
{
  public:
    virtual bool OnConnectToServer();
    virtual bool OnStartUp();

    CMVSim();
    virtual ~CMVSim();

  private:
    double dfLatOrigin, dfLongOrigin;
    goby::moos::CMOOSGeodesy m_Geodesy;

  protected:
    bool LogStartConditions();
    bool OpenLogFile();
    bool MakeBeacon(std::string sConfig);
    bool MakeAUV(std::string sConfig);
    bool Clean();
    CSixDOFAUV* GetSubscriber(const std::string& sName);

    bool m_bRealTime;
    CSimEnvironment m_Environment;
    bool m_bUseCurrent;

    SIM_ENTITY_LIST m_Entities;

    bool DoRegistrations();
    void PostResults();
    CSimParams m_Params;
    bool Initialise();
    bool m_bInitialised;
    double m_dfUpdateRate;

    void UTMUpdate();
    void GPSUpdate();
    double drift_x;
    double drift_y;
    double drift_z;
    double previous_gps;
    double gps_x;
    double gps_y;
    double utm_x;
    double utm_y;
    double east_current_est;
    double north_current_est;
    double down_current_est;
    bool gps_update;
    bool utm_update;
    bool dvl_lock;
    double drift_update;

    bool NeedToMail();
    double m_dfLastMailed;
    double m_dfStartTime;
    double m_dfSimulatorTime;
    double m_dfOldSimulatorTime;

    bool buoyancy_requested;
    bool trim_requested;
    double buoyancy_request_time;
    double trim_request_time;
    double buoyancy_delay;
    double max_trim_delay;
    double pitch_degrees;
    double pitch_tolerance;
    double last_report;
    double report_interval;

    bool Iterate();
    bool OnNewMail(MOOSMSG_LIST& NewMail);
};

#endif // !defined(AFX_SIMPLEAUVSIM_H__179D7D4B_5915_4664_AEEA_8B81C324A189__INCLUDED_)
