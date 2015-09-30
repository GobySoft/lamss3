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
#include "MBUtils.h"
#include "ivp/MOOSGeodesy.h"

class TrackManagerException : public std::runtime_error
{
  public:
  TrackManagerException(const std::string& what) : std::runtime_error(what)
    { }
};

class LamssMissionManager : public GobyMOOSApp
{
  public:
    static void* driver_library_handle_;
    static LamssMissionManager* get_instance();

  private:
    LamssMissionManager(lamss::dsop::protobuf::LamssMissionManagerConfig& cfg);
    ~LamssMissionManager() { }
    LamssMissionManager(const LamssMissionManager&);
    LamssMissionManager& operator=(const LamssMissionManager&);

    CMOOSGeodesy m_Geodesy;

    
    // synchronous event
    void loop();    
    void slow_loop(double curr_time);

    void try_publish();
    
    // mail handlers

    void handle_ifs_data(const CMOOSMsg& msg);
    void handle_sonar_control(const CMOOSMsg& msg);
    void update_sonar_mode(DSOP_SONARCONTROL sonar_control);
    void update_sonar_focus(DSOP_SONARCONTROL sonar_control);

    void handle_sonar_track_report(const CMOOSMsg& msg);

    void handle_primary_track_switch(const CMOOSMsg& msg);
    void handle_secondary_track_switch(const CMOOSMsg& msg);

    bool handle_track_switch(const CMOOSMsg& Msg, 
			     std::string & priority_track,
			     std::string & active_priority_track);

    bool PublishNodeReport(std::string track_name,
			   double track_utc,
			   double track_x,
			   double track_y,
			   double track_hdg,
			   double track_dep,
			   double track_spd,
			   double track_conf);
    void validateTracks();
    void postStaleTrack();
    bool createTrackNodeReport(std::string trk_report_msg);
    void resetTrackManager();

    void handle_vehicle_id(const CMOOSMsg& msg);

    // queue_event for FSM
    template<typename Event>
        void queue_event()
    { scheduler_.queue_event(fsm_, boost::intrusive_ptr<Event>(new Event())); }
  private:
    lamss::dsop::protobuf::LamssMissionManagerConfig& cfg_;
    boost::statechart::fifo_scheduler<> scheduler_;
    boost::statechart::fifo_scheduler<>::processor_handle fsm_;
    boost::shared_ptr<boost::thread> fsm_thread_;

    double last_slow_loop_t_;
    
    goby::moos::protobuf::NodeStatus last_nav_;

    // DSOP_SONARCONTROL mode is one of the actual pinging modes
    bool sonar_mode_active_;
    double doppler_threshold_lower_;
    double doppler_threshold_upper_;
    
    //    DSOP_SONARCONTROL last_sonar_control_;    

    static LamssMissionManager* inst_;    

// Henrik's target variables from pActiveSonarControl

    std::string target_focus_;
    double sonar_focus_depth_ ;
    bool valid_track_ ; 
    
    std::string sonar_xy_track_;
    std::string sonar_ro_track_;
    std::string sonar_geo_track_;
    std::string sonar_range_only_track_;
    
    std::string primary_track_key_;
    std::string active_primary_track_;
    bool valid_primary_track_ ; 
    bool got_primary_track_;
    double valid_primary_time_ ;
    
    std::string secondary_track_key_;
    std::string active_secondary_track_;
    bool valid_secondary_track_ ; 
    bool got_secondary_track_;
    

    double last_track_range_;
    double last_track_rate_;
    double last_track_bearing_;
    bool transition_mode_;
    bool got_last_contact_;
    double last_contact_utc_;
    double last_contact_range_;
    double last_contact_rate_;
    double last_contact_x_;
    double last_contact_y_;
    double last_contact_heading_;
    double last_contact_bearing_;
    double last_contact_speed_;
    double max_contact_range_;
    double forecast_decay_;
};

#endif
