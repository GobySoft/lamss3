/************************************************************/
/*    NAME: Supun Randeni                                              */
/*    ORGN: MIT                                             */
/*    FILE: SimDVL.cpp                                        */
/*    DATE:                                                 */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "SimDVL.h"
#include <random>

using namespace std;
std::default_random_engine gen;
//---------------------------------------------------------
// Constructor

SimDVL::SimDVL()
{
    m_previous_nav_depth_time=MOOSTime();
    m_previous_nav_x_time=MOOSTime();
    m_previous_nav_y_time=MOOSTime();
    m_icecamp_xdot=0;
    m_icecamp_ydot=0;
    m_icecamp_speed=0;
}

//---------------------------------------------------------
// Destructor

SimDVL::~SimDVL()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool SimDVL::OnNewMail(MOOSMSG_LIST &NewMail)
{
    MOOSMSG_LIST::iterator p;
    
    for(p=NewMail.begin(); p!=NewMail.end(); p++) {
        CMOOSMsg &msg = *p;
        string key   = msg.GetKey();
        double dval  = msg.GetDouble();
        string sval  = msg.GetString();
        
#if 0 // Keep these around just for template
        string comm  = msg.GetCommunity();
        string msrc  = msg.GetSource();
        double mtime = msg.GetTime();
        bool   mdbl  = msg.IsDouble();
        bool   mstr  = msg.IsString();
#endif
        if (key == "FRONTSEAT_X") {
            m_current_nav_x = dval;
            m_current_nav_x_time = MOOSTime();
        }
        else if(key == "FRONTSEAT_Y") {
            m_current_nav_y = dval;
            m_current_nav_y_time = MOOSTime();
        }
        else if(key == "FRONTSEAT_DEPTH") {
            m_current_nav_depth = dval;
            m_current_nav_depth_time = MOOSTime();
        }
        else if(key == "FRONTSEAT_ROLL") {
            m_current_nav_roll = dval;
        }
        else if(key == "FRONTSEAT_PITCH") {
            m_current_nav_pitch = dval;
        }
        else if(key == "FRONTSEAT_HEADING") {
            m_current_nav_heading = dval;
        }
        else if(key == "NODE_REPORT") {        
            if(c_ice_tracking_mode==1)
            {
            	MOOSValFromString(node_name,sval,"NAME");
            	if(node_name=="camp_sargo")
            	{
            		MOOSValFromString(m_icecamp_speed,sval,"SPD");
            		MOOSValFromString(m_icecamp_heading,sval,"HDG");
            	}
            }
               
        }
    }
    
    return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer

bool SimDVL::OnConnectToServer()
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

bool SimDVL::Iterate()
{
    // Calculating vel of AUV relative to earth in X/Y frame (i.e. xdot, ydot, zdot)
    m_current_nav_xdot = (m_current_nav_x - m_previous_nav_x)/(m_current_nav_x_time-m_previous_nav_x_time);
    m_current_nav_ydot = (m_current_nav_y - m_previous_nav_y)/(m_current_nav_y_time-m_previous_nav_y_time);
    m_current_nav_zdot = (m_current_nav_depth - m_previous_nav_depth)/(m_current_nav_depth_time-m_previous_nav_depth_time);
    // ice-camp drift velocity
    m_icecamp_ydot = m_icecamp_speed*cos(m_icecamp_heading*M_PI/180);
    m_icecamp_xdot = m_icecamp_speed*sin(m_icecamp_heading*M_PI/180);
    // Calculating vel of AUV relative to surface ice in X/Y frame (i.e. xdot, ydot, zdot)
    m_current_nav_xdot = m_current_nav_xdot - m_icecamp_xdot;
    m_current_nav_ydot = m_current_nav_ydot - m_icecamp_ydot;
    // Converting to body frame (i.e. to u,v,w)
    m_current_u = m_current_nav_ydot*cos(m_current_nav_pitch*M_PI/180)*cos(m_current_nav_heading*M_PI/180) + m_current_nav_xdot*cos(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_heading*M_PI/180) - m_current_nav_zdot*sin(m_current_nav_pitch*M_PI/180);
    m_current_v = m_current_nav_ydot*(sin(m_current_nav_roll*M_PI/180)*sin(m_current_nav_pitch*M_PI/180)*cos(m_current_nav_heading*M_PI/180) - cos(m_current_nav_roll*M_PI/180)*sin(m_current_nav_heading*M_PI/180)) + m_current_nav_xdot*(sin(m_current_nav_roll*M_PI/180)*sin(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_heading*M_PI/180)+cos(m_current_nav_roll*M_PI/180)*cos(m_current_nav_heading*M_PI/180)) + m_current_nav_zdot*sin(m_current_nav_roll*M_PI/180)*cos(m_current_nav_pitch*M_PI/180);
    m_current_w = m_current_nav_ydot*(cos(m_current_nav_roll*M_PI/180)*sin(m_current_nav_pitch*M_PI/180)*cos(m_current_nav_heading*M_PI/180)+sin(m_current_nav_roll*M_PI/180)*sin(m_current_nav_heading*M_PI/180)) + m_current_nav_xdot*(cos(m_current_nav_roll*M_PI/180)*sin(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_heading*M_PI/180)-sin(m_current_nav_roll*M_PI/180)*cos(m_current_nav_heading*M_PI/180)) + m_current_nav_zdot * cos(m_current_nav_roll*M_PI/180)*cos(m_current_nav_pitch*M_PI/180);
    // adding DVL uncertainties
    std::normal_distribution<double> dist_dvl_u(m_current_u, c_dvl_velocity_std);
    std::normal_distribution<double> dist_dvl_v(m_current_v, c_dvl_velocity_std);
    std::normal_distribution<double> dist_dvl_w(m_current_w, c_dvl_velocity_std);
    
    double m_dvl_u = dist_dvl_u(gen) + (c_dvl_velocity_bias); //white error and bias error
    double m_dvl_v = dist_dvl_v(gen) + (c_dvl_velocity_bias);
    double m_dvl_w = dist_dvl_w(gen) + (c_dvl_velocity_bias);
    
    if (m_current_nav_x_time>m_dvl_data_post_time)
    {
        Notify("HYDROMAN_SIM_DVL_U", m_dvl_u);
        Notify("HYDROMAN_SIM_DVL_V", m_dvl_v);
        Notify("HYDROMAN_SIM_DVL_W", m_dvl_w);
        m_dvl_data_post_time = m_current_nav_x_time + c_dvl_update_frequency;
    }
    
    //
    m_previous_nav_x = m_current_nav_x;
    m_previous_nav_y = m_current_nav_y;
    m_previous_nav_depth = m_current_nav_depth;
    m_previous_nav_xdot = m_current_nav_xdot;
    m_previous_nav_ydot = m_current_nav_ydot;
    m_previous_nav_zdot = m_current_nav_zdot;
    m_previous_nav_x_time = m_current_nav_x_time;
    m_previous_nav_y_time = m_current_nav_y_time;
    m_previous_nav_depth_time = m_current_nav_depth_time;
    return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool SimDVL::OnStartUp()
{
    list<string> sParams;
    m_MissionReader.EnableVerbatimQuoting(false);
    if(m_MissionReader.GetConfiguration(GetAppName(), sParams)) {
        list<string>::iterator p;
        for(p=sParams.begin(); p!=sParams.end(); p++) {
            string line  = *p;
            string param = tolower(biteStringX(line, '='));
            string value = line;
            
            if(param == "foo") {
                //handled
            }
            else if(param == "bar") {
                //handled
            }
        }
    }
    
    string sVal;
    
    if(m_MissionReader.GetConfigurationParam("dvl_update_frequency",sVal))
    {
        c_dvl_update_frequency = atof(sVal.c_str());
    }
    if(m_MissionReader.GetConfigurationParam("dvl_std",sVal))
    {
        c_dvl_velocity_std = atof(sVal.c_str());
    }
    if(m_MissionReader.GetConfigurationParam("dvl_bias_err",sVal))
    {
        c_dvl_velocity_bias = atof(sVal.c_str());
    }
    if(m_MissionReader.GetConfigurationParam("ice_tracking_mode",sVal))
    {
        c_ice_tracking_mode = atof(sVal.c_str());
    }
    m_dvl_data_post_time = m_current_nav_x_time;
    RegisterVariables();
    return(true);
}

//---------------------------------------------------------
// Procedure: RegisterVariables

void SimDVL::RegisterVariables()
{
    // Register("FOOBAR", 0);
    Register("FRONTSEAT_X", 0);
    Register("FRONTSEAT_Y", 0);
    Register("FRONTSEAT_DEPTH", 0);
    Register("FRONTSEAT_ROLL", 0);
    Register("FRONTSEAT_PITCH", 0);
    Register("FRONTSEAT_HEADING", 0);
    Register("NODE_REPORT", 0);
    
    
}

