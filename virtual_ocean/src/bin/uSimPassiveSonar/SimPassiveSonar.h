#ifndef SIMPASSIVESONAR_HEADER
#define SIMPASSIVESONAR_HEADER

#include <string>
#include <vector>
#include <math.h>
#include "MOOS/libMOOS/MOOSLib.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"
#include "MBUtils.h"
#include "ivp/MOOSGeodesy.h"
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

class CSimPassiveSonar : public CMOOSApp
{
  public:
    CSimPassiveSonar();
    virtual ~CSimPassiveSonar();

    bool Iterate();
    bool OnNewMail(MOOSMSG_LIST &NewMail);
    bool OnConnectToServer();
    bool OnStartUp();

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

    CmyVectorUtils vecUtils;
    // noise covariances
    std::vector<CovMat> noise_covar;
    // ComputeCovariance compCov;

    // RAP raytracer
    CRapRays Rap;
 
    //---Declare variables that get their valies directly from the .moos file---

    // Target control. true= SourceSim, false = prosecute message
    bool t_control; 

    // Multi target expansion

    int str2int_vec(std::string, std::vector<int>&);
    bool getTargetInfo(int i);

    bool putAcousticContact(std::string cn, CEigenRayTable eig_table);
    bool getAcousticContact(std::string cn, CEigenRayTable& eig_table);
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

    // output file location and name
    std::string base_dir_name;
    std::string acoustic_filename_prefix;
    std::string acoustic_filename_suffix;
    std::string nonacoustic_filename_prefix;
    std::string nonacoustic_filename_suffix;
    std::string sync_path;
    std::string sync_missions_root;
    bool sync;

    //---Variables that get their value from the MOOSDB---
    double ael_x;
    double ael_y;
    double ael_z;

    std::vector<double> array_x_rel; // x coordinates of array elements
    // relative to tow position (what's in the MOOSDB)
    std::vector<double> array_y_rel;
    std::vector<double> array_z_rel;

    double source_pos_x;
    double source_pos_y;
    double source_dep;

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
    bool ok_towx,ok_towy,ok_towz,ok_hydx,ok_hydy,ok_hydz, ok_array_pitch, ok_array_heading, ok_sourcex, ok_sourcey, got_array_info, got_source_loc, ok_source_bb_spl, ok_source_bb_freq, ok_source_bb_bw, got_source_bb_info;
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

    bool generate_signal_spectrum(int tgt_count);
    bool generate_corr_noise_spectrum();
    bool generate_spectral_timeseries();
    bool initialize_spectrum();

    bool write_hyd_file();
    bool write_nas_file(double t_now);
    bool write_nas_file_verbose(double t_now);
    bool read_covmat_file(string covmat_file, double nl_now, double& t_req);
      
};

#endif 

