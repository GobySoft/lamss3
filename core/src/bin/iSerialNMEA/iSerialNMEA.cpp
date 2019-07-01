// t. schneider tes@mit.edu 10.1.07
// laboratory for autonomous marine sensing systems
// massachusetts institute of technology 
// 
// this is iSerialNMEA.cpp 
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
//// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.


#include <sstream>
#include <iomanip>

#include <boost/algorithm/string.hpp>


#include "iSerialNMEA.h"

#include "goby/util/debug_logger.h"
using namespace goby::util::logger;
#include "goby/time/legacy.h"
using goby::common::ptime2unix_double;
using goby::util::Colors;

using goby::util::glogger;

iSerialNMEAConfig CiSerialNMEA::cfg_;
CiSerialNMEA* CiSerialNMEA::inst_ = 0;

CiSerialNMEA* CiSerialNMEA::get_instance()
{
    if(!inst_)
        inst_ = new CiSerialNMEA();
    return inst_;
}

CiSerialNMEA::CiSerialNMEA()
    : goby::moos::GobyMOOSApp(&cfg_),
      m_nav_x(0),
      m_nav_y(0),
      m_lat(0),
      m_long(0),
      m_depth(0),
      m_nav_speed(0),
      m_nav_heading(0),
      m_last_post_time(-1),
      m_read_db(false),
      m_read_port(false),
      m_last_x(0),
      m_last_y(0),
      m_last_time(0),
      m_i(0)
{
    for (int i=0; i<SPEED_POINTS; i++)
        m_speed[i] = 0;

    goby::glog.is(VERBOSE) && goby::glog  << "reading our configuration..." << std::endl;
    read_configuration();
    goby::glog.is(VERBOSE) && goby::glog  << "\t...success." << std::endl;

    goby::glog.is(VERBOSE) && goby::glog  << "doing subscriptions..." << std::endl;
    do_subscriptions();
    goby::glog.is(VERBOSE) && goby::glog  << "\t...success." << std::endl;
}

void CiSerialNMEA::inbox(const CMOOSMsg& msg)
{
    handleNMEA(msg.GetString());
}


// Iterate: called AppTick times per second
void CiSerialNMEA::loop()
{
    if (m_read_port)
    {
        std::string in;
        while(serial_->readline(&in))
        {
            boost::trim(in);
            publish(cfg_.raw_var_to_write(), in);
            handleNMEA(in);
        }
    }
}

void CiSerialNMEA::handleNMEA(const std::string& s)
{
    goby::glog.is(VERBOSE) && goby::glog  << group("serial") << s << std::endl;
    try
    {    
      goby::util::NMEASentence::strategy strat = cfg_.require_checksum() ? 
	goby::util::NMEASentence::REQUIRE :	
	goby::util::NMEASentence::IGNORE;

      goby::util::NMEASentence nmea(s, strat);
        parseNMEA(nmea);
    }
    catch(std::runtime_error& e)
    {
        goby::glog.is(VERBOSE) && goby::glog  << warn << e.what() << std::endl;
    }
}


void CiSerialNMEA::read_configuration()
{
    if(cfg_.input() == iSerialNMEAConfig::INPUT_MOOSDB)
        m_read_db = true;
    else if (cfg_.input() == iSerialNMEAConfig::INPUT_PORT)
        m_read_port = true;
    
    if (m_read_port)
    {
        if(cfg_.connection_type() == iSerialNMEAConfig::CONNECTION_SERIAL)
        {            
            serial_ = new goby::util::SerialClient(cfg_.serial_port(), cfg_.serial_baud());
            goby::glog.is(VERBOSE) && goby::glog   << "opening serial port " << cfg_.serial_port()
                       << " @ " << cfg_.serial_baud()  << std::endl;

        }
        else if(cfg_.connection_type() == iSerialNMEAConfig::CONNECTION_TCP_AS_CLIENT)
        {            
            goby::glog.is(VERBOSE) && goby::glog  << "opening tcp client: " << cfg_.tcp_server() << ":" << cfg_.tcp_port() << std::endl;            
            serial_ = new goby::util::TCPClient(cfg_.tcp_server(), cfg_.tcp_port());
        }
        else if(cfg_.connection_type() == iSerialNMEAConfig::CONNECTION_TCP_AS_SERVER)
        {            
            goby::glog.is(VERBOSE) && goby::glog  << "opening tcp server: " << cfg_.tcp_port() << std::endl;            
            serial_ = new goby::util::TCPServer(cfg_.tcp_port());
        }
        else
        {
            goby::glog.is(VERBOSE) && goby::glog  << die << "unsupported connection type" << std::endl;
        }
        
        serial_->start();
    }
    
    goby::glog.is(VERBOSE) && goby::glog  << "reading in geodesy information: " << std::endl;
    if (!cfg_.common().has_lat_origin() || !cfg_.common().has_lon_origin())
    {
        goby::glog.is(VERBOSE) && goby::glog  << die << "no lat_origin or lon_origin specified in configuration. this is required for geodesic conversion" << std::endl;
    }
    else
    {
        if(m_geodesy.Initialise(cfg_.common().lat_origin(), cfg_.common().lon_origin()))
            goby::glog.is(VERBOSE) && goby::glog  << "success!" << std::endl;
        else
            goby::glog.is(VERBOSE) && goby::glog  << die << "could not initialize geodesy" << std::endl;
    }

    goby::glog.add_group("serial", Colors::lt_magenta, "serial feed");
    goby::glog.add_group("report", Colors::lt_blue, "report feed");
}

void CiSerialNMEA::do_subscriptions()
{
    if (m_read_db) subscribe(cfg_.raw_var_to_read());
}

bool CiSerialNMEA::parseNMEA(const goby::util::NMEASentence& nmea)
{
    // let's see what we can get
    // by the end we want lat (to get x), long (to get y)
    // heading, speed

    // process different types of NMEA strings
    // position
    bool ok = true;
    bool got_position = false;
 
    if(nmea.sentence_id() == "GGA")
    {
        if (!processGGA(nmea))
            ok = false;
        else
            got_position = true;
    }
    
    else if (MOOSStrCmp(nmea.sentence_id(), "GLL"))
    {
        if(!processGLL(nmea))
            ok = false;
        else
            got_position = true;            
    }
    else if (MOOSStrCmp(nmea.sentence_id(), "RMC"))
    {
      if(!processRMC(nmea))
            ok = false;
        else
            got_position = true;            
    }
// heading true
    else if (MOOSStrCmp(nmea.sentence_id(), "HDT"))
    {
        if(!processHDT(nmea))
            ok = false;
    }
//    else if (MOOSStrCmp(nmea.sentence_id(), "VTG"))
//    {
//        if(!processVTG(nmea))
//            ok = false;
//    }

// proprietary types
    else if (MOOSStrCmp(nmea[0], "PASHR"))
    {
        if(!processPASHR(nmea))
            ok = false;    
    }
    // NURC OEX
    else if (MOOSStrCmp(nmea[0], "PSIMSSB"))
    {
        if(!processPSIMSSB(nmea))
            ok = false;        
    }


    // if you get a new x,y then try to guess
    if(got_position)
    {
//        cout << "m_i: " << m_i << "\n" << "m_speed: " << m_speed[0] << "\n";
  
        // estimate speed from a few last positions
        double dx = m_last_x - m_nav_x;
        double dy = m_last_y - m_nav_y;
        double dt = m_best_time - m_last_time;


        //       cout << "m_best_time: " << doubleToString(m_best_time) << endl;
        //  cout << "m_last_time: " << doubleToString(m_last_time) << endl;

        m_speed[m_i] = sqrt(dx*dx + dy*dy) / dt;

    
        m_last_x = m_nav_x;
        m_last_y = m_nav_y;
        m_last_time = m_best_time;

        
        m_i ++;
        if (m_i >= SPEED_POINTS)
            m_i = 0;

        m_nav_speed = 0;
        int i;
        for(i = 0; i < SPEED_POINTS; i++)
            m_nav_speed += m_speed[i] / SPEED_POINTS;    
    }
    

    double moos_time = MOOSTime();
    
    // borrowed from TransponderAIS.cpp
    // If (a) this is the first chance to post, or (b) there is no
    // blackout interval being implemented, or (c) the time since last
    // post exceeds the blackout interval, then perform a posting.

        
    if(ok && ((m_last_post_time == -1) || (cfg_.wait() <= 0) ||
	      ((moos_time-m_last_post_time) > cfg_.wait())))
    {
        std::string summary = assembleAIS(cfg_.vehicle_name(),
                                          cfg_.vehicle_type(),
                                          -1,
                                          m_best_time,
                                          m_nav_x,
                                          m_nav_y,
                                          m_lat,
                                          m_long,
                                          m_nav_speed,
                                          m_nav_heading,
                                          cfg_.has_fixed_depth() ? cfg_.fixed_depth() : m_depth);
        

        goby::glog.is(VERBOSE) && goby::glog  << group("report") << summary << std::endl;        
        m_Comms.Notify(cfg_.out_status_var(), summary);
      
        m_last_post_time = moos_time;
    }
    
    return true;
    
}



// parse GGA string
// 
bool CiSerialNMEA::processGGA(const goby::util::NMEASentence& nmea)
{
    goby::glog.is(VERBOSE) && goby::glog  << "iSerialNMEA: GGA type string" << std::endl;
    
    if(nmea.size() < 6)
      return false;

    m_best_time = ptime2unix_double(GPStime2ptime(nmea[1]));
    
    std::string lat_deg;
    std::string lat_min;

    if(nmea[2].length() > 2)
    {
        lat_deg = nmea[2].substr(0, 2);
        lat_min = nmea[2].substr(2, nmea[2].size());
    }
    else
        return false;
    
    m_lat = atof(lat_deg.c_str()) + (atof(lat_min.c_str()) / 60);

    if (MOOSStrCmp("S", nmea[3]))
        m_lat = -m_lat;
    
    std::string long_deg;
    std::string long_min;

    if(nmea[4].length() > 3)
    {
        long_deg = nmea[4].substr(0, 3);
        long_min = nmea[4].substr(3, nmea[4].size());
    }
    else
        return false;
    
    
    m_long = atof(long_deg.c_str())+(atof(long_min.c_str()) / 60);

    if (MOOSStrCmp("W", nmea[5]))
        m_long = -m_long;

    m_depth = -nmea.as<double>(9); // altitude

    // convert lat, long into x, y.
    if(!m_geodesy.LatLong2LocalUTM(m_lat, m_long, m_nav_y, m_nav_x))
        return false;

    return true;
}

// parse GGA string
// 
bool CiSerialNMEA::processRMC(const goby::util::NMEASentence& nmea)
{
    goby::glog.is(VERBOSE) && goby::glog  << "iSerialNMEA: RMC type string" << std::endl;
    
    if(nmea.size() < 6)
      return false;

    m_best_time = ptime2unix_double(GPStime2ptime(nmea[1]));
    
    std::string lat_deg;
    std::string lat_min;

    if(nmea[3].length() > 2)
    {
        lat_deg = nmea[3].substr(0, 2);
        lat_min = nmea[3].substr(2, nmea[3].size());
    }
    else
        return false;
    
    m_lat = atof(lat_deg.c_str()) + (atof(lat_min.c_str()) / 60);

    if (MOOSStrCmp("S", nmea[4]))
        m_lat = -m_lat;
    
    std::string long_deg;
    std::string long_min;

    if(nmea[5].length() > 3)
    {
        long_deg = nmea[5].substr(0, 3);
        long_min = nmea[5].substr(3, nmea[5].size());
    }
    else
        return false;
    

    m_long = atof(long_deg.c_str())+(atof(long_min.c_str()) / 60);

    if (MOOSStrCmp("W", nmea[6]))
        m_long = -m_long;



    // convert lat, long into x, y.
    if(!m_geodesy.LatLong2LocalUTM(m_lat, m_long, m_nav_y, m_nav_x))
        return false;

    return true;
}



// parse GLL string
// $INGLL,4405.66432,N,00951.68295,E,134322.30,A,D*77
bool CiSerialNMEA::processGLL(const goby::util::NMEASentence& nmea)
{
    goby::glog.is(VERBOSE) && goby::glog  << "iSerialNMEA: GLL type string" << std::endl;

    
    if(nmea.size() < 5)
      return false;


    if (MOOSStrCmp(nmea[1],""))
        return false;
    
    std::string lat_deg;
    std::string lat_min;

    if(nmea[1].length() > 2)
    {
        lat_deg = nmea[1].substr(0, 2);
        lat_min = nmea[1].substr(2, nmea[1].size());
    }
    else
        return false;
    
    
    m_lat = atof(lat_deg.c_str()) + (atof(lat_min.c_str()) / 60);

    if (MOOSStrCmp("S", nmea[2]))
        m_lat = -m_lat;



    std::string long_deg;
    std::string long_min;

    if(nmea[3].length() > 3)
    {
        long_deg = nmea[3].substr(0, 3);
        long_min = nmea[3].substr(3, nmea[3].size());
    }
    else
        return false;
    

    m_long = atof(long_deg.c_str())+(atof(long_min.c_str()) / 60);

    if (MOOSStrCmp("W", nmea[4]))
        m_long = -m_long;

    // convert lat, long into x, y.
    if(!m_geodesy.LatLong2LocalUTM(m_lat, m_long, m_nav_y, m_nav_x))
        return false;


    return true;
}



// parse PASHR string
bool CiSerialNMEA::processPASHR(const goby::util::NMEASentence& nmea)
{
    goby::glog.is(VERBOSE) && goby::glog  << "iSerialNMEA: PASHR type string" << std::endl;

    if (MOOSStrCmp(nmea[1], "ATT"))
    {
        m_nav_heading = atof(nmea[3].c_str());         
    }
    if (MOOSStrCmp(nmea[1], "AT2"))
    {
        // nothing that ATT doesn't give
    }


    return true;
}


// parse HDT string
bool CiSerialNMEA::processHDT(const goby::util::NMEASentence& nmea)
{
    goby::glog.is(VERBOSE) && goby::glog  << "iSerialNMEA: HDT type string" << std::endl;
    m_nav_heading = atof(nmea[1].c_str());
    return true;
}

// parse VTG string - track made good
bool CiSerialNMEA::processVTG(const goby::util::NMEASentence& nmea)
{
    goby::glog.is(VERBOSE) && goby::glog  << "iSerialNMEA: VTG type string" << std::endl;
    m_nav_heading = atof(nmea[1].c_str());
    return true;
}



// parse OEX PSIMSSB string
bool  CiSerialNMEA::processPSIMSSB(const goby::util::NMEASentence& nmea)
{
    goby::glog.is(VERBOSE) && goby::glog  << "iSerialNMEA: OEX PSIMSSB type string" << std::endl;
    
    if(MOOSStrCmp(nmea[3], "V") || MOOSStrCmp(nmea[4], "NRy"))
        return MOOSFail("iSerialNMEA: PSIMSSB string no good\n");

    // fill this in
    
    return true;
    
}


//------------------------------------------------------------------
// Procedure: assembleAIS
// Purpose: builds the string used for AIS_REPORT
// tes 11.19.07

std::string CiSerialNMEA::assembleAIS(std::string name,
                                      std::string type,
                                      double db_time,
                                      double utc_time,
                                      double x,
                                      double y,
                                      double lat,
                                      double lon,
                                      double spd,
                                      double hdg,
                                      double depth)
{
    std::stringstream summary;
    summary <<  "NAME=" << name
            << ",TYPE=" << type
            << ",MOOSDB_TIME=" << std::setprecision(15) << db_time
            << ",UTC_TIME=" << std::setprecision(15) << utc_time
            << ",X=" <<  x
            << ",Y="   << y
            << ",LAT=" << lat
            << ",LON=" << lon
            << ",SPD=" << spd
            << ",HDG=" << hdg
            << ",DEPTH=" << depth;
    
    return summary.str();

}

boost::posix_time::ptime CiSerialNMEA::GPStime2ptime(const std::string& mt)
{   
    using namespace boost::posix_time;
    using namespace boost::gregorian;

    // HHMMSS.SS
    if(mt.length() < 6)
        return ptime(not_a_date_time);  
    else
    {
        // HHMMSS, HHMMSS.SSSS
        std::string s_hour = mt.substr(0,2), s_min = mt.substr(2,2), s_sec = mt.substr(4,2), s_fs = "0";
        if(mt.length() > 7)
	  s_fs = mt.substr(7);
	        
        try
        {
            int hour = boost::lexical_cast<int>(s_hour);
            int min = boost::lexical_cast<int>(s_min);
            int sec = boost::lexical_cast<int>(s_sec);
            int micro_sec = boost::lexical_cast<int>(s_fs)*pow(10, 6-s_fs.size());
            
            return ptime(date(day_clock::universal_day()), boost::posix_time::time_duration(hour, min, sec, 0)) + microseconds(micro_sec);
        }
        catch (boost::bad_lexical_cast&)
        {
            return ptime(not_a_date_time);
        }        
    }
}
