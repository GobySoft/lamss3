// Copyright 2015 
// Henrik Schmidt
// Toby Schneider 
// Massachusetts Institute of Technology 

#ifndef LamssMissionManager20150402H
#define LamssMissionManager20150402H

#include <boost/circular_buffer.hpp>

#include "LamssMissionManager_fsm.h"
#include "goby/moos/goby_moos_app.h"
#include "LamssMissionManager_config.pb.h"

#include "lamss/lib_lamss_protobuf/icex_tracking.pb.h"
#include "lamss/lib_henrik_util/CMeasAvg.h"

//#include "MBUtils.h"
#include "goby/moos/moos_geodesy.h"
#include "XYPolygon.h"

class TrackManagerException : public std::runtime_error
{
  public:
  TrackManagerException(const std::string& what) : std::runtime_error(what)
    { }
};

class CStovepipe
{
 public:
  int ID;
  double utc;
  double x;
  double y;
  double camp_x;
  double camp_y;
  double depth;
  double bearing;
  double length;
  double width;
  int verteces;
};

class LamssMissionManager : public goby::moos::GobyMOOSApp
{
  public:
    static void* driver_library_handle_;
    static LamssMissionManager* get_instance();

  private:
    LamssMissionManager(LamssMissionManagerConfig& cfg);
    ~LamssMissionManager() { }
    LamssMissionManager(const LamssMissionManager&);
    LamssMissionManager& operator=(const LamssMissionManager&);

    goby::moos::CMOOSGeodesy m_Geodesy;
    XYPolygon opregion, opregion_init;
    
    // synchronous event
    void loop();    
    void slow_loop(double curr_time);
    void update_abort_mission();
    void try_publish();
    void send_deploy();
    void homing_prosecute();
    std::string NodeReport(std::string node_name, std::string node_type, 
			   double node_x, double node_y, double node_spd, 
			   double node_hdg, double node_dep );
    void initial_deploy();
    void initial_transit();
    void homing_return();

    // mail handlers

    void handle_ifs_data(const CMOOSMsg& msg);
    void handle_hydroman_data(const CMOOSMsg& msg);
    void handle_status(const CMOOSMsg& msg);
    void handle_deploy(const CMOOSMsg& msg);
    void handle_prosecute(const CMOOSMsg& msg);
    void handle_vehicle_id(const CMOOSMsg& msg);
    void handle_vehicle_name(const CMOOSMsg& msg);
    void handle_collaborator_id(const CMOOSMsg& msg);
    void handle_mission_state(const CMOOSMsg& msg);
    void handle_opbox_distance(const CMOOSMsg& msg);
    void handle_transit_route(const CMOOSMsg& msg);
    void handle_desired_depth(const CMOOSMsg& msg);
    void handle_goto_surface(const CMOOSMsg& msg);
    void handle_tracking(const CMOOSMsg& msg);
    void handle_target_bearing(const CMOOSMsg& msg);
    void handle_survey_flag(const CMOOSMsg& msg);
    void handle_gps_update(const CMOOSMsg& msg);
    void handle_in_track(const CMOOSMsg& msg);
    void handle_tracking_message(const CMOOSMsg& msg);
    void handle_stovepipe_message(const CMOOSMsg& msg);
    void handle_track_signal_update_message(const lamss::icex::protobuf::TrackingSignalUpdate& msg);
    void update_deploy_parameters();
    void post_transit_route();
    // Stovepipe manager
    void update_stovepipes(LAMSS_STOVEPIPE stovepipe_msg);

    // queue_event for FSM
    template<typename Event>
        void queue_event()
    { scheduler_.queue_event(fsm_, boost::intrusive_ptr<Event>(new Event())); }
  private:
    LamssMissionManagerConfig& cfg_;
    boost::statechart::fifo_scheduler<> scheduler_;
    boost::statechart::fifo_scheduler<>::processor_handle fsm_;
    boost::shared_ptr<boost::thread> fsm_thread_;

    double mission_start_time_;
    bool initial_deploy_done_;

    double slow_loop_period_;
    double last_slow_loop_t_;
    
    goby::moos::protobuf::NodeStatus last_nav_;

    static LamssMissionManager* inst_;    
    std::vector<double> transit_x;
    std::vector<double> transit_y;
    
    bool homing_beacon;
    double last_homing_prosecute_;
    bool tracking_;
    double target_bearing_;
    int collaborator_id_;
    
    //
    // Initial camp location
    //
    bool got_first_tracking_msg;
    double camp_x_init;
    double camp_y_init;
    double camp_x_last;
    double camp_y_last;
    double camp_x_deploy;
    double camp_y_deploy;
    
    double last_tracking_msg;
    
    CMeasAvg camp_x_spd_filt;
    CMeasAvg camp_y_spd_filt;
    int filter_length;    
    //
    // Abort location
    //
    double abort_x;
    double abort_y;
    // Offset of abort point (hydrohole) from origin (camp)
    double abort_x_offset;
    double abort_y_offset;

    bool in_track;
    bool depth_adapting;
    // Dynamic stovepipes (following ice in ICEX)
    std::vector<CStovepipe> Stovepipes;
};

#endif
