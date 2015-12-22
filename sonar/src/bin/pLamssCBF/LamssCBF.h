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
#include "boost/multi_array.hpp"

#include "lamss/lib_lamss_protobuf/lamss_btr.pb.h"
#include "lamss/lib_henrik_util/CMeasAvg.h"

#include "lamss/lib_lamss_protobuf/BeamformingVars.pb.h"
#include "lamss/lib_lamss_protobuf/ActiveSonarParameters.pb.h"
#include "lamss/lib_lamss_protobuf/SonarProcessingCommand.pb.h"


class CBFException : public std::runtime_error
{
  public:
  CBFException(const std::string& what) : std::runtime_error(what)
    { }
};

class SteeringVector
{
 public:

  void config(double p_tol, double s_tol);
  void put(double sv_pitch, double sv_ssp, Matrix sv_real, Matrix sv_imag);
  bool get(double sv_pitch, double sv_ssp, Matrix & sv_real, Matrix & sv_imag);

 private:
  double pitch_tol;
  double ssp_tol;
  double pitch;
  double ssp;
  Matrix       steering_vector_real;
  Matrix       steering_vector_imag;    
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
    void handle_sonar_processing_command(const CMOOSMsg& msg);
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
    void save_stft_file(std::string name);
    void publish_active_processsing_parameters();

    // local Utilities    
    int str2dbl_vec(std::string, std::vector<double>& values);
    bool dbl_vec2str(std::vector<double> numbers, std::string& str);
    double angle360(double hdg);

    SonarProcessingCommand last_processing_cmd_;
    vector<SteeringVector>  SteeringVectors;

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

    double prev_steering_pitch_;
    bool filt_set_;
    bool got_pitch_;
    int filter_length_;

    //prosecute mode stuff
    bool sonar_on_;
    bool processing_on_;

    bool new_file_flag_;
    bool got_filename_;
    std::string sonar_filename_;
    std::string sonar_basename_;
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
    double single_bin_spectral_normalization_;
    double spectral_normalization_;

    double num_overlapping_samples_;

    int num_samples_skip_;
    int num_samples_read_;

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
    //variables used to generate steering vectors using new convention
    Matrix       steering_vector_real;
    Matrix       steering_vector_imag;    
    ColumnVector azimuthal_beams;
    ColumnVector elevation_beams;

    int *array_elements_used;
    int num_elements_used_;
    int *bins;
    int num_bins_;

    Matrix       Spectrum_real;
    Matrix       Spectrum_imag;
    Matrix P;

    // Declare matrices which will contain Short Time Fourier Transform (STFT) along each steered beam as a function of azimuth/elevation.
    boost::multi_array<double, 4> Beam_STFT_real;
    boost::multi_array<double, 4> Beam_STFT_imag;

    //parameters used for active sonar detector
    double  transmit_elevation_angle_;
    double pingtime_;
    double recordtime_;
    double proc_offset_;
    double proc_duration_;
    SonarProcessingCommand::WaveformEnum waveform_;
    double ping_x_;
    double ping_y_;
    double ping_depth_;
    double ping_speed_;
    double ping_heading_;
    double ping_pitch_;
    
    std::vector<double> beam_angles;
	
    double time_window_constant_;
    double hydrophone_amplitude_gain_;
    double hydrophone_calibration_;
    double sound_speed_;

    double homing_beacon_frequency_;
    int homing_frequency_bin_;
    bool homing_mode_active_;
    std::string homing_bin_list_;
};

#endif
