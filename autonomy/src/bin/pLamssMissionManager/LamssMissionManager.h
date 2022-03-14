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
#include "lamss/lib_lamss_protobuf/icex_tracking_20.pb.h"
#include "lamss/lib_lamss_protobuf/platform_info.pb.h"
#include "lamss/lib_henrik_util/CMeasAvg.h"

//#include "MBUtils.h"
#include "goby/moos/modem_id_convert.h"

#include "goby/moos/moos_geodesy.h"
#include "XYPolygon.h"

// The activation of GEO messages done in CMakeLists.txt"
//#define GEO_COMMAND

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
  double lat;
  double lon;
  double camp_x;
  double camp_y;
  double depth;
  double bearing;
  double length;
  double width;
  int verteces;
  bool spawned;
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

#ifndef GEO_COMMAND
  LAMSS_TRANSIT last_transit_route_;    
  LAMSS_TRANSIT initial_transit_cmd_;    
  LAMSS_DEPLOY last_deploy_cmd_;    
  LAMSS_PROSECUTE last_prosecute_cmd_;    
  void configure_transit_route(LAMSS_TRANSIT transit_cmd);
#else
  LAMSS_TRANSIT_GEO last_transit_route_;    
  LAMSS_TRANSIT_GEO initial_transit_cmd_;    
  LAMSS_DEPLOY_GEO last_deploy_cmd_;    
  LAMSS_PROSECUTE_GEO last_prosecute_cmd_;    
  void configure_transit_route(LAMSS_TRANSIT_GEO transit_cmd);
#endif
  // Stovepipe manager
  void update_stovepipes(LAMSS_STOVEPIPE stovepipe_msg);
  // synchronous event
  void loop();    
  void slow_loop(double curr_time);
  void update_abort_mission();
  void try_publish();
  void UTM_update();
  void update_utm_datum(double curr_time);
  void send_deploy();
  void homing_prosecute();
  std::string NodeReport(std::string node_name, std::string node_type,
			 double utc_time,
			 double node_x, double node_y, double node_spd, 
			 double node_hdg, double node_dep );
  void initial_deploy();
  void initial_transit();
  void homing_return();

  // mail handlers

  void handle_helm_mode(const CMOOSMsg& msg);
  void handle_ifs_data(const CMOOSMsg& msg);
  void handle_hydroman_data(const CMOOSMsg& msg);
  void handle_status(const CMOOSMsg& msg);
  void handle_deploy(const CMOOSMsg& msg);
  void handle_prosecute(const CMOOSMsg& msg);
  void handle_lat_origin(const CMOOSMsg& msg);
  void handle_lon_origin(const CMOOSMsg& msg);
  void handle_vehicle_id(const CMOOSMsg& msg);
  void handle_vehicle_name(const CMOOSMsg& msg);
  void handle_collaborator_id(const CMOOSMsg& msg);
  void handle_collaborator_name(const CMOOSMsg& msg);
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
  void handle_icex20_tracking_message(const CMOOSMsg& msg);
  void handle_stovepipe_geo_message(const CMOOSMsg& msg);
  void handle_stovepipe_message(const CMOOSMsg& msg);
  void handle_homing_lock(const CMOOSMsg& msg);
  void handle_survey_wpt_index(const CMOOSMsg& msg);
  void handle_transit_flag(const CMOOSMsg& msg);
  void handle_transit_wpt_index(const CMOOSMsg& msg);
  void handle_track_signal_update_message(const lamss::icex::protobuf::TrackingSignalUpdate& msg);
  void update_deploy_parameters();
  void deploy_behavior_updates(bool new_deploy);
  void prosecute_behavior_updates(bool new_prosecute);
  void post_transit_route();
  // Stovepipe manager
  void publish_stovepipe(int i,double camp_x,double camp_y);
  void handle_vehicle_transmit_selection(const lamss::icex::protobuf::VehicleTransmitSelection& msg);

  // queue_event for FSM
  template<typename Event>
  void queue_event()
  { scheduler_.queue_event(fsm_, boost::intrusive_ptr<Event>(new Event())); }
private:
  LamssMissionManagerConfig& cfg_;
  boost::statechart::fifo_scheduler<> scheduler_;
  boost::statechart::fifo_scheduler<>::processor_handle fsm_;
  boost::shared_ptr<boost::thread> fsm_thread_;

  std::string helm_mode_;
  bool transit_state_;
  
  double mission_start_time_;
  bool initial_deploy_done_;

  double slow_loop_period_;
  double last_slow_loop_t_;
    
  double lat_origin_;
  double lon_origin_;
  bool  new_origin_;
  double last_utm_update_;
  
  goby::middleware::frontseat::protobuf::NodeStatus last_nav_;

  lamss::protobuf::PlatformInfo platform_info;
    
  static LamssMissionManager* inst_;    
  // Last deploy location 
  double last_deploy_x_;  
  double last_deploy_y_;
  double last_deploy_lon_;
  double last_deploy_lat_;
  // Last AIS prosecution cue
  double last_target_x_;  
  double last_target_y_;
  double last_target_lon_;
  double last_target_lat_;
  
  std::vector<double> transit_x;
  std::vector<double> transit_y;
  std::vector<double> transit_lon;
  std::vector<double> transit_lat;
  std::vector<double> initial_transit_x;
  std::vector<double> initial_transit_y;
  std::vector<double> initial_transit_lon;
  std::vector<double> initial_transit_lat;
    

  bool homing_beacon;
  double last_homing_prosecute_;
  bool tracking_;
  double target_bearing_;
  int collaborator_id_;
  std::string collaborator_name;
    
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
  double camp_lat_init;
  double camp_lon_init;
  double camp_lat_last;
  double camp_lon_last;
  double camp_lat_deploy;
  double camp_lon_deploy;
    
  double last_tracking_msg;
  bool got_gps_nav;    
  CMeasAvg camp_x_spd_filt;
  CMeasAvg camp_y_spd_filt;
  // Mooring speed filters
  std::vector<CMeasAvg> h_x_spd_filt;
  std::vector<CMeasAvg> h_y_spd_filt;
  std::vector<double> h_x_last;
  std::vector<double> h_y_last;
  int filter_length;    
  //
  // Abort location
  //
  double abort_x;
  double abort_y;
  double abort_lat;
  double abort_lon;
  // Offset of abort point (hydrohole) from origin (camp)
  double abort_x_offset;
  double abort_y_offset;
  bool homing_lock;
    
  int transit_wpt_index;
  bool new_transit_wpt_index;
  int survey_wpt_index;
  bool new_wpt_index;
    
  bool in_track;
  bool depth_adapting;
  // Dynamic stovepipes (following ice in ICEX)
  std::vector<CStovepipe> Stovepipes;

  // Keep track of current transmit modem (10kHz or 25kHz)
  lamss::icex::protobuf::VehicleTransmitSelection vehicle_transmit_;

  // all modem ids for this vehicle (vehicle_id_ plus additional configured)
  std::set<int> ownship_modem_ids_;

  // last update time for dynamic OpRegion
  double last_opregion_update_time_{0};
};

#endif
