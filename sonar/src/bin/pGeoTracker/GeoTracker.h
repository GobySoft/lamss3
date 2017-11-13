#include <MOOS/libMOOS/MOOSLib.h>
#include "MBUtils.h"
#include <MOOS/libMOOS/Utils/MOOSUtils.h>
#include <math.h>
#include<time.h>
#include <iostream>

#include<string>
#include<vector>

#include "GeoTrack.h"

#include "lamss/lib_henrik_util/parse_stat.h"
#include "lamss/lib_henrik_util/Measurement.h"
#include "lamss/lib_henrik_util/CVectorUtils.h"

#define NO_TRACK 0
#define LR_TRACK 1
#define TRACKING 2


using namespace std;


//typedef enum{
#define    NO_CONTACT                  0
#define    AMBIGUOUS_BEARING           1
#define    BEARING_ONLY                2
#define    RANGE_ONLY                  3
#define    RANGE_AMBIGUOUS_BEARING     4
#define    RANGE_BEARING               5
#define    RANGE_BEARING_HEADING_SPEED 6
      //}CONTACT_TYPE;

#define SINGLE_VEHICLE_TRACKING 0
#define MULTI_VEHICLE_TRACKING 1

class CINFO
{
 public:
  CINFO();
  ~CINFO();

  int          num_meas;

  char         id;
  int          Ctype;
  double       range;
  double       bearing;
  double       heading;
  double       speed;
  char         hours;
  char         minutes;
  char         seconds;
  double       x_pos;
  double       y_pos;

  double       Last_Meas_Time;
  double       total_time;

};

class Contact
{
 public:
  Contact();
  Contact(int id);
  ~Contact();

  double       range_guess;

  int          num_meas;

  char         id;
  int          Ctype;
  double       range;
  double       bearing;
  double       heading;
  double       speed;
  char         hours;
  char         minutes;
  char         seconds;
  double       x_pos;
  double       y_pos;

  double       Last_Meas_Time;
  double       total_time;
  double       start_time; 

  CGeoTrack Topt;

  void set_range_guess(double range);
  
};

class Contact_List
{
 public:

  Contact_List();
  ~Contact_List();

  int associate_bearing(Measurement*);
  int get_num_contacts();
  void set_num_contacts(int);
  void update_contact(int,Measurement*);
  void get_Cinfo(int,CINFO*);
  void reset();
  void reset_solution();
  void setTargetInfo(double, double, double, double, double);
  void setTargetCue(double, double, double, double, double);

  double start_time;
  double range_guess,range_max,speed_max;
  double range_min;

  int Z_length,skipped,min_meas,tgt_msg_ready;

  double tgt_x;
  double tgt_y;
  double tgt_utc;
  double tgt_hdg;
  double tgt_spd;

  double cue_x;
  double cue_y;
  double cue_utc;
  double cue_hdg;
  double cue_spd;

  double auv_x;
  double auv_y;
  double tgt_bearing;
  double bearing_time;
  double tgt_x_est;
  double tgt_y_est;
  double tgt_spd_est;
  double tgt_hdg_est;
  bool tgt_est;
  bool tgt_track;

 protected:

  int num_contacts;
  vector<Contact> Contacts;

  bool SolveTrack(int,double,double);
  bool Add_Measurement(int,Measurement*);

};


    
class CGeoTracker : public CMOOSApp  
{
public:
	CGeoTracker();
	virtual ~CGeoTracker();

	double  get_veh_x();
	double  get_veh_y();

protected:

	int max_meas,min_meas,bstate,tstate,Mnum,beamno;
	int init_meas,nmeas,meas_count,local_node,track_count;
	bool solution,tracking,close_range,prosecute;
	double total_time,cBearing,start_time,trange,last_btime,auv_speed,dtheta;
	double prosecute_target_range,range_guess;

	int vehicleID,tracking_mode;

	std::string target_id;
	std::string tgt_navx_var;
	std::string tgt_navy_var;
	std::string tgt_navutc_var;
	std::string tgt_navhdg_var;
	std::string tgt_navspd_var;

	double tgt_x;
	double tgt_y;
	double tgt_utc;
	double tgt_hdg;
	double tgt_spd;

	int cue_number;
	double cue_x;
	double cue_y;
	double cue_z;
	double cue_utc;
	double cue_hdg;
	double cue_spd;
	double cue_freq;
	double cue_bw;
	double cue_spl;
	double cue_str;
	std::vector<double> cue_tone_freq;
	std::vector<double> cue_tone_dbl;

	// Bearing rate variables
	CmyVectorUtils vec;
	double bearing_rate_interval;
	double bearing_rate_threshold;
	std::vector<double> times;
	std::vector<double> bearings;


	Contact_List CList;

	// Local methods
	//	bool decode_tgt_state(string);
	bool postTargetNav(double, double, double, double, double);



	//standard MOOS methods
	bool Iterate();
	bool OnNewMail(MOOSMSG_LIST &NewMail);
	bool OnConnectToServer();
	bool OnStartUp();	
	bool GetData();
	
        void reset();
        void single_reset();
	bool checkBearingRate(double curr_time, double true_bearing);

	//get new track solution
	//bool SolveTrack();
	//bool Add_Measurement();

	//track optimization object
	//contains methods for computing the 1-bearing
	//track solution
	//CGeoTrack Topt;
};


