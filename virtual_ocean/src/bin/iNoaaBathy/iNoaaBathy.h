// s. petillo spetillo@mit.edu 03.27.09
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
//
// this is iNoaaBathy.h
//
// see the readme file within this directory for information
// pertaining to usage and purpose of this script.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.

#ifndef iNoaaBathyH
#define iNoaaBathyH

#include "MBUtils.h"
#include "MOOS/libMOOS/MOOSLib.h"
#include "goby/moos/moos_geodesy.h"
#include "lamss/lib_henrik_util/CLamssNetcdf.h"
#include <cfloat>
#include <cmath>
#include <netcdf.h>

#include "netsim/acousticstoolbox/svp_request_response.pb.h"

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

    LamssNetCdf NOAAbathy;
    goby::moos::CMOOSGeodesy geodesy_;
};

#endif
