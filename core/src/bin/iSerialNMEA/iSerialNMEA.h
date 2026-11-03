// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ISERIALNMEAH
#define ISERIALNMEAH

#include <cmath>
#include <iostream>
#include <iterator>
#include <string>

#include <boost/date_time.hpp>

#include "goby/version.h"
#if GOBY_VERSION_MAJOR >= 2
#include "goby/moos/goby_moos_app.h"
#else
#include "goby/moos/libmoos_util/tes_moos_app.h"
#endif

#include "goby/util/linebasedcomms.h"

#include "iSerialNMEA_config.pb.h"
#include "lamss/core/henrik_util/CMeasAvg.h"

#include "goby/moos/moos_geodesy.h"

// number of last speeds to average (where a speed is the time between two GPS points)
// #define SPEED_POINTS 5

class CiSerialNMEA :
#if GOBY_VERSION_MAJOR < 2
    public TesMoosApp
{
    typedef TesMoosApp goby::moos::GobyMOOSApp;
#else
    public goby::moos::GobyMOOSApp
{
#endif
  public:
    static CiSerialNMEA* get_instance();

  private:
    CiSerialNMEA();
    void inbox(const CMOOSMsg& msg);
    void loop();
    void read_configuration();
    void do_subscriptions();

    void subscribe(const std::string& var)
    {
        goby::moos::GobyMOOSApp::subscribe(var, &CiSerialNMEA::inbox, this);
    }

    std::string assembleAIS(std::string name, std::string type, double db_time, double utc_time,
                            double x, double y, double lat, double lon, double spd, double hdg,
                            double depth);

    void handleNMEA(const std::string&);
    bool parseNMEA(const goby::util::NMEASentence&);

    bool processGGA(const goby::util::NMEASentence&);
    bool processRMC(const goby::util::NMEASentence&);
    bool processHDT(const goby::util::NMEASentence&);
    bool processVTG(const goby::util::NMEASentence&);
    bool processGLL(const goby::util::NMEASentence&);
    bool processPASHR(const goby::util::NMEASentence&);
    bool processPSIMSSB(const goby::util::NMEASentence&);

    bool estimateSpeed();
    bool estimateHeading();

    boost::posix_time::ptime GPStime2ptime(const std::string& mt);

    double m_nav_x;
    double m_nav_y;
    double m_lat;
    double m_long;
    double m_depth;
    double m_nav_speed;
    double m_nav_heading;
    double m_last_post_time;

    bool m_read_db;
    bool m_read_port;
    bool m_first_nav;

    // newest timestamp available from the serial stream
    double m_best_time;

    double m_last_x;
    double m_last_y;
    double m_last_time;

    int m_i;

    //    double m_speed[SPEED_POINTS];

    goby::moos::CMOOSGeodesy m_geodesy;

    goby::util::LineBasedInterface* serial_;

    static lamss::core::iSerialNMEAConfig cfg_;
    static CiSerialNMEA* inst_;

    // GPS filters
    lamss::core::CMeasAvg m_gps_x_filt;
    lamss::core::CMeasAvg m_gps_y_filt;
    // Mooring speed filters
    lamss::core::CMeasAvg m_x_spd_filt;
    lamss::core::CMeasAvg m_y_spd_filt;
};

#endif
