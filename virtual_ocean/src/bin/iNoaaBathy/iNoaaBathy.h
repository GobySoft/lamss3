// Copyright 2018-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef iNoaaBathyH
#define iNoaaBathyH

#include "MBUtils.h"
#include "MOOS/libMOOS/MOOSLib.h"
#include "goby/moos/moos_geodesy.h"
#include "lamss/virtual_ocean/henrik_util/CLamssNetcdf.h"
#include <cfloat>
#include <cmath>
#include <netcdf.h>

#ifdef LAMSS_VIRTUAL_OCEAN_HAVE_NETSIM
#include "netsim/acousticstoolbox/svp_request_response.pb.h"
#endif

class CiNoaaBathy : public CMOOSApp
{
  public:
    CiNoaaBathy();
    virtual ~CiNoaaBathy();

  private:
    bool OnNewMail(MOOSMSG_LIST& NewMail);
    bool Iterate();
    bool OnConnectToServer();
    bool OnStartUp();

    void RegisterVariables();
    bool ReadConfiguration();

    std::vector<double> get_bathy(double auv_lat_, double auv_lon_);

    /////
    //handle request/response for bathy at range points
    void handle_outside_bathy_request(const CMOOSMsg& msg);
    int bathy_request_id_;
    std::vector<double> bathy_request_lat_vec_;
    std::vector<double> bathy_request_lon_vec_;
    std::vector<double> lat_vec_;
    std::vector<double> lon_vec_;
    bool got_bathy_request_;

    void send_bathy_response();
    bool bathy_response_sent_;
    /////

    double auv_lat_;
    double auv_lon_;
    double auv_x_;
    double auv_y_;
    int ncid_;
    bool nc_is_open_;
    int latid_;
    int lonid_;
    int bathyid_;
    int lat_varid_;
    int lon_varid_;
    int bathy_varid_;

    double bathy_z_;
    double min_lat_val_, min_lat_val2_, min_lat_val3_;
    double min_lon_val_, min_lon_val2_, min_lon_val3_;

    std::string nc_bathy_file_;
    double publish_interval_;
    double start_time_;
    int publish_count_;

    lamss::virtual_ocean::LamssNetCdf NOAAbathy;
    goby::moos::CMOOSGeodesy geodesy_;
};

#endif
