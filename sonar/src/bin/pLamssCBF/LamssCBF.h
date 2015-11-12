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

#include "lamss/lib_lamss_protobuf/lamss_btr.pb.h"
#include "lamss/lib_henrik_util/CMeasAvg.h"
class CBFException : public std::runtime_error
{
  public:
  CBFException(const std::string& what) : std::runtime_error(what)
    { }
};

class AngleFilter
{
    public:

        AngleFilter(int length);
        ~AngleFilter();

        double get_FVal();
        void set_RawVal(double);

    protected:

        int filter_length;
        int num_measurements;
        double avg;

        deque<double>  svals;
        deque<double>  cvals;
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
    void handle_sonar_dir(const CMOOSMsg& msg);
    void handle_sonar_file_num(const CMOOSMsg& msg);
    void handle_sound_speed(const CMOOSMsg& msg);
    void handle_ael_pitch(const CMOOSMsg& msg);
    void handle_homing_active(const CMOOSMsg& msg);
    void handle_homing_beacon_frequency(const CMOOSMsg& msg);

    void update_beamforming_vars();
    void array_configuration();
    void handle_bin_list(std::string bin_list);
    void handle_fc_bw();
    // Processing modules
    bool LoadData(std::string);
    void hanning(Real *,int);
    void hamming(Real *,int);  //Added AJP 21 JUL 2010
    int  round(double);
    bool Process_Data();
    bool initialize_beamformer();
    bool generate_spatial_window();
    void steering_parameters();
    void zeros(double *,int);
    void clear_buffers();
    void compute_steering_vectors();
    size_t parseArrayCoordinates(std::string str, vector<double> & element_coordinate);
    //variables used to generate steering vectors using new convention
    Matrix       steering_vector_real;
    Matrix       steering_vector_imag;    
    ColumnVector azimuthal_beams;
    ColumnVector elevation_beams;
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
    LamssCBFConfig curr_cfg_;
    boost::statechart::fifo_scheduler<> scheduler_;
    boost::statechart::fifo_scheduler<>::processor_handle fsm_;
    boost::shared_ptr<boost::thread> fsm_thread_;

    double last_slow_loop_t_;
    
    static LamssCBF* inst_;    

    CRadianAvg pitch_filter;
    CAngleAvg heading_filter;

    bool filt_set_;
    bool got_pitch_;
    int filter_length_;

    //prosecute mode stuff
    bool sonar_on_;
    bool processing_on_;

    bool new_file_flag_;
    bool got_filename_;
    std::string sonar_filename_;
    bool got_dir_;
    std::string sonar_dir_;
    int frame_number_;
    int last_frame_processed_;
    int buff_index_;
    int end_data_;

    int num_blocks_integrated_;
    int int_blocks_;
    int num_files_processed_;

    double actual_integration_;
    double btr_normalization_;
    double num_overlapping_samples_;

    // newmat arrays for beamformers
    ColumnVector win;
    ColumnVector windowed_data;
    Matrix       Array_Data;
    ColumnVector win_spatial;
    ColumnVector nominal_element_locations; 
    ColumnVector beam_cosines;
    ColumnVector array_x;
    ColumnVector array_y;
    ColumnVector array_z;
    ColumnVector bin_center_frequencies;

    int *array_elements_used;
    int num_elements_used_;
    int *bins;
    int num_bins_;
    // 1D beamformer
    Matrix       S_real;
    Matrix       S_imag;

    // 2D beamformer
    Matrix       Spectrum_real;
    Matrix       Spectrum_imag;
    Matrix P;

    std::vector<double> beam_angles;
	
    double time_window_constant_;
    double hydrophone_amplitude_gain_;
    double sound_speed_;

    double homing_beacon_frequency_;
    int homing_frequency_bin_;
    bool homing_mode_active_;
    std::string homing_bin_list_;
};

#endif
