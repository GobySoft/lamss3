// Copyright 2015 
// Henrik Schmidt
// Toby Schneider 
// Massachusetts Institute of Technology 

#ifndef LamssAEL20151221H
#define LamssAEL20151221H

#include "goby/moos/goby_moos_app.h"
#include "MBUtils.h"
#include "goby/moos/moos_geodesy.h"

#include "lamss/lib_henrik_util/CMeasAvg.h"
#include "goby/moos/moos_protobuf_helpers.h"
#include "goby/middleware/protobuf/frontseat.pb.h"
#include "goby/middleware/frontseat/bluefin/bluefin.pb.h"

#include "LamssAEL_config.pb.h"
#include "lamss/lib_lamss_protobuf/ArrayNavigation.pb.h"

class AELException : public std::runtime_error
{
  public:
  AELException(const std::string& what) : std::runtime_error(what)
    { }
};

class Nav
{
 public:

  double nav_x;
  double nav_y;
  double nav_hdg;
  double nav_dep;
  double nav_pitch;
  double nav_roll;
  double nav_spd;
  double nav_utc;
};

class LamssAEL : public goby::moos::GobyMOOSApp
{
  public:
    static void* driver_library_handle_;
    static LamssAEL* get_instance();

  private:
    LamssAEL(LamssAELConfig& cfg);
    ~LamssAEL() { }

    // synchronous event
    void loop();    
    void slow_loop(double curr_time);

    // mail handlers

    void handle_nav_data(const CMOOSMsg& msg);

    ArrayNavigation computeAEL();
    double angle360(double angle);
  private:
    LamssAELConfig& cfg_;
    double last_slow_loop_t_;
    
    static LamssAEL* inst_;    

    goby::moos::CMOOSGeodesy m_Geodesy;
    CRadianAvg pitch_filter;
    CRadianAvg roll_filter;
    CAngleAvg heading_filter;

    goby::middleware::frontseat::protobuf::NodeStatus last_nav_;
    int nav_count_;
    std::vector<Nav> nav_hist;

    bool filt_set_;
    bool got_nav_;
    int filter_length_;

};

#endif
