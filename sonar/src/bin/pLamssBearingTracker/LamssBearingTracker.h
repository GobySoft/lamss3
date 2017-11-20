// Copyright 2015 
// Henrik Schmidt
// Massachusetts Institute of Technology 

#ifndef LamssBearingTracker20171117H
#define LamssBearingTracker20171117H

#include "goby/moos/goby_moos_app.h"
#include "MBUtils.h"

#include <sstream>
#include "math.h"
#include "lamss/lib_newmat10D/newmatap.h"
#include <sys/time.h>
#include <deque>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#include <stdlib.h>
#include "goby/moos/moos_protobuf_helpers.h"

#include "LamssBearingTracker_config.pb.h"

// from lib_henrik_util
#include <lamss/lib_henrik_util/CTrack.h>
// Lamss protobuf messages
#include "lamss/lib_lamss_protobuf/BeamformingVars.pb.h"
#include "lamss/lib_lamss_protobuf/ArrayNavigation.pb.h"

#define NO_TRACK     0
#define LR_TRACK     1
#define TRACKING     2
#define MAX_PREVIEW  512

//using namespace NEWMAT;
using namespace std;

class TargetsException : public std::runtime_error
{
  public:
  TargetsException(const std::string& what) : std::runtime_error(what)
    { }
};

class LamssBearingTracker : public GobyMOOSApp
{
  public:
    static void* driver_library_handle_;
    static LamssBearingTracker* get_instance();

  private:
    LamssBearingTracker(LamssBearingTrackerConfig& cfg);
    ~LamssBearingTracker() { }

    // synchronous event
    void loop();    
    void slow_loop(double curr_time);

    // mail handlers

    void handle_beam_data(const CMOOSMsg& msg);
    void handle_beam_angles(const CMOOSMsg& msg);
    void handle_ael_nav(const CMOOSMsg& msg);
    void handle_beamforming_vars(const CMOOSMsg& msg);
    void handle_tracker_command(const CMOOSMsg& msg);
    void handle_sonar_control(const CMOOSMsg& msg);
    void handle_sonar_process(const CMOOSMsg& msg);

    LamssBearingTrackerConfig& cfg_;
    double last_slow_loop_t_;

    static LamssBearingTracker* inst_;    

 protected:

		bool old_notify;
		CTrack Track;
		vector<double> class_features;
		bool postTrack();

		// The following buffer arrays added for MOOSDB comms of BTR data

		bool new_btr;
		bool new_cosines;
		bool new_sampling;
		bool got_heading;
		bool got_pitch;
		bool got_beamforming_vars;
		bool initialized;

                std::vector<double> beam_data;
                std::vector<double> beam_angles;

//array navigation
		double ael_depth_;
		double ael_x_;
		double ael_y_;
		double ael_heading_;
		double ael_pitch_;

		double *P;
		double *Beam_cosines;
		double *Beam_sines;

		double *P_sorted; //Added AJP 4 Oct 2007
		int *P_indx; //Added AJP 4 Oct 2007
		int *P_indx_sorted; //Added AJP Oct 2007
	
//number of good elements
		int num_elements_used;
//average element spacing on array
		double mean_element_spacing;
//length of DFT
		int fft_length;
//number of beams for DOA estimation
		int num_beams;
//Amount of overlap between FFT blocks
		double overlap;
//Integration time (in seconds)
		double integrate;
//sound speed of water
		double sound_speed;
//center frequency of source (Hz)
		double mean_frequency;
//sampling frequency (Hz)
		double sample_freq;
//number of integration blocks
		int Int_Blocks;
//Actual integration
		double Actual_Integration;

		int Max_Index(double *x, int len);
		void hanning(Real *,int);
		int  round(double);
		bool Process_Data();
		bool initialize();
		void zeroes(double *,int);
		double get_msigma();
		void reset_tracking();
		void reset_lr();

//beam tracking stuff
		bool ambiguous;
		double err_const,ufilt,udot,uguessn,uguesso,del_cos,u_est,u_estn;
		double alpha,stab_az,dep_angle,pitch; // JPI/DHG 2006/07/06 Make "alpha_head" a parameter
		double K,thethat1n_sv,u_error,beta,DEN,NVM,rel_bear,thethat1n,thethat1;
		double del_beam,del_beamo2,maxmean;
		int endfire,beam_no,IPS,lridx,save_idx,det_count,lrcount,IPS_Old,IPS_Change,Endfire_Old;
		int ICOAST,IPSsv,endfire_out,turn,ICOAST_SV,beam_no_sv;
		double heading,heading_save,head_filt, head_filt_dot,beta_head,DEN_SV;
		double old_heading,maxsum,sigmasum,sigma,detection_threshold;
		double aaa,bbb,ccc,fff,ggg,iii,jjj,rel_bear_sv,argg,head_filt_sv,head_filt_dot_sv;
		int ddd,eee,hhh,kkk,old_max,overiter,missed_count;
		bool initial_time,idx_set,tracking,noise_var_set,filt_set;
		bool sonar_on;
		bool processing_on;
		bool detect_mode;
		int dcount,freq_selection,filt_len;
		int detector_type, detector_num_bins, detector_first_bin; // AJP 4 Oct 2007
		double final_detection_threshold, mean_sum, amb_noise_mean, std_dev_sum; // AJP 4 Oct 2007
		double noise_depth, detector_alpha; // AJP 5 Oct 2007

// Added the following parameters by JPI/DHG 2006/07/06
		double alpha_head;                // Units are in degrees (0.7)
		double turn_rate_threshold;       // Units are in degrees per second (0.1 for line array, 2 for swordfish)
		int    beam_changed_threshold;    // Units are in beams (7)
		int    max_ubias_iter;            // Units are in number of iterations (25)
		double abs_uerror_threshold;      // Units are in ? (1.0)
		double theta_min;                 // Units are in degrees (10.0)
		int    detection_count_threshold; // Units are in number of detections (5)

//local node number
		int node;

		bool detection();
		int track_lr();
		void filter_heading();
		bool Track_Bearing();
		double Min(double,double);
		double Max(double,double);
		int Sign(double);
		int asort (int num_val, double *in_val, int *in_indx, double *out_val, int *out_indx); // DHG Ascending sort, added from _GENERIC by AJP on Oct 4, 2007
		int dsort (int num_val, double *in_val, int *in_indx, double *out_val, int *out_indx); // DHG Descending sort, added from _GENERIC by AJP on Oct 4, 2007

//timer stuff
		struct timeval start;
		struct timeval finish;
		double calc_elapsed(struct timeval,struct timeval);

//prosecute mode stuff
		unsigned int prosecute_mode,deploy_mission;
		bool prosecute,deploy;
		bool tracking_published;
		
		// Added by FB 08/2008 cancel the array file after processing to save disk space (SLITA array samples at 51kHz...)
		bool bDelFileAfterProc;
		
		// Added ustilities for getting beam data from MOOSDB
		
		int str2dbl_vec(std::string, std::vector<double>& values);
		bool dbl_vec2str(std::vector<double> numbers, std::string& str);
		double angle360(double hdg);
		
		// added hs 033109
		std::string ael_nav_variable;
		std::string heading_variable;
		std::string pitch_variable;
    
};

#endif
