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
    m_current_u_time=MOOSTime();
    m_previous_u_time=MOOSTime();
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
        if (key == "FRONTSEAT_BODY_VEL_Y") {
            m_current_u = dval;             // MOOS convension Y is surge
            m_current_u_time = MOOSTime();
        }
        else if (key == "FRONTSEAT_BODY_VEL_X") {
            m_current_v = dval;             // MOOS convension X is sway
        }
        else if (key == "FRONTSEAT_BODY_VEL_Z") {
            m_current_w = -dval;             // MOOS convension Z is up
        }
        else if (key == "CURRENT_X") {
            m_current_water_vel_x = dval;             // east
        }
        else if (key == "CURRENT_Y") {
            m_current_water_vel_y = dval;             // north
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
        else if(key == "NODE_REPORT") {        
            if(c_ice_tracking_mode==1)
            {
            	MOOSValFromString(node_name,sval,"NAME");
            	if(node_name==c_ice_sheet_name)
            	{
            		MOOSValFromString(m_icecamp_speed,sval,"SPD");
            		MOOSValFromString(m_icecamp_heading,sval,"HDG");
                    // MOOSTrace("Ice sheet drift: speed %.2f, heading %.1f \n",m_icecamp_speed,m_icecamp_heading);
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
    // Converting ground relative AUV velocity from body frame to earth frame
    double m_current_ydot = m_current_u*cos(m_current_nav_heading*M_PI/180)*cos(m_current_nav_pitch*M_PI/180)+m_current_v*(cos(m_current_nav_heading*M_PI/180)*sin(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_roll*M_PI/180) - sin(m_current_nav_heading*M_PI/180)*cos(m_current_nav_roll*M_PI/180))+m_current_w*(sin(m_current_nav_heading*M_PI/180)*sin(m_current_nav_roll*M_PI/180)+cos(m_current_nav_heading*M_PI/180)*cos(m_current_nav_roll*M_PI/180)*sin(m_current_nav_pitch*M_PI/180));
    double m_current_xdot = m_current_u*sin(m_current_nav_heading*M_PI/180)*cos(m_current_nav_pitch*M_PI/180)+m_current_v*(cos(m_current_nav_heading*M_PI/180)*cos(m_current_nav_roll*M_PI/180)+sin(m_current_nav_roll*M_PI/180)*sin(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_heading*M_PI/180))+m_current_w*(sin(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_heading*M_PI/180)*cos(m_current_nav_roll*M_PI/180)-cos(m_current_nav_heading*M_PI/180)*sin(m_current_nav_roll*M_PI/180));
    double m_current_zdot = m_current_u*-1*sin(m_current_nav_pitch*M_PI/180) + m_current_v*cos(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_roll*M_PI/180) + m_current_w*cos(m_current_nav_pitch*M_PI/180)*cos(m_current_nav_roll*M_PI/180);
    MOOSTrace("AUV vel relative to water: xdot=%.2f, ydot=%.2f, zdot=%.2f \n",m_current_xdot,m_current_ydot,m_current_zdot);
    // Converting the ice-camp drift velocity to earth frame
    m_icecamp_ydot = m_icecamp_speed*cos(m_icecamp_heading*M_PI/180);
    m_icecamp_xdot = m_icecamp_speed*sin(m_icecamp_heading*M_PI/180);
    double m_icecamp_zdot = 0.0;
    // If DVL has bottom-lock, then include water current velocity as well
    if (c_dvl_bottom_lock==1)
    {
        // Calculating vel of the AUV relative to surface ice in X/Y frame (i.e. xdot, ydot, zdot)
        // Adding water velocity since DVL has bottom lock
        auvVelRelIce_xdot = m_current_xdot - m_icecamp_xdot + m_current_water_vel_x;
        auvVelRelIce_ydot = m_current_ydot - m_icecamp_ydot + m_current_water_vel_y;
        auvVelRelIce_zdot = m_current_zdot - m_icecamp_zdot;
    }
    if (c_dvl_bottom_lock==0)
    {
        // Calculating vel of the AUV relative to surface ice in X/Y frame (i.e. xdot, ydot, zdot)
        auvVelRelIce_xdot = m_current_xdot - m_icecamp_xdot;
        auvVelRelIce_ydot = m_current_ydot - m_icecamp_ydot;
        auvVelRelIce_zdot = m_current_zdot - m_icecamp_zdot;
    }
    // Converting ice relative AUV velocity from earth-frame to body frame
    double auvVelRelIce_u = auvVelRelIce_ydot*cos(m_current_nav_pitch*M_PI/180)*cos(m_current_nav_heading*M_PI/180) + auvVelRelIce_xdot*cos(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_heading*M_PI/180) - auvVelRelIce_zdot*sin(m_current_nav_pitch*M_PI/180);
    double auvVelRelIce_v = auvVelRelIce_ydot*(sin(m_current_nav_roll*M_PI/180)*sin(m_current_nav_pitch*M_PI/180)*cos(m_current_nav_heading*M_PI/180) - cos(m_current_nav_roll*M_PI/180)*sin(m_current_nav_heading*M_PI/180)) + auvVelRelIce_xdot*(sin(m_current_nav_roll*M_PI/180)*sin(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_heading*M_PI/180)+cos(m_current_nav_roll*M_PI/180)*cos(m_current_nav_heading*M_PI/180)) + auvVelRelIce_zdot*sin(m_current_nav_roll*M_PI/180)*cos(m_current_nav_pitch*M_PI/180);
    double auvVelRelIce_w = auvVelRelIce_ydot*(cos(m_current_nav_roll*M_PI/180)*sin(m_current_nav_pitch*M_PI/180)*cos(m_current_nav_heading*M_PI/180)+sin(m_current_nav_roll*M_PI/180)*sin(m_current_nav_heading*M_PI/180)) + auvVelRelIce_xdot*(cos(m_current_nav_roll*M_PI/180)*sin(m_current_nav_pitch*M_PI/180)*sin(m_current_nav_heading*M_PI/180)-sin(m_current_nav_roll*M_PI/180)*cos(m_current_nav_heading*M_PI/180)) + auvVelRelIce_zdot * cos(m_current_nav_roll*M_PI/180)*cos(m_current_nav_pitch*M_PI/180);
    // adding DVL uncertainties
    std::normal_distribution<double> dist_dvl_u(auvVelRelIce_u, c_dvl_velocity_std);
    std::normal_distribution<double> dist_dvl_v(auvVelRelIce_v, c_dvl_velocity_std);
    std::normal_distribution<double> dist_dvl_w(auvVelRelIce_w, c_dvl_velocity_std);
    double m_dvl_u = dist_dvl_u(gen) + (c_dvl_velocity_bias); //white error and bias error
    double m_dvl_v = dist_dvl_v(gen) + (c_dvl_velocity_bias);
    double m_dvl_w = dist_dvl_w(gen) + (c_dvl_velocity_bias);

    // DVL orientation change (i.e. when switching from downward to upward looking)

    // Posting the DVL measurements acording to the configured DVL update frequency
    if (m_current_u_time>m_dvl_data_post_time)
    {
        Notify("HYDROMAN_SIM_DVL_U", m_dvl_u);
        Notify("HYDROMAN_SIM_DVL_V", m_dvl_v);
        Notify("HYDROMAN_SIM_DVL_W", m_dvl_w);
        // MOOSTrace("DVL measurement: u=%.2f, v=%.2f, w=%.2f \n",m_dvl_u,m_dvl_v,m_dvl_w);
        m_dvl_data_post_time = m_current_u_time + c_dvl_update_frequency;
    }
    
    m_previous_nav_depth = m_current_nav_depth;
    m_previous_u_time = m_current_u_time;
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
        MOOSTrace("DVL update frequency: %.1f \n",c_dvl_update_frequency);
    }
    if(m_MissionReader.GetConfigurationParam("dvl_std",sVal))
    {
        c_dvl_velocity_std = atof(sVal.c_str());
        MOOSTrace("DVL velocity stdev: %.4f \n",c_dvl_velocity_std);
    }
    if(m_MissionReader.GetConfigurationParam("dvl_bias_err",sVal))
    {
        c_dvl_velocity_bias = atof(sVal.c_str());
        MOOSTrace("DVL velocity bias error: %.4f \n",c_dvl_velocity_bias);
    }
    if(m_MissionReader.GetConfigurationParam("ice_tracking_mode",sVal))
    {
        c_ice_tracking_mode = atof(sVal.c_str());
        MOOSTrace("DVL ice tracking: %.1f \n",c_ice_tracking_mode);
    }
    if(m_MissionReader.GetConfigurationParam("ice_sheet_name",sVal))
    {
        c_ice_sheet_name = sVal;
        MOOSTrace("Ice sheet name: %s \n",c_ice_sheet_name);
    }
    if(m_MissionReader.GetConfigurationParam("dvl_bottom_lock",sVal))
    {
        c_dvl_bottom_lock = atof(sVal.c_str());
        MOOSTrace("DVL bottom lock: %i \n",c_dvl_bottom_lock);
    }
    m_dvl_data_post_time = m_current_u_time;
    RegisterVariables();
    return(true);
}

//---------------------------------------------------------
// Procedure: RegisterVariables

void SimDVL::RegisterVariables()
{
    // Register("FOOBAR", 0);
    Register("FRONTSEAT_DEPTH", 0);
    Register("FRONTSEAT_ROLL", 0);
    Register("FRONTSEAT_PITCH", 0);
    Register("FRONTSEAT_HEADING", 0);
    Register("NODE_REPORT", 0);
    Register("FRONTSEAT_BODY_VEL_Y", 0);
    Register("FRONTSEAT_BODY_VEL_X", 0);
    Register("FRONTSEAT_BODY_VEL_Z", 0);
    Register("CURRENT_X", 0);
    Register("CURRENT_Y", 0); 
}

