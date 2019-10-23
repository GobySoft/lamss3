/************************************************************/
/*    NAME: Supun Randeni                                              */
/*    ORGN: MIT                                             */
/*    FILE: SimPhinsINS.cpp                                        */
/*    DATE:                                                 */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "SimPhinsINS.h"
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <random>

using namespace std;
std::default_random_engine gen;
//---------------------------------------------------------
// Constructor

SimPhinsINS::SimPhinsINS()
{
  m_current_u_received = false;
  m_current_u_time = MOOSTime();
  m_prev_u_time = MOOSTime();
  m_prev_auv_u = 0;
  m_prev_auv_v = 0;
  m_prev_auv_w = 0;
  m_ins_update_no=0;
}

//---------------------------------------------------------
// Destructor

SimPhinsINS::~SimPhinsINS()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool SimPhinsINS::OnNewMail(MOOSMSG_LIST &NewMail)
{
  MOOSMSG_LIST::iterator p;
   
  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key   = msg.GetKey();
    double dval  = msg.GetDouble();
    string sval  = msg.GetString();

#if 0 // Keep these around just for template
    string key   = msg.GetKey();
    string comm  = msg.GetCommunity();
    double dval  = msg.GetDouble();
    string sval  = msg.GetString(); 
    string msrc  = msg.GetSource();
    double mtime = msg.GetTime();
    bool   mdbl  = msg.IsDouble();
    bool   mstr  = msg.IsString();
#endif
    if (key == "FRONTSEAT_BODY_VEL_Y") {
      m_current_u = dval;             // MOOS convension Y is surge
      m_current_u_time = MOOSTime();
      m_current_u_received = true;
      m_ins_update_no +=1;
    }
    else if (key == "FRONTSEAT_BODY_VEL_X") {
      m_current_v = dval;             // MOOS convension X is sway
    }
    else if (key == "FRONTSEAT_BODY_VEL_Z") {
      m_current_w = -dval;             // MOOS convension Z is up
    }
    else if (key == "CURRENT_X") {
      m_current_water_vel_x = dval;  // east
    }
    else if (key == "CURRENT_Y") {
      m_current_water_vel_y = dval;  // north
    }
    else if(key == "FRONTSEAT_DEPTH") {
      m_current_nav_depth = dval;
      m_current_nav_depth_time = MOOSTime();
    }
    else if(key == "FRONTSEAT_ROLL") {
      m_current_nav_roll = dval*180/M_PI;
    }
    else if(key == "FRONTSEAT_PITCH") {
      m_current_nav_pitch = dval*180/M_PI;
    }
    else if(key == "FRONTSEAT_HEADING") {
      m_current_nav_heading = dval;
    }
    else if(key == "FRONTSEAT_X") {
      m_current_nav_x = dval;
    }
    else if(key == "FRONTSEAT_Y") {
      m_current_nav_y = dval;
    }

    
   }
	
   return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer

bool SimPhinsINS::OnConnectToServer()
{
   // register for variables here
   // possibly look at the mission file?
   // m_MissionReader.GetConfigurationParam("Name", <string>);
   // m_Comms.Register("VARNAME", 0);
	
   RegisterVariables();
   return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool SimPhinsINS::Iterate()
{
  // Converting ground relative AUV velocity from body frame to earth frame
  double current_ydot = m_current_u*cos(m_current_nav_heading*M_PI/180)*cos(m_current_nav_pitch*M_PI/180)+m_current_v*(cos(m_current_nav_heading*M_PI/180)*sin(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_roll*M_PI/180) - sin(m_current_nav_heading*M_PI/180)*cos(m_current_nav_roll*M_PI/180))+m_current_w*(sin(m_current_nav_heading*M_PI/180)*sin(m_current_nav_roll*M_PI/180)+cos(m_current_nav_heading*M_PI/180)*cos(m_current_nav_roll*M_PI/180)*sin(m_current_nav_pitch*M_PI/180));
  double current_xdot = m_current_u*sin(m_current_nav_heading*M_PI/180)*cos(m_current_nav_pitch*M_PI/180)+m_current_v*(cos(m_current_nav_heading*M_PI/180)*cos(m_current_nav_roll*M_PI/180)+sin(m_current_nav_roll*M_PI/180)*sin(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_heading*M_PI/180))+m_current_w*(sin(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_heading*M_PI/180)*cos(m_current_nav_roll*M_PI/180)-cos(m_current_nav_heading*M_PI/180)*sin(m_current_nav_roll*M_PI/180));
  double current_zdot = m_current_u*-1*sin(m_current_nav_pitch*M_PI/180) + m_current_v*cos(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_roll*M_PI/180) + m_current_w*cos(m_current_nav_pitch*M_PI/180)*cos(m_current_nav_roll*M_PI/180);
  // Acounting water current
  double GrndRel_auv_xdot = current_xdot + m_current_water_vel_x;
  double GrndRel_auv_ydot = current_ydot + m_current_water_vel_y;
  double GrndRel_auv_zdot = current_zdot;
  // Converting earth relative (without gravity) AUV velocity from earth-frame to body frame
  double auv_u = GrndRel_auv_ydot*cos(m_current_nav_pitch*M_PI/180)*cos(m_current_nav_heading*M_PI/180) + GrndRel_auv_xdot*cos(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_heading*M_PI/180) - GrndRel_auv_zdot*sin(m_current_nav_pitch*M_PI/180);
  double auv_v = GrndRel_auv_ydot*(sin(m_current_nav_roll*M_PI/180)*sin(m_current_nav_pitch*M_PI/180)*cos(m_current_nav_heading*M_PI/180) - cos(m_current_nav_roll*M_PI/180)*sin(m_current_nav_heading*M_PI/180)) + GrndRel_auv_xdot*(sin(m_current_nav_roll*M_PI/180)*sin(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_heading*M_PI/180)+cos(m_current_nav_roll*M_PI/180)*cos(m_current_nav_heading*M_PI/180)) + GrndRel_auv_zdot*sin(m_current_nav_roll*M_PI/180)*cos(m_current_nav_pitch*M_PI/180);
  double auv_w = GrndRel_auv_ydot*(cos(m_current_nav_roll*M_PI/180)*sin(m_current_nav_pitch*M_PI/180)*cos(m_current_nav_heading*M_PI/180)+sin(m_current_nav_roll*M_PI/180)*sin(m_current_nav_heading*M_PI/180)) + GrndRel_auv_xdot*(cos(m_current_nav_roll*M_PI/180)*sin(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_heading*M_PI/180)-sin(m_current_nav_roll*M_PI/180)*cos(m_current_nav_heading*M_PI/180)) + GrndRel_auv_zdot * cos(m_current_nav_roll*M_PI/180)*cos(m_current_nav_pitch*M_PI/180);
  MOOSTrace("Vel: u=%.4f, v=%.4f, w=%.4f \n",auv_u,auv_v,auv_w);
  // Differentiate to Calculate acceleration components
  if (m_current_u_received == true && (m_current_u_time-m_prev_u_time)<2)
  {
    m_auv_udot = (auv_u-m_prev_auv_u)/(m_current_u_time-m_prev_u_time);
    m_auv_vdot = (auv_v-m_prev_auv_v)/(m_current_u_time-m_prev_u_time);
    m_auv_wdot = (auv_w-m_prev_auv_w)/(m_current_u_time-m_prev_u_time);
  }
  // Adding INS uncertainties
  std::normal_distribution<double> dist_ins_stdev_udot(m_auv_udot, c_ins_stdev); // stdev
  std::normal_distribution<double> dist_ins_stdev_vdot(m_auv_vdot, c_ins_stdev); // stdev
  std::normal_distribution<double> dist_ins_stdev_wdot(m_auv_wdot, c_ins_stdev); // stdev
  m_ins_udot = dist_ins_stdev_udot(gen) + c_ins_scale_err*m_auv_udot + c_ins_bias_err_udot ; //white error, scale error and bias error
  m_ins_vdot = dist_ins_stdev_vdot(gen) + c_ins_scale_err*m_auv_vdot + c_ins_bias_err_vdot ; //white error, scale error and bias error
  m_ins_wdot = dist_ins_stdev_wdot(gen) + c_ins_scale_err*m_auv_wdot + c_ins_bias_err_wdot ; //white error, scale error and bias error
  MOOSTrace("INS accel: udot=%.4f, vdot=%.4f, wdot=%.4f, dt=%.3f \n",m_ins_udot,m_ins_vdot,m_ins_wdot,m_current_u_time-m_prev_u_time);
 
  // Integrating to get the unaided INS based position - not using this for anything -----------------------
  // Calculating the velocity
  double ins_calc_ydot;
  double ins_calc_xdot;
  double ins_calc_zdot;
  if (isnan(m_ins_udot)==0 && isnan(m_ins_vdot)==0 && isnan(m_ins_wdot)==0)
  {
    m_ins_calc_u = m_ins_calc_u + m_ins_udot * (m_current_u_time-m_prev_u_time);
    m_ins_calc_v = m_ins_calc_v + m_ins_vdot * (m_current_u_time-m_prev_u_time);
    m_ins_calc_w = m_ins_calc_w + m_ins_wdot * (m_current_u_time-m_prev_u_time);
    // Converting body-frame to earth-frame
    ins_calc_ydot = m_ins_calc_u*cos(m_current_nav_heading*M_PI/180)*cos(m_current_nav_pitch*M_PI/180)+m_ins_calc_v*(cos(m_current_nav_heading*M_PI/180)*sin(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_roll*M_PI/180) - sin(m_current_nav_heading*M_PI/180)*cos(m_current_nav_roll*M_PI/180))+m_ins_calc_w*(sin(m_current_nav_heading*M_PI/180)*sin(m_current_nav_roll*M_PI/180)+cos(m_current_nav_heading*M_PI/180)*cos(m_current_nav_roll*M_PI/180)*sin(m_current_nav_pitch*M_PI/180));
    ins_calc_xdot = m_ins_calc_u*sin(m_current_nav_heading*M_PI/180)*cos(m_current_nav_pitch*M_PI/180)+m_ins_calc_v*(cos(m_current_nav_heading*M_PI/180)*cos(m_current_nav_roll*M_PI/180)+sin(m_current_nav_roll*M_PI/180)*sin(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_heading*M_PI/180))+m_ins_calc_w*(sin(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_heading*M_PI/180)*cos(m_current_nav_roll*M_PI/180)-cos(m_current_nav_heading*M_PI/180)*sin(m_current_nav_roll*M_PI/180));
    ins_calc_zdot = m_ins_calc_u*-1*sin(m_current_nav_pitch*M_PI/180) + m_ins_calc_v*cos(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_roll*M_PI/180) + m_ins_calc_w*cos(m_current_nav_pitch*M_PI/180)*cos(m_current_nav_roll*M_PI/180);
  }
  // calculating the position
  if (isnan(ins_calc_ydot)==0 && isnan(ins_calc_xdot)==0)
  {
    if (m_ins_update_no <50)
    {
      m_ins_calc_x=m_current_nav_x;
      m_ins_calc_y=m_current_nav_y;
    }
    if (abs(m_current_u_time-m_prev_u_time) < 5)
    {
      m_ins_calc_x = m_ins_calc_x + ins_calc_xdot * (m_current_u_time-m_prev_u_time);
      m_ins_calc_y = m_ins_calc_y + ins_calc_ydot * (m_current_u_time-m_prev_u_time);
    }
  }
  // Calculate error
  double ins_pos_error = sqrt(pow((m_ins_calc_x-m_current_nav_x),2)+pow((m_ins_calc_y-m_current_nav_y),2));
  double ins_u_error = abs(m_ins_calc_u - auv_u);
  double ins_v_error = abs(m_ins_calc_v - auv_v);
  double ins_w_error = abs(m_ins_calc_w - auv_w);
  // Publish  - not using this for anything
  m_Comms.Notify("HYDROMAN_ERROR_INS", (ins_pos_error));
  MOOSTrace("INS error: pos=%.4f, u=%.4f, v=%.4f, w=%.3f \n",ins_pos_error, ins_u_error, ins_v_error, ins_w_error);
  // --------------------------------------------------------------------------------------------------------

  // Publish Phins Data
  PublishPhinsDataMOOSVar();
    
  // Setting previous values for differentiation
  m_prev_auv_u = auv_u;
  m_prev_auv_v = auv_v;
  m_prev_auv_w = auv_w;
  m_prev_u_time = m_current_u_time;
  m_current_u_received == false;
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool SimPhinsINS::OnStartUp()
{
  list<string> sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(m_MissionReader.GetConfiguration(GetAppName(), sParams)) {
    list<string>::iterator p;
    for(p=sParams.begin(); p!=sParams.end(); p++) {
      string original_line = *p;
      string param = stripBlankEnds(toupper(biteString(*p, '=')));
      string value = stripBlankEnds(*p);
      
      if(param == "FOO") {
        //handled
      }
      else if(param == "BAR") {
        //handled
      }
    }
  }
  string sVal;
  if(m_MissionReader.GetConfigurationParam("ins_bias_udot",sVal))
  {
    c_ins_bias_err_udot = atof(sVal.c_str());
    MOOSTrace("INS accel bias error udot: %.4f \n",c_ins_bias_err_udot);
  }
  if(m_MissionReader.GetConfigurationParam("ins_bias_vdot",sVal))
  {
    c_ins_bias_err_vdot = atof(sVal.c_str());
    MOOSTrace("INS accel bias error vdot: %.4f \n",c_ins_bias_err_vdot);
  }
  if(m_MissionReader.GetConfigurationParam("ins_bias_wdot",sVal))
  {
    c_ins_bias_err_wdot = atof(sVal.c_str());
    MOOSTrace("INS accel bias error wdot: %.4f \n",c_ins_bias_err_wdot);
  }
  if(m_MissionReader.GetConfigurationParam("ins_scale_err",sVal))
  {
    c_ins_scale_err = atof(sVal.c_str());
    MOOSTrace("INS accel scale error: %.4f \n",c_ins_scale_err);
  }
  if(m_MissionReader.GetConfigurationParam("ins_stdev",sVal))
  {
    c_ins_stdev = atof(sVal.c_str());
    MOOSTrace("INS accel random walk (Stdev): %.4f \n",c_ins_stdev);
  }

  // To send INS data via UDP using std bin protocol - is not implemented yet
  if(m_MissionReader.GetConfigurationParam("phins_ip",sVal))
  {
    c_phins_ins_ip = sVal;
    std::cout << "IP address: " << c_phins_ins_ip << std::endl;
  }
  if(m_MissionReader.GetConfigurationParam("phins_port",sVal))
  {
    c_phins_ins_port = sVal;
    std::cout << "Port: " << c_phins_ins_port << std::endl;
  } 
  RegisterVariables();	
  m_udp_client = new UDPClient(m_io, c_phins_ins_ip.c_str(), c_phins_ins_port.c_str());
  return(true);
}

//---------------------------------------------------------
// Procedure: RegisterVariables

void SimPhinsINS::RegisterVariables()
{
  // Register("FOOBAR", 0);
  Register("FRONTSEAT_DEPTH", 0);
  Register("FRONTSEAT_ROLL", 0);
  Register("FRONTSEAT_PITCH", 0);
  Register("FRONTSEAT_HEADING", 0);
  Register("FRONTSEAT_BODY_VEL_Y", 0);
  Register("FRONTSEAT_BODY_VEL_X", 0);
  Register("FRONTSEAT_BODY_VEL_Z", 0);
  Register("CURRENT_X", 0);
  Register("CURRENT_Y", 0); 
  Register("FRONTSEAT_X", 0); 
  Register("FRONTSEAT_Y", 0);
}

void SimPhinsINS::PublishPhinsDataMOOSVar()
{
  lamss::hydroman::protobuf::IXBlueINSData phins_data;
  // Set headers - Just setting values to fill the placeholder (EXCEPT FOR 'navigation_data_validity_time_100us')
  phins_data.mutable_header()->set_protocol_version(3);
  phins_data.mutable_header()->set_navigation_bit_mask(2145648639);
  phins_data.mutable_header()->set_extended_navigation_bit_mask(3);
  phins_data.mutable_header()->set_telegram_size(390);
  phins_data.mutable_header()->set_navigation_data_validity_time_100us(299149998);  // TO-DO e.g. 299149998
  // Setting attitude data
  phins_data.mutable_attitude_deg()->set_roll(m_current_nav_roll);
  phins_data.mutable_attitude_deg()->set_pitch(m_current_nav_pitch);
  phins_data.mutable_attitude_deg()->set_heading(m_current_nav_heading);
  // Setting linear acceleration vessel frame data
  phins_data.mutable_acce_vessel_frame_msec2()->set_xv1(m_ins_udot);
  phins_data.mutable_acce_vessel_frame_msec2()->set_xv2(-1*m_ins_vdot); // xv2 is positive port
  phins_data.mutable_acce_vessel_frame_msec2()->set_xv3(-1*m_ins_wdot); // xv3 is positive up
  // Setting linear acceleration vessel frame stdev data
  phins_data.mutable_acce_vessel_frame_stdev()->set_xv1(c_ins_stdev);
  phins_data.mutable_acce_vessel_frame_stdev()->set_xv2(c_ins_stdev);
  phins_data.mutable_acce_vessel_frame_stdev()->set_xv3(c_ins_stdev);
  // Setting speed vessel frame data
  phins_data.mutable_speed_vessel_frame_msec()->set_xv1(m_ins_calc_u);
  phins_data.mutable_speed_vessel_frame_msec()->set_xv2(-1*m_ins_calc_v); // xv2 is positive port
  phins_data.mutable_speed_vessel_frame_msec()->set_xv3(-1*m_ins_calc_w); // xv3 is positive up

  std::string serialized;
  serialize_for_moos(&serialized,phins_data);
  Notify("FRONTSEAT_PHINS_DATA", serialized);
}