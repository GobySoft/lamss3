// Copyright 2015 
// Henrik Schmidt
// Toby Schneider 
// Massachusetts Institute of Technology 

#ifndef LamssCBF20150402H
#define LamssCBF20150402H

#include <boost/circular_buffer.hpp>
#include "LamssCBF_fsm.h"
#include "goby/moos/goby_moos_app.h"
#include "MBUtils.h"

class CBFException : public std::runtime_error
{
  public:
  CBFException(const std::string& what) : std::runtime_error(what)
    { }
};

class LamssCBF : public GobyMOOSApp
{
  public:
    static void* driver_library_handle_;
    static LamssCBF* get_instance();

  private:
    LamssCBF(LamssCBFConfig& cfg);
    ~LamssCBF() { }
    LamssCBF(const LamssCBF&);
    LamssCBF& operator=(const LamssCBF&);

    // synchronous event
    void loop();    
    void slow_loop(double curr_time);
    void try_publish();

    // mail handlers

    void handle_sonar_control(const CMOOSMsg& msg);
    void handle_sonar_process(const CMOOSMsg& msg);

    // queue_event for FSM
    template<typename Event>
        void queue_event()
    { scheduler_.queue_event(fsm_, boost::intrusive_ptr<Event>(new Event())); }
  private:
    LamssCBFConfig& cfg_;
    boost::statechart::fifo_scheduler<> scheduler_;
    boost::statechart::fifo_scheduler<>::processor_handle fsm_;
    boost::shared_ptr<boost::thread> fsm_thread_;

    double last_slow_loop_t_;
    
    static LamssCBF* inst_;    

    //prosecute mode stuff
    bool sonar_on_;
    bool processing_on_;

    // local Utilities    
    int str2dbl_vec(std::string, std::vector<double>& values);
    bool dbl_vec2str(std::vector<double> numbers, std::string& str);
    double angle360(double hdg);
    
};

#endif
