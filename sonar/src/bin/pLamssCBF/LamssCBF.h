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
#include "lamss/lib_newmat10D/newmatap.h"

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
    void handle_sonar_filename(const CMOOSMsg& msg);
    void handle_sound_speed(const CMOOSMsg& msg);

    void update_beamforming_vars();
    void update_array_configuration();
    void handle_bin_list(std::string bin_list);
    void handle_fc_bw();

    // local Utilities    
    int str2dbl_vec(std::string, std::vector<double>& values);
    bool dbl_vec2str(std::vector<double> numbers, std::string& str);
    double angle360(double hdg);

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

    bool got_filename_;
    std::string sonar_filename_;
    int frame_number_;
    int num_blocks_integrated_;
    int num_files_processed_;

    ColumnVector win;
    ColumnVector windowed_data;
    Matrix       Array_Data;
    Matrix       Spectrum_real;
    Matrix       Spectrum_imag;
    Matrix       S_real;
    Matrix       S_imag;
    ColumnVector win_spatial;
    ColumnVector nominal_relative_array_element_locations; 
    ColumnVector beam_cosines;

    int *array_elements_used;
    int num_elements_used_;
    int *bins;
    int num_bins_;
    
    double sound_speed_;
};

#endif
