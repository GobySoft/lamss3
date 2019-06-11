#ifndef SIMNOISECOVARIANCE_HEADER
#define SIMNOISECOVARIANCE_HEADER

#include <string>
#include <vector>
#include <math.h>
#include "MOOS/libMOOS/MOOSLib.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"
#include "MBUtils.h"
#include "lamss/lib_newmat10D/newmatap.h"

// from lib_henrik_util
#include <lamss/lib_henrik_util/str_num.h>

#include "lamss/lib_henrik_util/CVectorUtils.h"
#include "lamss/lib_henrik_util/Random.h"
//#include "lamss/lib_henrik_util/CBessel.h"
#include "lamss/lib_henrik_util/CNoiseCovariance.h"
#include "lamss/lib_henrik_util/CCovMat.h"

/*
class ComputeCovariance
{
 public:


  bool initialize(double freq, double sound_speed, 
		  std::vector<double> theta, 
		  std::vector<double> Sin); 
  bool hydCovariance(double delta_R, double delta_z, 
		     std::complex<double> & cov);
  bool vsaCovariance(double delta_R, double delta_z,
		     std::vector<std::complex<double> >& cov);
  bool vsaIsotropicCovariance(double delta_R, double delta_x, double delta_y, double delta_z, vector<complex<double> >& cov);

  bool hydIsotropicCovariance(double delta_R, complex<double> & cov);

  CBessel besgen;

  CBessel spherical_besgen;

 protected:
  complex<double> filon(vector<double> x,
			vector<complex<double> > cfun,
			vector<complex<double> > cex,
			vector<double> arg);

  std::complex<double> conjg(std::complex<double>);
  int indx( int i, int j);
  int n_th;
  double omega;
  double rho;
  double c;
  double k;
  std::vector<double> k_z;
  std::vector<double> k_r;
  std::vector<double> S;
};
*/

class CSimNoiseCovariance : public CMOOSApp
{
  public:
    CSimNoiseCovariance();
    virtual ~CSimNoiseCovariance();

    bool Iterate();
    bool OnNewMail(MOOSMSG_LIST &NewMail);
    bool OnConnectToServer();
    bool OnStartUp();

  protected:

    bool covariance_requested;
    bool debug;
    bool debug2;

    CmyVectorUtils vecUtils;
    // noise covariances
    std::vector<CovMat> noise_covar;
    ComputeCovariance compCov;

    //---Declare variables that get their valies directly from the .moos file---

    int str2int_vec(std::string, std::vector<int>&);

    double computeNoiseCovariance(double depth, 
			   std::vector<double> dom_nl,
			   std::vector<double> noise_db);
    bool parseNoiseProfile( std::string noise_in, 
			   std::vector<double>& dom_nl,
			   std::vector<double>& noise_db);
    bool parseNoiseDirectionality( std::string noise_in, 
			   std::vector<double>& theta_deg,
			   std::vector<double>& S_dB);

    bool noise_profile;
    std::vector<double> depth_nl;
    std::vector<double> noise_db;

    bool noise_direct;
    std::vector<double> theta_deg;
    std::vector<double> S_dB;
    std::vector<double> theta;
    std::vector<double> S;

    double start_time;

    bool vsa;
    bool ISOTROPIC;

    // environmental parameters
    double water_depth; // Used for region of spherical spreading.
    double water_c;
    double c_0;
    double c_grad;
    double z_max;

    double noise_level;
    int iseed;

    // array and sampling parameters
    std::string array_type;
    int num_array_elements;
    double sampling_frequency;  
    int num_samples;
    int num_freqs;
    unsigned int nfft;
    double file_interval_time;
    bool noise_cov_built;
    double minimum_frequency;
    double maximum_frequency;
  

    // output file location and name
    std::string base_dir_name;

    //---Variables that get their value from the MOOSDB---
    double ael_x;
    double ael_y;
    double ael_z;

    std::vector<double> array_x_rel; // x coordinates of array elements
    // relative to tow position (what's in the MOOSDB)
    std::vector<double> array_y_rel;
    std::vector<double> array_z_rel;

    double nl_prev;
    int icnt_rot;

    // --- Declare other variables ---
    bool ok_hydx,ok_hydy,ok_hydz, ok_array_pitch, ok_array_heading, got_array_info ;
    double noise_std;

    std::vector<double> array_x; // x coordinates of array elements
    // relative to (0,0) -- origina of coordinate system. 
    std::vector<double> array_y;
    std::vector<double> array_z;
    std::vector<double> array_heading;
    std::vector<double> array_pitch;
    double array_avg_pitch;
    double array_prev_pitch;

    // doubles to keep track of time
    double time_last;

    // --- Functions ---

    bool write_covmat_file(double t_now, double nl_now, int i_first, int i_last);
    // Bessel function generator class
    CBessel besgen;

    CBessel spherical_besgen;
};

#endif 

