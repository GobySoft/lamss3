/************************************************************/
/*    NAME: Supun Randeni                                              */
/*    ORGN: MIT                                             */
/*    FILE: SimDVL.h                                          */
/*    DATE:                                                 */
/************************************************************/

#ifndef SimDVL_HEADER
#define SimDVL_HEADER

#include "MOOS/libMOOS/MOOSLib.h"

class SimDVL : public CMOOSApp
{
 public:
   SimDVL();
   ~SimDVL();

 protected: // Standard MOOSApp functions to overload  
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();

 protected:
   void RegisterVariables();

 private: // Configuration variables
    double c_dvl_update_frequency;
    double c_dvl_velocity_std;
    double c_dvl_velocity_bias;
    int c_ice_tracking_mode;
    int c_dvl_bottom_lock;
    std::string c_ice_sheet_name;
    double c_inverted_dvl;

 private: // State variables
    double m_dvl_data_post_time;
    double m_current_nav_depth;
    double m_current_nav_depth_time;
    double m_previous_nav_depth;
    double m_previous_nav_depth_time;
    
    double m_current_u;
    double m_current_v;
    double m_current_w;
    double m_current_u_time;
    double m_previous_u_time;
    double m_current_nav_roll;
    double m_current_nav_pitch;
    double m_current_nav_heading;
    std::string node_name;
    double m_icecamp_speed;
    double m_icecamp_heading;
    double m_icecamp_u;
    double m_icecamp_v;
    double m_icecamp_ydot;
    double m_icecamp_xdot;
    double m_icecamp_w;
    double m_current_water_vel_x;
    double m_current_water_vel_y;

    double auvVelRelIce_xdot;
    double auvVelRelIce_ydot;
    double auvVelRelIce_zdot;
};

#endif 
