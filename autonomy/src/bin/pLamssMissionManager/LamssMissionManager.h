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
    LamssMissionManager(LamssMissionManagerConfig& cfg);
    ~LamssMissionManager() { }
    LamssMissionManager(const LamssMissionManager&);
    LamssMissionManager& operator=(const LamssMissionManager&);

    CMOOSGeodesy m_Geodesy;

    
    // synchronous event
    void loop();    
    void slow_loop(double curr_time);
    void update_abort_mission();
    void try_publish();
    void send_deploy();

    // mail handlers

    void handle_ifs_data(const CMOOSMsg& msg);
    void handle_deploy(const CMOOSMsg& msg);
    void handle_prosecute(const CMOOSMsg& msg);
    void handle_vehicle_id(const CMOOSMsg& msg);
    void handle_mission_state(const CMOOSMsg& msg);
    void handle_opbox_distance(const CMOOSMsg& msg);
    void handle_transit_route(const CMOOSMsg& msg);
    void handle_desired_depth(const CMOOSMsg& msg);

    // queue_event for FSM
    template<typename Event>
        void queue_event()
    { scheduler_.queue_event(fsm_, boost::intrusive_ptr<Event>(new Event())); }
  private:
    LamssMissionManagerConfig& cfg_;
    boost::statechart::fifo_scheduler<> scheduler_;
    boost::statechart::fifo_scheduler<>::processor_handle fsm_;
    boost::shared_ptr<boost::thread> fsm_thread_;

    double last_slow_loop_t_;
    
    goby::moos::protobuf::NodeStatus last_nav_;

    static LamssMissionManager* inst_;    

};

#endif
