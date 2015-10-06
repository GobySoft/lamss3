// Copyright 2015 
// Henrik Schmidt
// Toby Schneider 
// Massachusetts Institute of Technology 

#ifndef LamssTrackManager20150402H
#define LamssTrackManager20150402H

#include <boost/circular_buffer.hpp>

#include "LamssTrackManager_fsm.h"
#include "goby/moos/goby_moos_app.h"
#include "LamssTrackManager_config.pb.h"
#include "MBUtils.h"
#include "ivp/MOOSGeodesy.h"

class TrackManagerException : public std::runtime_error
{
  public:
  TrackManagerException(const std::string& what) : std::runtime_error(what)
    { }
};

class LamssTrackManager : public GobyMOOSApp
{
  public:
    static void* driver_library_handle_;
    static LamssTrackManager* get_instance();

  private:
    LamssTrackManager(LamssTrackManagerConfig& cfg);
    ~LamssTrackManager() { }
    LamssTrackManager(const LamssTrackManager&);
    LamssTrackManager& operator=(const LamssTrackManager&);

    CMOOSGeodesy m_Geodesy;

    
    // synchronous event
    void loop();    
    void slow_loop(double curr_time);
    void try_publish();
    void send_deploy();

    // mail handlers

    void handle_ifs_data(const CMOOSMsg& msg);
    void handle_deploy(const CMOOSMsg& msg);
    void handle_prosecute(const CMOOSMsg& msg);
    void handle_vehicle_id(const CMOOSMsg& msg);
    void handle_mission_state(const CMOOSMsg& msg);

    // queue_event for FSM
    template<typename Event>
        void queue_event()
    { scheduler_.queue_event(fsm_, boost::intrusive_ptr<Event>(new Event())); }
  private:
    LamssTrackManagerConfig& cfg_;
    boost::statechart::fifo_scheduler<> scheduler_;
    boost::statechart::fifo_scheduler<>::processor_handle fsm_;
    boost::shared_ptr<boost::thread> fsm_thread_;

    double last_slow_loop_t_;
    
    goby::moos::protobuf::NodeStatus last_nav_;

    static LamssTrackManager* inst_;    

};

#endif
