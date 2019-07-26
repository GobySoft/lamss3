// t. schneider tes@mit.edu 10.1.07
// laboratory for autonomous marine sensing systems
// massachusetts institute of technology 
// 
// this is iSerialNMEA.h 
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

#ifndef ISERIALNMEAH
#define ISERIALNMEAH

#include <string>
#include <cmath>
#include <iostream>
#include <iterator>

#include <boost/date_time.hpp>

#include "goby/version.h"
#if GOBY_VERSION_MAJOR >= 2 
 #include "goby/moos/goby_moos_app.h"
#else
 #include "goby/moos/libmoos_util/tes_moos_app.h"
#endif

#include "goby/util/linebasedcomms.h"

#include "iSerialNMEA_config.pb.h"

#include "MOOS/libMOOSGeodesy/MOOSGeodesy.h"

// number of last speeds to average (where a speed is the time between two GPS points)
#define SPEED_POINTS 5

class CiSerialNMEA :
#if GOBY_VERSION_MAJOR < 2
   public TesMoosApp {
    typedef TesMoosApp goby::moos::GobyMOOSApp;
#else
   public goby::moos::GobyMOOSApp  {       
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
    { goby::moos::GobyMOOSApp::subscribe(var, &CiSerialNMEA::inbox, this); }

    
    std::string assembleAIS(std::string name,
                            std::string type,
                            double db_time,
                            double utc_time,
                            double x,
                            double y,
                            double lat,
                            double lon,
                            double spd,
                            double hdg,
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

    
    // newest timestamp available from the serial stream
    double m_best_time;
    
    
    double m_last_x;
    double m_last_y;
    double m_last_time;
    
    
    int m_i;

    double m_speed[SPEED_POINTS];

    CMOOSGeodesy m_geodesy;

    goby::util::LineBasedInterface* serial_;
    
    static iSerialNMEAConfig cfg_;
    static CiSerialNMEA* inst_;    

};

#endif 
