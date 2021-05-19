// Copyright 2015 
// Henrik Schmidt
// Massachusetts Institute of Technology 
// goby::moos::GobyMOOSApp Version of pBearingTracker
/*--------------------------------------------------------------------------

File:        pBearingTracker.cpp
Author:      JPI/SAIC,  Don Eickstadt & Andrew Poulsen /MIT
Generalized: Henrik Schmidt/MIT
Date:
Revisions:   JPI/DHG/SAIC 2006/07/06 Accomdate new VSA
    DHG/SAIC     2007/08/06 Read new HYD (acoustic pressure sensor) format
    DHG/SAIC     2007/09/24 Do not process data unless we have fresh new incoming data files
                and add A.Poulsen fix to index for steering vector.
    HS/MIT       2008/03/17 Generic array version
        Description: This program performs the detection,
    and stabilized beam tracking of data collected all arrays used on the MIT Bluefin AUVS
    , and is interfaced to MOOS to get and publish information. 


--------------------------------------------------------------------------*/

//***************************************************************************
//*  LamssBearingTracker.cpp                                                       *
//*                                                                         *
//*  Bearing Track is a real-time acoustic beamformer that reads in         *
//*  acoustic data files coming from a data acquisition system and provides *
//*  a bearing to an acoustic target.                                       *
//***************************************************************************      


#include <boost/thread.hpp>

#include "goby/util/debug_logger.h"
#include "goby/acomms/connect.h"
#include "goby/time/legacy.h"

#include "LamssBearingTracker.h"
#include "lamss/lib_anrp_util/sliva_parse.h"

#define SAIC_DEBUG 0

//#if SAIC_DEBUG
#if 1
FILE *fpPow = 0;  // debug beam power
#endif

#include "goby/moos/moos_protobuf_helpers.h"

using namespace goby::util::logger;
using namespace goby::util::logger_lock;
namespace gpb = goby::moos::protobuf;
using goby::common::goby_time;
using goby::glog;

const double pi = 3.14159;

boost::shared_ptr<LamssBearingTrackerConfig> master_config;
LamssBearingTracker* LamssBearingTracker::inst_ = 0;
void* LamssBearingTracker::driver_library_handle_= 0;

int main(int argc, char * argv[])
{
    return goby::moos::run<LamssBearingTracker>(argc, argv);
}

LamssBearingTracker* LamssBearingTracker::get_instance()
{
    if(!inst_)    {
        master_config.reset(new LamssBearingTrackerConfig);
        inst_ = new LamssBearingTracker(*master_config);
    }
    return inst_;
}

//***************************************************************************
//*  LamssBearingTracker::LamssBearingTracker()                             *
//*                                                                         *
//*  The constructor for the LamssBearingTracker class.                     *
//*  Variables can be initialized here.                                     *
//*                                                                         *
//***************************************************************************   

LamssBearingTracker::LamssBearingTracker(LamssBearingTrackerConfig& cfg)
    : goby::moos::GobyMOOSApp(&cfg),
  cfg_(cfg),
  new_btr(false),
  new_cosines(false),
  new_sampling(false),     
  ael_pitch_(0.0),
  det_count(0),
  lrcount(0),
  sonar_on(false),
  processing_on(false),
  tracking(true),
  noise_var_set(false),
  dcount(0),
  detect_mode(false),
  prosecute_mode(1),
  bbb(0),
  thethat1(0),
  thethat1n(0),
  IPS(-100),
  overiter(0),
  got_heading(false),
  got_pitch(false),
  got_beamforming_vars(false),
  ambiguous(false),
  initialized(false),
  last_slow_loop_t_(0)
{

  set_ignore_stale(false);

  // get config variables
  ael_nav_variable = cfg_.ael_nav_variable();
  alpha = cfg_.alpha();
  dep_angle = cfg_.dep_angle();
  detection_threshold = cfg_.detection_threshold();
  lridx = (int) cfg_.lr_beams();
  alpha_head = cfg_.alpha_head();
  turn_rate_threshold = cfg_.turn_rate_threshold();
  beam_changed_threshold = cfg_.beam_changed_threshold();
  max_ubias_iter = cfg_.max_ubias_iter();
  abs_uerror_threshold = cfg_.abs_uerror_threshold();
  theta_min = cfg_.theta_min();
  detection_count_threshold = cfg_.detection_count_threshold();
  detector_type = cfg_.detector_type();
  detector_num_bins =  cfg_.detector_number_bins();
  detector_first_bin =  cfg_.detector_first_noise_bin();
  detector_alpha =  cfg_.detector_alpha();

  class_features.clear(),

  glog.is(DEBUG1) && glog << "Alpha = " << alpha << std:: endl;
  
  // JPI/DHG 2006/07/06 
  glog.is(DEBUG1) && glog << "Alpha_Head = " <<  alpha_head << std::endl;
  glog.is(DEBUG1) && glog << "Turn Rate Threshold = " <<  turn_rate_threshold 
			  << std::endl;
  glog.is(DEBUG1) && glog << "beam_changed_threshold = " << beam_changed_threshold 
			  << std::endl;
  glog.is(DEBUG1) && glog << "max_ubias_iter = " << max_ubias_iter << std::endl;
  glog.is(DEBUG1) && glog << "abs_uerror_threshold = " << abs_uerror_threshold 
			  << std::endl;
  glog.is(DEBUG1) && glog << "theta_min = " << theta_min << std::endl;
  glog.is(DEBUG1) && glog << "detection_count_threshold = " 
			  << detection_count_threshold 
			  << std::endl;
  
  // set up subscriptions before starting separate thread
  subscribe("BEAM_DATA", &LamssBearingTracker::handle_beam_data, this);    
  subscribe("BEAM_ANGLES", &LamssBearingTracker::handle_beam_angles, this);
  subscribe(ael_nav_variable, &LamssBearingTracker::handle_ael_nav, this);
  subscribe("BEAMFORMING_VARS_PB", &LamssBearingTracker::handle_beamforming_vars, this);    
  subscribe("TRACKER_COMMAND", &LamssBearingTracker::handle_tracker_command, this);    
  subscribe("SONAR_CONTROL", &LamssBearingTracker::handle_sonar_control, this);    
  subscribe("SONAR_PROCESS", &LamssBearingTracker::handle_sonar_process, this);    

  //

  initialized = initialize();
  reset_tracking();
  
  glog.is(DEBUG1) && glog << cfg_.DebugString() << std::endl;
}

//***************************************************************************
//*  LamssBearingTracker::~LamssBearingTracker()                            *
//*                                                                         *
//*  The destructor for the LamssBearingTracker class.                          *
//*  Dynamic memory can be freed here.                                      *
//*************************************************************************** 

void LamssBearingTracker::handle_beam_angles(const CMOOSMsg& msg)
{
  string sbeam_data = msg.m_sVal;
  num_beams = str2dbl_vec(sbeam_data, beam_angles);
  glog.is(DEBUG1) && glog << "Getting BEAM_COSINES. num_beams = " << num_beams 
			  << std::endl;
  double max_angle = -1e6;
  double min_angle = 1e6;
  for (int i=0; i< num_beams; i++)
    {
      max_angle = std::max(max_angle,beam_angles[i]);
      min_angle = std::min(min_angle,beam_angles[i]);
    }
  
  ambiguous = (max_angle < 185) && (min_angle > -5);
  
  if (new_cosines)
    {
      delete(P);
      delete(Beam_cosines);
      delete(Beam_sines);
    }
  
  P = new double[num_beams+1];
  zeroes(P,num_beams+1);
  
  P_sorted = new double[num_beams+1];
  zeroes(P_sorted,num_beams+1);
  
  P_indx = new int[num_beams+1]; 
  P_indx_sorted = new int[num_beams+1];
  for (int k=1; k <= num_beams; k++)
    { 
      P_indx[k]=k;
      P_indx_sorted[k]=0;
    }
  
  Beam_cosines = new double[num_beams+1];
  Beam_sines = new double[num_beams+1];
  
  for (int mm=0; mm<num_beams;mm++)
    {
      Beam_cosines[mm+1] = cos(beam_angles[mm]*M_PI/180.0);
      Beam_sines[mm+1] = sin(beam_angles[mm]*M_PI/180.0);
    }
  new_cosines = true;
}

void LamssBearingTracker::handle_beam_data(const CMOOSMsg& msg)
{    
  glog.is(DEBUG1) && glog << "Getting BEAM_DATA" << std::endl;
  string sbeam_data = msg.m_sVal;
  if (new_cosines && new_sampling)
    {
      int nb = str2dbl_vec(sbeam_data, beam_data);
      if (nb == num_beams )
	new_btr = true;        
      else
	{
	  glog.is(DEBUG1) && glog << "Mismatch: nb = " << nb
				  << ", num_beams = " <<  num_beams << std::endl;
	  new_btr = false;
	}
    }
}

void LamssBearingTracker::handle_beamforming_vars(const CMOOSMsg& msg)
{
  BeamformingVars beamforming_vars;
  parse_for_moos(msg.GetString(), &beamforming_vars);
  fft_length = beamforming_vars.fft_size();
  num_beams = beamforming_vars.number_beams();
  overlap = beamforming_vars.overlap();
  integrate = beamforming_vars.integration_time();
  sound_speed = beamforming_vars.sound_speed();
  sample_freq = beamforming_vars.sampling_frequency();
  num_elements_used = beamforming_vars.num_elements_used();
  mean_frequency = beamforming_vars.mean_frequency();
  mean_element_spacing = beamforming_vars.mean_element_spacing();
  got_beamforming_vars = true;
  new_sampling = true;
}

void LamssBearingTracker::handle_ael_nav(const CMOOSMsg& msg)
{
  ArrayNavigation ael_nav;
  parse_for_moos(msg.GetString(), &ael_nav);
  ael_x_ = ael_nav.ael_x();
  ael_y_ = ael_nav.ael_y();
  ael_depth_ = -ael_nav.ael_z();
  ael_heading_ = ael_nav.ael_heading();
  ael_pitch_ = ael_nav.ael_pitch();
  got_heading = true;
  got_pitch = true;
}

void LamssBearingTracker::handle_sonar_control(const CMOOSMsg& msg)
{    
  string mess = msg.m_sVal;
  if (MOOSStrCmp(mess,"OFF"))
    {
      if (sonar_on) 
	glog.is(VERBOSE) && glog << "sonar is turned off" << std::endl;
      sonar_on = false;
    }
  else if (MOOSStrCmp(mess,"ON"))
    {
      if (!sonar_on) 
	glog.is(VERBOSE) && glog << "sonar is turned on" << std::endl;
      sonar_on = true;
    }
}

void LamssBearingTracker::handle_sonar_process(const CMOOSMsg& msg)
{    
  string mess = msg.m_sVal;
  if (MOOSStrCmp(mess,"OFF"))
    {
      if (processing_on) 
	glog.is(VERBOSE) && glog << "processing is turned off" << std::endl;
      processing_on = false;
    }
  else if (MOOSStrCmp(mess,"ON"))
    {
      if (!processing_on) 
	glog.is(VERBOSE) && glog << "processing is turned on" << std::endl;
      processing_on = true;
    }
}

void LamssBearingTracker::handle_tracker_command(const CMOOSMsg& msg)
{     
  string t_state = msg.m_sVal;
  
  if(MOOSStrCmp(t_state,"RESET"))
    reset_tracking();
  else if(MOOSStrCmp(t_state,"ON"))
    {
      if(!detect_mode)
	reset_tracking();
      detect_mode = true;
    }            
  else
    detect_mode = false;
}

void LamssBearingTracker::loop()
{

  double curr_time = MOOSTime();
  
  if (sonar_on && processing_on)
    {
      if (got_heading && got_pitch)
	{
	  if (initialized)
	    {
	      if (!Process_Data())
		glog.is(VERBOSE) && glog << "Processing failed!" << std::endl;
	    }
	  else if (got_beamforming_vars && new_cosines)
	    {
	      initialized = initialize();
	    }
	  else
	    glog.is(DEBUG1) && glog << "Waiting for Beamforming variables" << std::endl; 
	}
      else
	glog.is(DEBUG1) && glog << "Waiting for array navigation" << std::endl; 

    }

  // process some events at a slower rate
  const int slow_loop_period = 10;
  if(curr_time > last_slow_loop_t_ + slow_loop_period)
    slow_loop(curr_time);
}

void LamssBearingTracker::slow_loop(double curr_time)
{
  // Here we should add slow loop actions.

  last_slow_loop_t_ = curr_time;
}

//***************************************************************************
//*  LamssBearingTracker::initialize()                                                *
//*                                                                         *
//*  Initialize all of the data structures and constants needed to process  *
//*  data.                                                                  *
//*                                                                         *
//*************************************************************************** 

bool LamssBearingTracker::initialize()
{

    bool silent = true;
    double freq,temp;
    int k,z,m;

    //beam tracking constants
    //calculate a bunch of constants

    Int_Blocks = round((double)(integrate*sample_freq/((1-overlap)*fft_length)));

    Actual_Integration = Int_Blocks*((1-overlap)*fft_length)/sample_freq;
    glog.is(DEBUG1) && glog << "Int_Blocks = " << Int_Blocks
			    << " integrate = " << integrate
			    << " sample_freq = " << sample_freq
			    << " overlap = " << overlap
			    << " fft_length = " << fft_length
			    << "Actual_Integration = "
			    << Actual_Integration << std::endl;
    err_const = (2.0/3.0)*(1.0/pow(3,0.5));

    //if (num_beams == 9)             //JPI/DHG 2007/07/06
    //    K = -0.1185/err_const;        //JPI/DHG 2007/07/06
    //else if (num_beams >= 17)       //JPI/DHG 2007/07/06
    K = 0.1185/err_const;           //JPI 2006/07/06 Make this the ONLY option

    //  K = 0.1185/err_const;         //JPI/DHG 2007/07/06


    ufilt = 0.0;
    udot = 0.0;
    del_beam = 2.0/(num_beams-1);
    del_beamo2 = del_beam/2.0;
    head_filt = ael_heading_;
    head_filt_dot = 0.0;
    //JPI/DHG 2006/07/06 This is now a parameter alpha_head = 0.7;
    beta_head = (alpha_head*alpha_head)/(2.0 - alpha_head);
    beta = (alpha*alpha)/(2.0-alpha);
    initial_time = true;
    IPS = -100;
    dcount = 0;
    maxsum = 0.0;
    sigmasum = 0.0;
    sigma = 0.0;
    idx_set = false;
    endfire = 0;
    u_estn = 0.0;
    mean_sum = 0.0; // AJP 4 OCT 2007
    std_dev_sum = 0.0; // AJP 4 OCT 2007
    amb_noise_mean = 0.0; // AJP 5 OCT 2007
    maxmean = 0.0; // AJP 9 OCT 2007

    return true;
}

//***************************************************************************
//*  LamssBearingTracker::ProcessData()                                          *
//*                                                                         *
//*  Called after a new data file has been read. This method calculates     *
//*  target bearings.                                                       *
//*************************************************************************** 
bool LamssBearingTracker::Process_Data()
{    
  
    bool detect_flag;

    if (new_btr && new_cosines && new_sampling)    
    {
        // New beam data available
        for (int mm=0; mm<num_beams;mm++)
            P[mm+1] = beam_data[mm]*1e12;
        new_btr = false;
        filter_heading();
        glog.is(DEBUG1) && glog <<"Entering Detector " << std::endl;
	  glog.is(DEBUG1) && glog <<"heading = " << ael_heading_ << std::endl;
        detect_flag = detection(); //check for detection
        glog.is(DEBUG1) && glog <<"Returning from Detector " << std::endl;
	  glog.is(DEBUG1) && glog <<"detect_flag: " << detect_flag 
				<< ", IPS = " << IPS <<std::endl;
        //we have a detection
        if (detect_flag)
        {
            if (IPS  < -1) //prev state = no detection
            {          
                IPS = 0;                 //now ambiguous
                // for volumetric array, declare l/r    
                if (!ambiguous)
                {
                    beam_no = Max_Index(P,num_beams);
                    IPS = Sign(Beam_sines[beam_no]);
                    if (IPS ==0)
                        IPS = 1;
                }
                else
                    IPS = track_lr();
            }
            else if (IPS == 0)   //prev_state = ambiguous
            {
                IPS=track_lr();  
            }
            else if ((IPS == 1)||(IPS == -1)) //prev_state = tracking
            {
                //Track_Bearing();
                if ( !Track_Bearing() )
                {
                    string msg;
                    char buff[200];
                    reset_tracking();
                    IPS = -100;
                }    
            }
        } //if detect flag
        else
        {
            string msg;
            char buff[200];
            if (IPS >= -1)
            {
                reset_tracking();
                IPS = -100;
            }
        }
    }
    return true;
}


/*    Create a Hanning window of length M
    win: output window vector, with length M
*/
void LamssBearingTracker::hanning(Real *window, int M)
{

    int n;

    for (n=0; n < M; n++)
        window[n] = (double) (0.5*(1 - cos(2*M_PI*(n+1)/(M+1))));

}

//round to the nearest integer
int LamssBearingTracker::round(double num) 
{

    if (num >= 0.0)
        return ((int) (num + 0.5));
    else    
        return ((int) (num - 0.5));

}

//return the index of the maximum element in vector x
int LamssBearingTracker:: Max_Index(double *x, int len)
{
    int index = 0;
//double maximum = x[0];
    double maximum = 0.0;

    for (int nn = 1; nn <= len; nn++)
    {
        if (x[nn] > maximum)
        {
            maximum = x[nn];
            index = nn;
        }
    }

    return index;
}

//initialize a vector with zeroes
void LamssBearingTracker::zeroes(double * array,int length)
{
    for (int ii = 0; ii < length; ii++ )
        array[ii] = 0.0;
}


//calculate the elapsed time in seconds
//can only be used to time events lasting < 1 minute
double LamssBearingTracker::calc_elapsed(struct timeval one, struct timeval two)
{

    double one_time = one.tv_sec + one.tv_usec/1000000.0;
    double two_time = two.tv_sec + two.tv_usec/1000000.0;

    if (two.tv_sec < one.tv_sec)
        two_time = two_time + 60;

    return (two_time-one_time);
}

void LamssBearingTracker::filter_heading()
{
    double resid_H;
    /*
    if (ael_heading_ >= 360.0)
        ael_heading_ = ael_heading_ - abs(round(ael_heading_/360.0))*360.0;

    if (ael_heading_ < 0.0)
        ael_heading_ = ael_heading_ + abs(round(ael_heading_/360.0))*360.0;

    if (ael_heading_ >= 360.0)
        ael_heading_ = ael_heading_ - 360.0;

    if (ael_heading_ < 0.0)
        ael_heading_ = ael_heading_ + 360.0;
    */
    ael_heading_ = angle360(ael_heading_);
    //    resid_H = (ael_heading_-head_filt);
    // Proper wrapped heading filter
    double ch = cos(ael_heading_*M_PI/180);
    double sh = sin(ael_heading_*M_PI/180);
    double chf = cos(head_filt *M_PI/180);
    double shf = sin(head_filt *M_PI/180);
    resid_H = atan2(sh-shf,ch-chf)*180/M_PI;

    glog.is(DEBUG1) && glog << "heading = " << ael_heading_ 
			    <<", head_filt =" <<  head_filt
			    << std::endl;

    if (resid_H > 180.0)
        resid_H = resid_H - 360.0;
    else if (resid_H <= -180.0)
        resid_H = resid_H + 360.0;
    glog.is(DEBUG1) && glog << "beta_head = " << beta_head
			    << ", resid_H = " << resid_H
			    << ", Actual_Integration = " << Actual_Integration
			    << std::endl;

    head_filt_dot = head_filt_dot + beta_head*(resid_H)/Actual_Integration;
    head_filt = head_filt + (head_filt_dot*Actual_Integration) + (alpha_head*resid_H);
    head_filt=angle360(head_filt);
    /*
    if (head_filt >= 360.0)
        head_filt = head_filt - 360.0;
    else if (head_filt < 0.0)
        head_filt = head_filt + 360.0;
    */
    glog.is(DEBUG2) && glog <<"head_filt_dot = " << head_filt_dot << std::endl;
    if (fabs(head_filt_dot) > turn_rate_threshold)   //JPI/DHG 2006/07/06 Changed "2" to a parameter
        turn = Sign(head_filt_dot);
    else
        turn = 0;


}

inline double LamssBearingTracker::Min(double v1, double v2)
{
    return(v1 < v2 ? v1 : v2);
}



inline double LamssBearingTracker::Max(double v1, double v2)
{
    return(v1 > v2 ? v1 : v2);
}

int LamssBearingTracker::Sign(double num)
{
    if (num < 0.0)
        return (-1);
    else 
        return (1);
}

bool LamssBearingTracker::Track_Bearing()
{

    double f,df,bmid,bleft,bright,denom;
    int nmid,iter;
    double relative_az; //ADDED AJP 17NOV2006
    
    glog.is(VERBOSE) && glog <<"Tracking Bearing" << std::endl;
      glog.is(DEBUG2) && glog <<"bbb value is  = " << bbb << std::endl;
    if (isnan(bbb))
      {
	bbb=0;
	return(false);
	//      reset_tracking();
      }    
    if(initial_time)
    {
        old_heading = ael_heading_;
        beam_no = Max_Index(P,num_beams);
        old_max = beam_no;

        thethat1n_sv = IPS*acos(Beam_cosines[beam_no])+ (ael_heading_*M_PI/180.0);
        glog.is(DEBUG2) && glog <<"initial thethat1n_sv = " << thethat1n_sv  
				<< std::endl;
        //head_old = ael_heading_;
        //************************
        // IPS_Old = IPS;
        // IPS_Change = 0;

        if ((beam_no > 1) && (beam_no < num_beams))
        {
	  glog.is(DEBUG2) && glog <<"Debug 0 => not endfire" << std::endl;
            endfire = 0;
            //Endfire_Old = 0;
        }
        else if (beam_no == 1)
        {
            glog.is(DEBUG2) && glog <<"Debug 0 => endfire" << std::endl;
            endfire = 1;
            DEN_SV = 0;
        }
        else if (beam_no = num_beams){
            glog.is(DEBUG2) && glog <<"Debug 0 => endfire" << std::endl;
            endfire = -1;
            DEN_SV = 0;
        }
        //*************************
    }

    u_error = 0;

    beam_no = Max_Index(P,num_beams);        

    if (abs(old_max - beam_no) > beam_changed_threshold)   //JPI/DHG 2006/07/06 Changed "7" to a parameter
    {
        beam_no = old_max;
        overiter = 1;
        glog.is(DEBUG2) && glog <<"Debug 1" << std::endl;
    }
    else
    {
        overiter = 0;
        old_max = beam_no;
        glog.is(DEBUG2) && glog <<"Debug 2" << std::endl;
    }

    if (overiter == 0)
    {
        if ((beam_no > 1) && (beam_no < num_beams))
        {
            glog.is(DEBUG2) && glog <<"Debug 3" << std::endl;
            bmid = P[beam_no];
            bleft = P[beam_no -1 ];
            bright = P[beam_no + 1];
            endfire = 0;
        }
        else if (beam_no == 1)
        {
            glog.is(DEBUG2) && glog <<"Debug 4" << std::endl;
            endfire = 1;
            DEN_SV = 0;
        }
        else if (beam_no == num_beams)
        {
            glog.is(DEBUG2) && glog <<"Debug 5" << std::endl;
            endfire = -1;
            DEN_SV = 0;
        }

        if (endfire == 0)
        {
            glog.is(DEBUG2) && glog <<"Debug 6" << std::endl;
            nmid = beam_no;
            denom = Max((bmid-bleft),(bmid-bright));
            del_cos = -(bright-bleft)/denom;
            u_est = del_cos;
            uguesso = del_cos;

            for (iter = 1; iter <= max_ubias_iter; iter++)  //JPI/DHG 2006/07/06 Change "25" to a parameter
            { 
                glog.is(DEBUG2) && glog <<"Debug 7" << std::endl;
                f = (1+K)*uguesso - K*pow(uguesso,3) - u_est;
                df = (1+K) - 3*K*pow(uguesso,2);
                uguessn = uguesso - f/df;
        
                if (fabs(uguessn - uguesso) < 0.01)
                    break;
        
                uguesso = uguessn;
            }

            if (iter == max_ubias_iter)   // JPI/DHG 2006/07/06 Replace "50" with parameter
                glog.is(VERBOSE) && glog <<"Did not converge" << std::endl;

            u_estn = Beam_cosines[nmid] + uguessn*del_beamo2;


            if (initial_time)
	      {
		glog.is(DEBUG2) && glog <<"Setting ufilt = " << u_estn << std::endl;
		ufilt = u_estn;
		initial_time = false;
		thethat1n_sv = IPS*acos(ufilt) + (ael_heading_*M_PI/180.0);
		glog.is(DEBUG1) && glog <<"thethat1n_sv = " << thethat1n_sv << std::endl;
	      }
        
            stab_az = thethat1n_sv-(ael_heading_*M_PI/180);
            glog.is(DEBUG2) && glog <<"stab_az 1 = " << stab_az
				    << ", thethat1n_sv = " << thethat1n_sv
				    << ", heading = " << ael_heading_ << std::endl;
            if(stab_az > (2.0*M_PI))
	      stab_az = stab_az - (2.0*M_PI);
            else if (stab_az < 0.0)
	      stab_az = stab_az + (2.0*M_PI);
	    glog.is(DEBUG2) && glog <<"stab_az 2 = " << stab_az
				     <<", thethat1n_sv = " <<  thethat1n_sv
				     <<", heading = " << ael_heading_ << std::endl;

	    ufilt = cos(stab_az)*cos(ael_pitch_)*cos(dep_angle) + 
	      sin(ael_pitch_)*sin(dep_angle);

	    glog.is(DEBUG2) && glog << "ufilt = " << ufilt << std::endl;
	    u_error = u_estn - ufilt;
	    
            if (fabs(u_error) > abs_uerror_threshold)  //JPI/DHG 2006/07/06 Change "1.0" to a parameter
	      u_error = udot*Actual_Integration;
	    
            // udot = udot + beta*(u_estn - ufilt)/Actual_Integration;
            udot = udot + (beta*u_error)/Actual_Integration;
            //ufilt = ufilt + (udot*Actual_Integration) + alpha*(u_estn - ufilt);
            ufilt = ufilt + (udot*Actual_Integration) + alpha*u_error;
            //glog.is(DEBUG2) && glog <<"ufilt = %lf, udot = %lf\n",ufilt,udot);
            glog.is(DEBUG2) && glog <<"udot = " << udot
				    << ",ufilt = " << ufilt
				    << ", dep_angle = " << dep_angle
				    << ", ael_pitch_ = " << ael_pitch_ 
				    << std::endl;
            DEN = (ufilt-sin(ael_pitch_)*sin(dep_angle))/
	      (cos(ael_pitch_)*cos(dep_angle));

            DEN_SV = DEN;

            if(fabs(DEN) > 1.0)
                DEN = (double) Sign(ufilt);

            NVM = IPS*pow((1-DEN*DEN),0.5);
            glog.is(DEBUG2) && glog <<"NVM = " << NVM << std::endl;
            glog.is(DEBUG2) && glog <<"DEN = " << DEN << std::endl;
            rel_bear = atan2(NVM,DEN);
            glog.is(DEBUG1) && glog <<"rel_bear = " << rel_bear << std::endl;
            //**********************
            //    if (rel_bear < 0.0)
            //      rel_bear = rel_bear + 2.0*M_PI;
            //    if (rel_bear > 2.0*M_PI)
            //      rel_bear = rel_bear - 2.0*M_PI;
            //********************


            rel_bear_sv = rel_bear*180.0/M_PI;
            argg = (u_est-sin(dep_angle)*Sign(ael_pitch_))/(cos(ael_pitch_)*cos(dep_angle));

            if(fabs(argg) <= 1.0)
            {
                thethat1 = IPS*acos(argg)*180.0/M_PI + ael_heading_;
                glog.is(DEBUG2) && glog <<"Debug 8 => thethat1 = " << thethat1 
					<< std::endl;
            }
            else if (fabs(argg) > 1.0)
            {
                thethat1 = IPS*Sign(argg) + ael_heading_;
                glog.is(DEBUG2) && glog <<"Debug 9a => thethat1 = " << thethat1 
					<< std::endl;
            }
            thethat1n = (rel_bear*180.0/M_PI) + ael_heading_;
            glog.is(DEBUG2) && glog <<"Debug 9b => thethat1n = " << thethat1n
				    << std::endl;
            if (thethat1n < 0.0)
            {   
                thethat1n = thethat1n + 360.0;
                glog.is(DEBUG2) && glog <<"Debug 10 => thethat1n = " << thethat1n
					<< std::endl;
            }
            //yes, check twice, this is not an error
            if (thethat1n < 0.0)
            {
                thethat1n = thethat1n + 360.0;
                glog.is(DEBUG2) && glog <<"Debug 11 => thethat1n = " << thethat1n
					<< std::endl;
            }

            //********************************
            if (thethat1n > 360.0)
                thethat1n = thethat1n - 360.0;
            //yes, check twice                           
            if (thethat1n > 360.0)
                thethat1n = thethat1n - 360.0;
            //*********************************    

            thethat1n_sv = thethat1n*M_PI/180.0;  // thethat1n_sv in radians
            glog.is(DEBUG2) && glog <<"Debug 11a => thethat1n_sv = " << thethat1n_sv
				    << std::endl;
        }
        else //endfire .ne. 0
        {
            u_estn = Sign(endfire);  

	    //            if (initial_time == 0)
            if (!initial_time)
            {
                thethat1n = thethat1n_sv*180.0/M_PI;
                thethat1 = thethat1n;
                glog.is(DEBUG2) && glog <<"Debug 20 => thethat1n = " 
					<< thethat1n
					<< std::endl;
            }
            else
            {
                if (endfire == 1)
                {
                    thethat1n = ael_heading_;
                    thethat1 = thethat1n;
                    glog.is(DEBUG2) && glog <<"Debug 21 => thethat1n = " << thethat1n
					    << std::endl;
                }
                else if (endfire == -1)
                {
                    thethat1n = 180.0 + ael_heading_;
                    thethat1 = thethat1n;
                    glog.is(DEBUG2) && glog <<"Debug 22 => thethat1n = "
					    << thethat1n
					    << std::endl;
                }
                initial_time = false;
            }
            rel_bear = thethat1n - ael_heading_;

            thethat1n_sv = thethat1n*M_PI/180.0;
            stab_az = thethat1n_sv-(ael_heading_*M_PI/180);
            rel_bear_sv = rel_bear;   
        }

    }//end if (overiter == 0)

    ICOAST = 0;
    IPSsv = IPS;
    endfire_out = endfire;
    //JPI/DHG 2006/07/06 if(((IPS*turn == 1)&&(fabs(rel_bear_sv) < 10.0))||

    //                      ((IPS*turn == -1)&&(fabs(rel_bear_sv) > 170.0))){

    if (!initial_time)
    {
    ///////////////////////////////BEGIN CHANGE AJP 25NOV200
        if (stab_az<=M_PI) 
            relative_az=stab_az; 
        else 
            relative_az=2*M_PI-stab_az; 
//if(((IPS*turn == 1)&&(fabs(stab_az*180.0/M_PI) < theta_min))||((IPS*turn == -1)&&(fabs(stab_az*180.0/M_PI) > (180.0-theta_min)))){ 
        if(((IPS*turn == 1)&&(fabs(relative_az*180.0/M_PI) < theta_min))||((IPS*turn == -1)&&(fabs(relative_az*180.0/M_PI) > (180.0-theta_min)))){ 
///////////////////////////////END CHANGE AJP 25NOV2006
            ICOAST = 1;
            thethat1 = aaa;
            thethat1n = bbb;
            glog.is(DEBUG2) && glog <<"Debug 23 => thethat1n = " << thethat1n
				    << std::endl;
            thethat1n_sv = bbb * M_PI / 180.0; //JPI/DHG 2006/07/06 by alex
        }
    }
//JPI/DHG 2006/07/06 if((turn == 0)||(fabs(sin(rel_bear_sv*M_PI/180.0)) > sin(10*M_PI/180.0))) 
    if ((turn == 0) || (fabs(sin(stab_az)) > sin(theta_min*M_PI/180.0))) 
      {
        ICOAST = 0;                          
      }
    glog.is(DEBUG1) && glog <<"TURN = " << turn << std::endl;
//JPI/DHG 2006/07/06 else //turn ne 0
    if (turn != 0)  // turn ne 0
      {   
//JPI/DHG 2006/07/06 IPS = Sign(sin(rel_bear_sv*M_PI/180.0));
        IPS = Sign(sin(stab_az));
        m_Comms.Notify("STAB_AZ",stab_az*180.0/M_PI);
        m_Comms.Notify("IPS",IPS);
        if (IPS == 0)
            IPS = IPSsv;
    }

    ICOAST_SV = ICOAST;
    beam_no_sv = beam_no;
    if(overiter == 0){
        aaa = thethat1;
        bbb = thethat1n;
        glog.is(DEBUG2) && glog <<"Debug 12" << std::endl;
	glog.is(DEBUG2) && glog << "thethat1n = " << thethat1n << std::endl;
        ccc = rel_bear_sv;
        ddd = IPSsv;
        eee = beam_no_sv;
        fff = head_filt_sv;
        ggg = head_filt_dot_sv;
        hhh = endfire_out;
        iii = DEN_SV;
        jjj = u_estn;
        kkk = turn;
    }
    else{
        thethat1=aaa;
        thethat1n=bbb;  
        glog.is(DEBUG2) && glog <<"Debug 13" << std::endl;
	  glog.is(DEBUG2) && glog <<"thethat1 = " << thethat1n << std::endl;
        rel_bear_sv=ccc;
        IPSsv=ddd;
        beam_no_sv=eee;
        head_filt_sv=fff;
        head_filt_dot_sv=ggg;
        endfire_out=hhh;
        DEN_SV=iii;
        u_estn=jjj;
        turn=kkk; 
    }

//compute the measurement standard deviation
    double msigma = get_msigma(); 

    double rate,rate_sigma;
    Track.bearingRate(thethat1n, msigma, rate, rate_sigma);
    Track.setTrack(1, 10, 0, TRACKING, IPS, ael_x_, ael_y_, thethat1n, msigma, rate, rate_sigma, 0.0, class_features);
    postTrack();

    glog.is(VERBOSE) && glog <<"Bearing = " << thethat1n << std::endl;
    return true;
}

//***************************************************************************
//*  LamssBearingTracker::track_lr()                                                  *
//*                                                                         *
//*  track_lr() determines whether the source is on the left or right side  *
//*  of our line array                                                      *
//*                                                                         *
//*************************************************************************** 
int LamssBearingTracker::track_lr()
{

    int maxidx,cw,enfidx;

    lrcount++;

    maxidx = Max_Index(P,num_beams);

    enfidx= (int) floor(lridx*1.5);

    string msg;
    char buff[200];
    double ambig_angle = acos(Beam_cosines[maxidx])*180.0/M_PI;

    class_features.clear();
    Track.setTrack(1, 10, 0, LR_TRACK, 0, ael_x_, ael_y_, ambig_angle, 0.0, 0.0, 0.0, 0.0, class_features);
    postTrack();

 

    if(!idx_set)
    { 
//make sure target is not close to endfire
        if ((maxidx > enfidx)&&(maxidx < (num_beams-enfidx+1)))
        {
            save_idx = maxidx;
            heading_save = ael_heading_;
            if(heading_save < 0.0)
                heading_save = heading_save + 360.0;
            idx_set = true;
            return 0;
        }
        else
            return 0;
    }
    else
    { //have target moved enough beams to make a decision?
        if (abs(maxidx - save_idx) >= lridx)
        {

//did we rotate CW or CCW?
            double hdiff = heading_save-ael_heading_;     
            if (hdiff < 0.0){
                if (hdiff < -180.0)
                    cw = -1;
                else 
                    cw = 1;
            }
            else{
                if (hdiff > 180.0)
                    cw = 1;
                else 
                    cw = -1;
            }
            glog.is(DEBUG2) && glog <<"cw = " << cw << std::endl;
            glog.is(DEBUG2) && glog <<"hdiff = " << hdiff << std::endl;
            m_Comms.Notify("CW",cw);

            if (maxidx < save_idx)
                return 1*cw;
            else
                return -1*cw;
        }
        return 0;
    }
}

//***************************************************************************
//*  LamssBearingTracker::detection()                                                 *
//*                                                                         *
//*  Detect() creates a statistic for the ambient noise power and then      *
//*  declares a detection if the max power in a beam exceeds this by some   *
//*  amount.                                                                *
//*************************************************************************** 

bool LamssBearingTracker::detection()
{

    int i; // AJP 4 OCT 2007        
    int maxidx;
    double maxpow,sqterm;

//find the beam with the maximum power
    maxidx = Max_Index(P,num_beams);

    maxpow = P[maxidx];

    glog.is(DEBUG2) && glog <<"noise_var_set = " << noise_var_set
			    << ",ael_depth_ = " << ael_depth_ 
			    << ", detector_type = "<< detector_type << std::endl;


//compute the "variance" statistic 
    if( (!noise_var_set) && (ael_depth_ > noise_depth) )
    {
        //dcount ++;
        if (detector_type==0)
        {
            glog.is(DEBUG2) && glog <<"In Detector Type 0 " << std::endl;    
            
            dcount ++;
        
            maxsum += maxpow;

            maxmean = maxsum/dcount;

            sqterm = pow((maxpow-maxmean),2);

            sigmasum += sqterm;

            sigma = pow(sigmasum/(dcount-1),0.5);

            final_detection_threshold=(detection_threshold*sigma+maxmean);

            amb_noise_mean=maxmean;
        }  
        else if (detector_type==1)
        {
            glog.is(DEBUG2) && glog <<"In Detector Type 1 " << std::endl;
            int idum = asort(num_beams,P,P_indx,P_sorted,P_indx_sorted);

            for (i = detector_first_bin; i <= (detector_first_bin + detector_num_bins - 1); i++)
                mean_sum += P_sorted[i];

            if (!isnan(mean_sum))
            {
                dcount ++;
                amb_noise_mean=mean_sum/(dcount*detector_num_bins);
            
		glog.is(DEBUG1) && glog <<"amb_noise_mean = " << amb_noise_mean
					<< std::endl;

                for (i = detector_first_bin; i <= (detector_first_bin + detector_num_bins - 1); i++)
                {
                    std_dev_sum += pow((P_sorted[i]-amb_noise_mean),2);
                }
                sigma = pow(std_dev_sum/(dcount*detector_num_bins-1),0.5);
    
                final_detection_threshold=(detection_threshold*sigma+amb_noise_mean);

                maxsum += maxpow; //ONLY USED BY get_msigma()

                maxmean = maxsum/dcount; //ONLY USED BY get_msigma()
            }
        }
        else //if (detector_type==2)
        {
            glog.is(DEBUG2) && glog <<"In Detector Type 2 " << std::endl;
            int idum = asort(num_beams,P,P_indx,P_sorted,P_indx_sorted);
            mean_sum = 0.0;
            for (i = detector_first_bin; i <= (detector_first_bin + detector_num_bins - 1); i++)
                mean_sum += P_sorted[i];

            if (!isnan(mean_sum))
            {
                dcount ++;
                if (dcount==1)
                {    
                    amb_noise_mean=(mean_sum/detector_num_bins);
                }
                else
                {
                    amb_noise_mean=amb_noise_mean*detector_alpha+(mean_sum/detector_num_bins)*(1-detector_alpha);
                }
    
		glog.is(DEBUG1) && glog <<"amb_noise_mean = " << amb_noise_mean 
					<< std::endl;
                std_dev_sum = 0;
                for (i = detector_first_bin; i <= (detector_first_bin + detector_num_bins - 1); i++)
                {
                    std_dev_sum += pow((P_sorted[i]-amb_noise_mean),2);
                }

                if (dcount==1)
                {    
                    sigma = pow(std_dev_sum/detector_num_bins,0.5);
                    maxmean = maxpow; //ONLY USED BY get_msigma()
                }
                else
                {
                    sigma = sigma*detector_alpha+pow(std_dev_sum/detector_num_bins,0.5)*(1-detector_alpha);
                    maxmean = maxmean*detector_alpha+maxpow*(1-detector_alpha); //ONLY USED BY get_msigma()
                }
    
                final_detection_threshold=(detection_threshold*sigma+amb_noise_mean);
            }
            else
            {
                glog.is(DEBUG1) && glog << "NAN Detection: mean_sum = "
					<< mean_sum << std::endl;
            }
            if (dcount>=1)
            {
	      glog.is(DEBUG2) && glog << "amb_noise_mean = " << amb_noise_mean*1e-15
				      << ", sigma = " << sigma*1e-15
				      << ", threshold = " 
				      << final_detection_threshold*1e-15
				      << ", (maxpow-mean)/sigma = "
				      << (maxpow-amb_noise_mean)/sigma
				      << ", maxpow/threshold = "
				      << maxpow/final_detection_threshold 
				      << std::endl;
            }
        }

        string msg_detect;
        char buff_detect[200];
        sprintf(buff_detect,"noise_mean = %.3f, noise_sigma = %.3f, threshold = %.3f, (maxpow-mean)/sigma = %.3f, maxpow/threshold = %.4f \n",amb_noise_mean*1e-12, sigma*1e-12,final_detection_threshold*1e-12, (maxpow-amb_noise_mean)/sigma, maxpow/final_detection_threshold);  
        msg_detect = buff_detect;
        m_Comms.Notify ("DETECTOR_STAT", msg_detect);

    }

    if(detect_mode)
    {  
        glog.is(DEBUG1) && glog <<"detect_mode=TRUE " << std::endl;
//statistic only valid for > 1 measurement
//determine if a beam exceeds our statistic
        if(dcount > 1)
        {

//sigma = pow(sigmasum/(dcount-1),0.5); //MOVED BELOW AJP 4 OCT 2007
// if (maxpow > (detection_threshold*sigma+maxmean))   
            if (maxpow > final_detection_threshold)     
            {
                det_count++;
                missed_count = 0;
                if (detector_type==2)
                {    
                    noise_var_set = false;
                }
                else
                {
                    noise_var_set = true;
                }
    //wait for "detection_count_threshold" (5 usually) detections in a row
                if (det_count >= detection_count_threshold) //JPI/DHG 2006/07/06 Change "5" to a parameter
                {

                    return true;
                }
            }
            else
            {
    //no detection  
                det_count = 0;
                missed_count++;
                if (missed_count >= detection_count_threshold)
                {
                    if(noise_var_set || detector_type == 2 )    
                        return false;
                }    
            }
        }
    }
    else 
    {
        glog.is(DEBUG1) && glog <<"detect_mode=FALSE " << std::endl;
    }


//not 5 hits or misses in a row so don't change state
    if (IPS < -1)
        return false;
    else
        return true;
}



void LamssBearingTracker::reset_tracking()
{
    glog.is(VERBOSE) && glog <<"RESETTING!!!!!!!!!!!!!!" << std::endl;
    ufilt = 0.0;
    udot = 0.0;
    head_filt = ael_heading_;
    head_filt_dot = 0.0;
    initial_time = true;
    IPS = -100;
    idx_set = false;
    endfire = 0;
    u_estn = 0.0;
    lrcount = 0;
    overiter = 0;
    bbb = 0;
    thethat1 = 0;
    thethat1n = 0;
    dcount = 0;
    lrcount = 0;
    class_features.clear();
    Track.setTrack(1, 0, 0, NO_TRACK, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, class_features);
    postTrack();

}

void LamssBearingTracker::reset_lr()
{
// redoing the LR ambiguity sequence
    glog.is(VERBOSE) && glog <<"RESETTING LR AMBIGUITY!!!!!" << std::endl;
    ufilt = 0.0;
    udot = 0.0;
    head_filt = ael_heading_;
    head_filt_dot = 0.0;
    initial_time = true;
    IPS = -100;
    idx_set = false;
    endfire = 0;
    u_estn = 0.0;
    thethat1 = 0;
    thethat1n = 0;
    lrcount = 0;

    class_features.clear();
    Track.setTrack(1, 0, 0, LR_TRACK, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, class_features);
    postTrack();

}

////////////////////////////////////////////////////////////////////////////////////////////////////

// File:        asort.c
// Author:      DHG
// Date:        2007/05/17
// Revisions: 
// Description: Ascending and Descending bubble sorts which return a vector of 
//              sorted values as well as the corresponding indeces of those 
//              values. It assumes that 2 vectors enter: the values and the 
//              indeces of the values.

int LamssBearingTracker::asort (int num_elems, double *values, int *indeces, 
               double *sorted_values, int *sorted_indeces)
{
    int    i, j;
    int    index_temp;
    double value_temp;

    if (num_elems <= 0)
    {
        return (0);
    }

// Make a local copy of the entering vectors

    for (i = 1; i <= num_elems; i++)
    {
        sorted_values[i] = values[i];
        sorted_indeces[i] = indeces[i];
    }

    if (num_elems == 1)
    {
        return (1);
    }

// Acsending bubble sort by value. 
// Preserve the indeces as the values are sorted.

    for (j = 1; j <= num_elems; j++)
    {
        for (i = 1; i <= num_elems-1; i++)
        {
            if (sorted_values[i] > sorted_values[i+1])
            {
                value_temp = sorted_values[i];
                sorted_values[i] = sorted_values[i+1];
                sorted_values[i+1] = value_temp;
                index_temp = sorted_indeces[i];
                sorted_indeces[i] = sorted_indeces[i+1];
                sorted_indeces[i+1] = index_temp;
            }
        }
    }

    return (1);
}

///////////////////////////////////////////////////////////////////////////////

int LamssBearingTracker::dsort (int num_elems, double *values, int *indeces, 
               double *sorted_values, int *sorted_indeces)
{
    int    i, j;
    int    index_temp;
    double value_temp;

    if (num_elems <= 0)
    {
        return (0);
    }

// Make a local copy of the entering vectors

    for (i = 1; i <= num_elems; i++)
    {
        sorted_values[i] = values[i];
        sorted_indeces[i] = indeces[i];
    }

    if (num_elems == 1)
    {
        return (1);
    }

// Decsending bubble sort by value. 
// Preserve the indeces as the values are sorted.

    for (j = 1; j <= num_elems; j++)
    {
        for (i = 1; i <= num_elems-1; i++)
        {
            if (sorted_values[i] < sorted_values[i+1])
            {
                value_temp = sorted_values[i];
                sorted_values[i] = sorted_values[i+1];
                sorted_values[i+1] = value_temp;
                index_temp = sorted_indeces[i];
                sorted_indeces[i] = sorted_indeces[i+1];
                sorted_indeces[i+1] = index_temp;
            }
        }
    }

    return (1);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////



//***************************************************************************
//*  LamssBearingTracker::get_msigma()                                                *
//*                                                                         *
//*  return the std dev of the current bearing measurement                  *
//*                                                                         *
//*************************************************************************** 

double LamssBearingTracker::get_msigma()
{
    int maxidx;
    double maxpow, SNR, sigma_u, sigma_theta, sigma_theta_deg, rel_az, temp_az;

    maxidx = Max_Index(P,num_beams);
    maxpow = P[maxidx];
    SNR=(maxpow-maxmean)/maxmean;

    sigma_u = (sound_speed/(M_PI*mean_frequency*mean_element_spacing)) * pow((3/(SNR*num_elements_used*(num_elements_used*num_elements_used-1))),0.5);

    if (stab_az<0)
        temp_az=stab_az+2.0*M_PI;
    else if (stab_az>=(2.0*M_PI))
        temp_az=stab_az-2.0*M_PI;
    else
        temp_az=stab_az;
    if (stab_az<=M_PI)
        rel_az=temp_az;
    else
        rel_az=2*M_PI-temp_az;
    if (rel_az <= M_PI*0.5)
        sigma_theta = acos(cos(rel_az)-sigma_u) - rel_az;
    else //rel_az >M_PI*0.
        sigma_theta = rel_az - acos(cos(rel_az)+sigma_u);
    sigma_theta_deg=sigma_theta*180.0/M_PI;
//glog.is(DEBUG2) && glog <<"SNR (linear) = %.3f, SNR (dB) = %.3f, maxidx = %d, maxpow = %.3f, maxmean = %.3f, sigma_u=%.3f, sigma_theta=%.3f, sigma_theta_deg=%.3f, stab_az = %.3f\n, rel_az = %.3f\n",SNR,(10.0*log(SNR)/log(10.0)),maxidx,maxpow,maxmean,sigma_u,sigma_theta,sigma_theta_deg,(stab_az*180.0/M_PI),(rel_az*180.0/M_PI))
    return sigma_theta_deg;
}

double LamssBearingTracker::angle360(double angle)
{
  double res = angle;
  //  unwrap angles
  while (res >= 360.0)
    res -= 360.0;
  while (res < 0.0)
    res += 360.0;
  
  return(res);
  
}

// >>>>>>>>>>>>>>> vector utility methods <<<<<<<<<<<<
int LamssBearingTracker::str2dbl_vec(string msg, vector<double>& values)
{
  string bite;
  
  values.clear();
  int nz = 0;
  while (msg.length() > 0)
    {
      bite = MOOSChomp(msg,",");
      values.push_back(atof(bite.c_str()));
      nz++;
    }
  
  return(nz);
}

bool LamssBearingTracker::dbl_vec2str(vector<double> numbers, string& output)
{
  output = "";
  for(int i=0; i<numbers.size(); i++) 
    {
      if(i > 0)
    output += ",";
      string istr = doubleToString(numbers[i], 2); // 2 is the precision 123.45
      output += istr;
    }
  return(true);
}

bool LamssBearingTracker::postTrack()
{
  string moosvar;
  string svar;
  Track.postTrack(moosvar,svar);
  m_Comms.Notify(moosvar,svar);
  return(true);
}

