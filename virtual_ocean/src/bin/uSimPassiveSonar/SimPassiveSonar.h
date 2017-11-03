#ifndef SIMPASSIVESONAR_HEADER
#define SIMPASSIVESONAR_HEADER

#include <string>
#include <vector>
#include <math.h>
#include "MOOS/libMOOS/MOOSLib.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"
#include "MBUtils.h"
#include "goby/moos/goby_moos_app.h"
#include "MOOS/libMOOSGeodesy/MOOSGeodesy.h"
#include "lamss/lib_newmat10D/newmatap.h"
#include <lamss/lib_henrik_util/CRapRays.h>
#include <lamss/lib_henrik_util/CEigenRays.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_fft_halfcomplex.h>
#include "lamss/lib_bellhop/svp_request_response.pb.h"
#include "lamss/lib_bellhop/iBellhop_messages.pb.h" 
#include "goby/moos/moos_protobuf_helpers.h"
#include "SimPassiveSonar_config.pb.h"
#include "lamss/lib_lamss_protobuf/lamss_pinger_command.pb.h"


// from lib_henrik_util
#include <lamss/lib_henrik_util/str_num.h>
#include <lamss/lib_henrik_util/CTarget.h>


#include "lamss/lib_henrik_util/CVectorUtils.h"
#include "lamss/lib_henrik_util/Random.h"
#include "lamss/lib_henrik_util/CCovMat.h"

class CSensor
{
 public:
  vector<double> spectrum;
  bool initSpectrum(int nfft_in);
  bool clearSpectrum();
  bool putSpectrum(vector<double> spec_in);
  bool addToSpectrum(vector<double> spec_in);
  vector<double> getSpectrum();
  vector<double> timeseries();

 protected:
  int nfft;
};
  
class CAcousticContact
{
 public:
  std::string contact;
  double contact_x;
  double contact_y;
  double contact_depth;
  bool ready;
  double last_request;
  double last_range;
  double last_rate;
  double last_rate_update;
  bool svp_ready;
  int svp_request_id;
  double last_svp_request;
  CEigenRayTable  ray_table;
  SVP::Response svp_response;
};

class SimPassiveSonar : public GobyMOOSApp
{
  public:
    static void* driver_library_handle_;
    static SimPassiveSonar* get_instance();

  private:
    SimPassiveSonar(SimPassiveSonarConfig& cfg);
    ~SimPassiveSonar() { }

    // synchronous event
    void loop();    
    void slow_loop(double curr_time);
    double slow_loop_period;

    // mail handlers

    void handle_ael_heading(const CMOOSMsg& msg);
    void handle_ael_pitch(const CMOOSMsg& msg);
    void handle_nav_speed(const CMOOSMsg& msg);
    void handle_array_x(const CMOOSMsg& msg);
    void handle_array_y(const CMOOSMsg& msg);
    void handle_array_z(const CMOOSMsg& msg);
    void handle_array_dir_x(const CMOOSMsg& msg);
    void handle_array_dir_y(const CMOOSMsg& msg);
    void handle_array_dir_z(const CMOOSMsg& msg);
    void handle_array_heading(const CMOOSMsg& msg);
    void handle_array_pitch(const CMOOSMsg& msg);
    void handle_target_sim_list(const CMOOSMsg& msg);
    void handle_target_sim_state(const CMOOSMsg& msg);
    void handle_target_arrival_file(const CMOOSMsg& msg);
    void handle_pinger_arrival_file(const CMOOSMsg& msg);
    void handle_ambient_nl_depth(const CMOOSMsg& msg);
    void handle_noise_vert_dir(const CMOOSMsg& msg);
    void handle_svp_response(const CMOOSMsg& msg);
    void handle_covariance_file(const CMOOSMsg& msg);
    void handle_vehicle_name(const CMOOSMsg& msg);
    void handle_sound_speed(const CMOOSMsg& msg);
    void handle_pinger_command(const CMOOSMsg& msg);

    SimPassiveSonarConfig& cfg_;
    double last_slow_loop_t_;
    
    static SimPassiveSonar* inst_;    

  protected:

    bool debug;
    bool debug2;

    // Sensor arrays
    std::vector<CSensor> hydrophones;
    std::vector<CSensor> x_accelerometers;
    std::vector<CSensor> y_accelerometers;
    std::vector<CSensor> z_accelerometers;


    CMOOSGeodesy m_Geodesy;
    bool mseas_svp_updates;
    double mseas_update_interval;
    double svp_range_resolution;
    double svp_depth_resolution;
    std::vector<std::string> svp_requests;
    double sound_speed;

    CmyVectorUtils vecUtils;
    // noise covariances
    std::vector<CovMat> noise_covar;
    // ComputeCovariance compCov;

    // RAP raytracer
    CRapRays Rap;
 
    //---Declare variables that get their valies directly from the .moos file---
    LAMSS_PINGER_COMMAND pinger_command;
    bool getPingerInfo();
    void generate_doppler_waveform(std::vector<double>& source_waveform, double dop_fac);
    bool pinger_active;
    std::string pinger_id;
    bool pinger_enabled;
    double pinger_x;
    double pinger_y;
    double pinger_depth;
    double pinger_db;

    double pingtime;
    double ping_center_frequency;
    double ping_bandwidth;
    double ping_pulsewidth;
    std::vector<double> waveform;
    // Time stam for each recorded file
    double timestamp;
    // Target control. true= SourceSim, false = prosecute message
    bool t_control; 

    // Multi target expansion

    int str2int_vec(std::string, std::vector<int>&);
    bool getTargetInfo(int i);

    bool putAcousticContact(std::string cn, double contact_x,
			    double contact_y, double contact_depth,
			    CEigenRayTable eig_table);
    bool getAcousticContact(std::string cn, CEigenRayTable& eig_table);
    int findAcousticContact(std::string cn);
    bool readyAcousticContact(std::string cn);
    bool requestRayTrace(std::string cn, 
			 double r_min, double r_max, int num_r, 
			 double dep_min, double dep_max, int num_dep,
			 double freq, int target_number);
    double getAmbientNoise(double depth, 
			   std::vector<double> dom_nl,
			   std::vector<double> noise_db);
    bool parseNoiseProfile( std::string noise_in, 
			   std::vector<double>& dom_nl,
			   std::vector<double>& noise_db);
    bool parseNoiseDirectionality( std::string noise_in, 
			   std::vector<double>& theta_deg,
			   std::vector<double>& S_dB);
    bool requestSVPUpdate(std::string cn, 
			  int target_number);
    bool readyAcousticContactSVP(std::string cn);
    bool putAcousticContactSVP(SVP::Response svp_response);
    bool getAcousticContactSVP(std::string cn, 
			       SVP::Response& svp_response );
    bool handleSVPResponse(const CMOOSMsg& msg);
    bool defineRayTracingBox(CAcousticContact& target,
			     double& r_min,
			     double& r_max,
			     double& dep_min,
			     double& dep_max,
			     int& num_r,
			     int& num_dep);

    bool noise_profile;
    std::vector<double> depth_nl;
    std::vector<double> noise_db;

    bool noise_direct;
    std::vector<double> theta_deg;
    std::vector<double> S_dB;
    std::vector<double> theta;
    std::vector<double> S;

    std::vector<CAcousticContact> acoustic_contacts;

    double start_time;
    bool multi_target;
    int number_of_targets;
    double raytracing_interval;
    bool vsa;
    bool ISOTROPIC;

    std::vector<CTarget> Targets;

    std::vector<int> m_target_number;

    std::vector<bool> ok_target_info;


    // source parameters
    std::vector<double> tone_freqs;
    std::vector<double> tone_amps;

    // environmental parameters
    double water_depth; // Used for region of spherical spreading.
    double water_c;
    double c_0;
    double c_grad;
    double z_max;
    bool rap_acoustics;
    bool ray_acoustics;
    int max_number_bounces;
    std::string arr_file_path;
    double bottom_loss; // modal attenuation dB/lambda
    double noise_level;
    int iseed;

    // array and sampling parameters
    std::string array_type;
    int num_array_elements;
    double sampling_frequency;  
    int num_samples;
    int num_freqs;
    unsigned int nfft;

    bool noise_cov_built;
    double minimum_frequency;
    double maximum_frequency;
  
    double hydrophone_gain;
    double accel_gain;


    std::string ownship;
    // output file location and name
    std::string base_dir_name;
    std::string acoustic_filename_prefix;
    std::string acoustic_filename_suffix;
    std::string nonacoustic_filename_prefix;
    std::string nonacoustic_filename_suffix;
    std::string sync_path;
    std::string sync_password;
    bool sync;

    //---Variables that get their value from the MOOSDB---
    double ael_x;
    double ael_y;
    double ael_z;
    double ael_heading;
    double ael_speed;
    double ael_pitch;
    double ael_yaw;

    std::vector<double> array_x_rel; // x coordinates of array elements
    // relative to tow position (what's in the MOOSDB)
    std::vector<double> array_y_rel;
    std::vector<double> array_z_rel;
    std::vector<CEigenRay> array_ray_table;

    double source_pos_x;
    double source_pos_y;
    double source_dep;
    double source_spd;
    double source_hdg;
    bool got_source_loc;
    bool source_status;

    double source_bb_spl;
    double source_bb_amp;
    double source_bb_freq;
    double source_bb_bw;


    double depth_ref;
    double range_ref;
    double dip_ref;
    double p_ref;
    double tau_ref;
    bool   turn_ref;
    double ph_shift_ref;
    double tl_est;
    double nl_prev;
    int icnt_rot;

    // Eigenrays
    CEigenRayTable eigenray_table;
    CEigenRay eig;
 
    // --- Declare other variables ---
    bool ok_towx,ok_towy,ok_towz,ok_hydx,ok_hydy,ok_hydz, 
      ok_array_pitch, ok_array_heading, 
      ok_pinger_x, ok_pinger_y, 
      got_array_info, 
      ok_source_bb_spl, ok_source_bb_freq, 
      ok_source_bb_bw, got_source_bb_info;
    bool ok_dir_x, ok_dir_y, ok_dir_z, got_array_dir_info;

    double noise_std;
    std::vector<double> tone_amps_Pa; 
    std::vector<double> tone_ks; 

    std::vector<double> array_x; // x coordinates of array elements
    // relative to (0,0) -- origina of coordinate system. 
    std::vector<double> array_y;
    std::vector<double> array_z;
    std::vector<double> array_dir_x;
    std::vector<double> array_dir_y;
    std::vector<double> array_dir_z;

    std::vector<bool> array_dir;

    std::vector<double> array_heading;
    std::vector<double> array_pitch;
    double array_avg_pitch;
    double array_prev_pitch;

    double surface_rms_roughness;
    double surface_roughness_loss;
    double bottom_rms_roughness;
    double bottom_roughness_loss;

    // doubles to keep track of time
    double time_last;
    double file_interval_time;
    int frame_number;

    // matricies 
    gsl_matrix * sum_of_timeseries;
    gsl_matrix * sum_of_timeseries_x;
    gsl_matrix * sum_of_timeseries_y;
    gsl_matrix * sum_of_timeseries_z;
    
    // gsl random number generator
    const gsl_rng_type * gsl_rng_type_1;
    gsl_rng * gsl_rng_1;
    std::vector<gsl_rng *> gsl_rng_tgt;

    // --- Functions ---
    //    int get_values(std::string, std::vector<double>& values);
    bool print_vec(vector<double> temp);
    bool print_array(double a[], int l);

    bool generate_eigenray_table(int cn_num, 
				 CAcousticContact& acoustic_contact);
    bool generate_signal_spectrum(int tgt_count);
    bool generate_corr_noise_spectrum();
    bool save_spectral_timeseries();
    bool add_pinger_timeseries();
    bool initialize_spectrum();
    bool ray_arrivals(CEigenRay eig, 
		      double rcv_range,
		      double rcv_depth, 
		      std::vector<double> & cos_Src,
		      std::vector<double> & cos_Rcv,
		      std::vector<double> & delays, 
		      std::vector<double> & amplitudes);

    void ray_doppler_vel(double v_S, double a_S,double e_S, 
				double v_R, double a_R, double e_R, 
				double& vr_S, double& vr_R );
    void array_ray_arrivals(std::vector<std::vector<double> > & delays,
			    std::vector<std::vector<double> > & amplitudes,
			    std::vector<std::vector<double> > & doppler);

    bool write_hyd_file();
    bool write_nas_file(double timestamp);
    bool write_nas_file_verbose(double timestamp);
    bool write_eigenray_file();
    bool read_covmat_file(string covmat_file, double nl_now, double& t_req);
      
};

#endif 

