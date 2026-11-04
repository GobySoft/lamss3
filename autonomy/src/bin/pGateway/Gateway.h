// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GATEWAY_HEADER
#define GATEWAY_HEADER

#include "MBUtils.h"
#include "MOOS/libMOOS/MOOSLib.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"
#include "goby/moos/moos_geodesy.h"
#include <string>

class CGateway : public CMOOSApp
{
  public:
    CGateway();
    virtual ~CGateway();

    bool Iterate();
    bool OnNewMail(MOOSMSG_LIST& NewMail);
    bool OnConnectToServer();
    bool OnStartUp();

  protected:
    goby::moos::CMOOSGeodesy m_Geodesy;
    double start_time, deploy_x, deploy_y, deploy_spd, deploy_hdg, deploy_depth, depth_trigger;
    double deploy_lat, deploy_lon;
    int dest_addr;
    std::string ship_name;
    //  bool UTM2LatLong(double, double, double&, double& );
    double latitude, longitude;
};

#endif
