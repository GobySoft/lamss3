#ifndef SIMMODEMNETWORK_HEADER
#define SIMMODEMNETWORK_HEADER

#include <string>
#include <vector>
#include <math.h>
#include "MOOS/libMOOS/MOOSLib.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"
#include "MBUtils.h"
#include "goby/moos/goby_moos_app.h"
#include "ivp/MOOSGeodesy.h"
#include "lamss/lib_newmat10D/newmatap.h"
#include <lamss/lib_henrik_util/CEigenRays.h>
#include "lamss/lib_bellhop/svp_request_response.pb.h"
#include "lamss/lib_bellhop/iBellhop_messages.pb.h" 
#include "lamss/lib_lamss_protobuf/modem_sim.pb.h" 
#include "goby/moos/moos_protobuf_helpers.h"
#include "SimModemNetwork_config.pb.h"


// from lib_henrik_util
#include <lamss/lib_henrik_util/str_num.h>
#include <lamss/lib_henrik_util/CTarget.h>


#include "lamss/lib_henrik_util/CVectorUtils.h"
#include "lamss/lib_henrik_util/Random.h"

class CAcousticNode
{
 public:
  std::string name;
  bool ok_info;
  double utc;
  double x;
  double y;
  double depth;
  double heading;
  double speed;
  double pitch;
};

class CAcousticContact
{
 public:
  std::string ownship;
  std::string contact;
  bool ready;
  double last_request;
  double last_range;
  double last_rate;
  double last_rate_update;
  int bellhop_request_id;
  bool svp_ready;
  int svp_request_id;
  double last_svp_request;
  CEigenRayTable  ray_table;
  SVP::Response svp_response;
};

class SimModemNetwork : public GobyMOOSApp
{
  public:
    static void* driver_library_handle_;
    static SimModemNetwork* get_instance();

  private:
    SimModemNetwork(SimModemNetworkConfig& cfg);
    ~SimModemNetwork() { }

    // synchronous event
    void loop();    
    void slow_loop(double curr_time);
    double slow_loop_period;

    // mail handlers

    void handle_transmit_test(const CMOOSMsg& msg);
    void handle_bellhop_response(const CMOOSMsg& msg);
    void handle_svp_response(const CMOOSMsg& msg);
    void handle_node_report(const CMOOSMsg& msg);
    void handle_impulse_request(const CMOOSMsg& msg);

    SimModemNetworkConfig& cfg_;
    double last_slow_loop_t_;
    
    static SimModemNetwork* inst_;    

  protected:

    bool debug;
    bool debug2;

    CMOOSGeodesy m_Geodesy;
    bool mseas_svp_updates;
    double mseas_update_interval;
    double svp_range_resolution;
    double svp_depth_resolution;
    std::vector<std::string> svp_requests;
    std::vector<std::string> bellhop_requests;
    double sound_speed;
    double acomms_frequency;

    CmyVectorUtils vecUtils;
 
    //---Declare variables that get their valies directly from the .moos file---
    // Time stam for each recorded file
    double timestamp;

    // Multi target expansion

    bool getAcousticContact(std:: string os, std::string cn, CEigenRayTable& eig_table);
    int  findAcousticContact(std:: string os, std::string cn);
    bool readyAcousticContact(std:: string os, std::string cn);
    bool requestRayTrace(std:: string os, std::string cn, 
			 double r_min, double r_max, int num_r, 
			 double dep_min, double dep_max, int num_dep,
			 double freq);
    bool requestSVPUpdate(std:: string os, std::string cn);
    bool readyAcousticContactSVP(std:: string os, std::string cn);
    bool putAcousticContactSVP(SVP::Response svp_response);
    bool getAcousticContactSVP(std:: string os, std::string cn, 
			       SVP::Response& svp_response );
    bool handleSVPResponse(const CMOOSMsg& msg);
    bool defineRayTracingBox(CAcousticContact& contact,
			     double& r_min,
			     double& r_max,
			     double& dep_min,
			     double& dep_max,
			     int& num_r,
			     int& num_dep);
    bool getNodeInfo(std::string node_name,
		     double& node_x,
		     double& node_y,
		     double& node_depth, 
		     double& node_heading,
		     double& node_speed);
    
    std::vector<CAcousticContact> acoustic_contacts;

    double start_time;

    std::vector<CAcousticNode> Nodes;

    int max_number_bounces;
    std::string arr_file_path;

    // ownship and contact navigation
    double ownship_x;
    double ownship_y;
    double ownship_depth;
    double ownship_heading;
    double ownship_pitch;
    double ownship_speed;

    double contact_x;
    double contact_y;
    double contact_depth;
    double contact_heading;
    double contact_pitch;
    double contact_speed;

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
 
    // doubles to keep track of time
    double time_last;
    double file_interval_time;
    int frame_number;

    // --- Functions ---

    bool generate_eigenray_table(CAcousticContact& acoustic_contact);

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
    void modem_ray_arrivals(CEigenRayTable eigenray_table,
			    std::vector<double> & delay,
			    std::vector<double> & amplitude,
			    std::vector<double> & doppler,
			    std::vector<double> & elevation);
    
      
};

#endif 

