/************************************************************/
/*    NAME: Nick Rypkema                                    */
/*    ORGN: MIT                                             */
/*    FILE: SimAcoBeaconRngBrg.h                            */
/*    DATE: April 2017                                      */
/************************************************************/

#ifndef SimAcoBeaconRngBrg_HEADER
#define SimAcoBeaconRngBrg_HEADER

#include "MOOS/libMOOS/MOOSLib.h"
#include "XYPolygon.h"            //needed for drawing sensor swath arc
#include "XYFormatUtilsPoly.h"
#include <cmath>

#include "goby/moos/protobuf/frontseat.pb.h"
#include "goby/moos/frontseat/bluefin/bluefin.pb.h"
#include "goby/util/linebasedcomms/nmea_sentence.h"

class SimAcoBeaconRngBrg : public CMOOSApp
{
 public:
   SimAcoBeaconRngBrg();
   ~SimAcoBeaconRngBrg();

 protected:
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();
   void RegisterVariables();
   double Randn();
   bool CheckSensor();
   void MeanParticles();
   void StdDevParticles();
   void handle_ifs_data(const CMOOSMsg& msg);


 private: // Configuration variables
   double       m_aco_beacon_x;             // acoustic beacon position
   double       m_aco_beacon_y;
   double       m_sensor_range;             // sensor max range and swath
   double       m_sensor_swath;
   double       m_sensor_range_stddev;      // sensor standard deviations of range and swath
   double       m_sensor_swath_stddev;
   double       m_random_walk_stddev;       // standard deviation for random walk of beacon location
   double       m_sensor_probability;       // probability of a successful sensor measurement
   double       m_sensor_period;            // time between successive sensor measurements
   unsigned int m_num_particles;
   std::string  m_update_var;

 private: // State variables
   goby::moos::protobuf::NodeStatus last_nav_;
   double       m_prev_time;
   double       m_dt_accum;
   double       m_nav_x;
   double       m_nav_y;
   double       m_nav_set;
   double       m_target_x;
   double       m_target_y;
   std::vector<double> m_particles_x;
   std::vector<double> m_particles_y;
   std::vector<double> m_particles_a;
   std::vector<double> m_particles_r;
   double       m_target_a;
   double       m_target_r;
   bool         m_target_found;
   double       m_nav_heading;
   XYPolygon    m_sensor_arc;               // plot the sensor arc
   double       m_std_azimuth;
   double       m_std_range;
   bool         m_usbl_enabled;
   unsigned int m_iterations;
   double       m_timewarp;
};

#endif
