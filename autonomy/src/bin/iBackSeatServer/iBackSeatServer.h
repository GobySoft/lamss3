// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __CiBackSeatServer_h__
#define __CiBackSeatServer_h__

#include "MOOS/libMOOS/MOOSLib.h"
#include "goby/moos/moos_geodesy.h"
#include "goby/util/linebasedcomms.h"
#include "goby/util/linebasedcomms/nmea_sentence.h"
#include <memory>
#include <random>
#include <string>
#include <tuple>

class CiBackSeatServer : public CMOOSApp
{
  public:
    CiBackSeatServer();
    virtual ~CiBackSeatServer();

    bool OnNewMail(MOOSMSG_LIST& NewMail);
    bool Iterate();
    bool OnConnectToServer();
    bool OnStartUp();

  private:
    goby::moos::CMOOSGeodesy m_Geodesy;
    // insert local vars here
    std::string SecondsSinceMidnight(int nDigits);
    std::string gen_NMEAchecksum(const char* str);
    void Translate(const goby::util::NMEASentence& sentence);
    void check_send_dvl();

    std::string tcp_port;
    std::unique_ptr<goby::util::TCPServer> tcp_server;

    std::string navx;
    std::string navy;
    std::string navz;
    std::string heading;
    std::string depth;
    std::string speed;
    std::string yaw;
    std::string pitch;
    std::string altitude_str;
    std::string pitch_rate;
    std::string yaw_rate;

    double nav_x;
    double nav_y;
    double nav_z;
    double course;
    double dep;
    double spd;
    double yw;
    double pit;
    double last_bathy_z;
    double altitude;

    double prev_pit;
    double prev_yw;
    double pit_time;
    double yw_time;

    bool msgack;

    double last_gps_sim_update_time;

    // DVL u, v, w
    std::tuple<double, double, double> sim_dvl_values{std::numeric_limits<double>::quiet_NaN(),
                                                      std::numeric_limits<double>::quiet_NaN(),
                                                      std::numeric_limits<double>::quiet_NaN()};

    // Ice-tracking DVL
    double dvl_ice_track;
    double ice_roughness;
    double ice_thickness;
    double ice_roughness_mean;
    double ice_roughness_stdev;
    std::default_random_engine generator;
    std::normal_distribution<double> ice_roughness_dist;

    // Simulating a lost DVL lock
    bool simulate_dvl_loss;
    double dvl_lost_interval;
    double simulate_dvl_loss_start_time;

    // Simulating DVL min/max range
    double simulate_dvl_range;
    double dvl_min_range;
    double dvl_max_range;

    //goby::util::TCPServer tcp_server(as<unsigned>(std::string()));
};

#endif /* __CiBackSeatServer_h__ */
