// Copyright 2015 
// Henrik Schmidt
// Toby Schneider 
// Massachusetts Institute of Technology 

#ifndef SimTargets20171006H
#define SimTargets20171006H

#include "goby/moos/goby_moos_app.h"
#include "MBUtils.h"
#include "goby/moos/moos_geodesy.h"

#include "goby/moos/moos_protobuf_helpers.h"
#include "goby/middleware/protobuf/frontseat.pb.h"
#include "goby/middleware/frontseat/bluefin/bluefin.pb.h"

#include "SimTargets_config.pb.h"
#include "lamss/lib_lamss_protobuf/lamss_targetsim.pb.h" // DCCL message for self noise
#include "lamss/lib_lamss_protobuf/lamss_pinger_command.pb.h"

// from lib_henrik_util
#include <lamss/lib_henrik_util/CTarget.h>
#include <lamss/lib_henrik_util/parse_stat.h>
#include <lamss/lib_henrik_util/str_num.h>

using namespace std;

class TargetsException : public std::runtime_error
{
  public:
  TargetsException(const std::string& what) : std::runtime_error(what)
    { }
};

class SimTargets : public goby::moos::GobyMOOSApp
{
  public:
    static void* driver_library_handle_;
    static SimTargets* get_instance();

  private:
    SimTargets(SimTargetsConfig& cfg);
    ~SimTargets() { }

    // synchronous event
    void loop();    
    void slow_loop(double curr_time);

    // mail handlers

    void handle_tgt_state(const CMOOSMsg& msg);
    void handle_nav_x(const CMOOSMsg& msg);
    void handle_nav_y(const CMOOSMsg& msg);

    SimTargetsConfig& cfg_;
    double last_slow_loop_t_;

    static SimTargets* inst_;    

 protected:

    void update_target(LAMSS_SIMULATE_TARGET tgt_msg);

    void updateTargetList();
    void cancelTarget(std::string);
    void postTargetList();
    std::string target_AIS(int indx);
    
    bool prosecuting,target_set;
    bool target_init;
    
    double target_utc,target_x,target_y,target_heading,target_speed;
    double time_now,auv_x,auv_y,target_depth,last_time,current_time;
    double elapsed, source_range, rangeCrit, target_x_start, target_y_start;
    int deploy_mission,prosecute_mode, target_number, t_num; 
    bool source_control;
    
    double target_freq;
    double target_spl;
    double target_str;
    double target_bw;
    double dest_id;
    vector<double> tone_freq;
    vector<double> tone_dbl;
    
    bool got_x;
    bool got_y;
    bool noRangeCrit;
    
    goby::moos::CMOOSGeodesy m_Geodesy;
    std::vector<CTarget> Targets;
    
};

#endif
