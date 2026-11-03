/************************************************************/
/*    NAME: Nick Rypkema                                    */
/*    ORGN: MIT                                             */
/*    FILE: TetraHoming.cpp                                 */
/*    DATE: April 2017                                      */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "AngleUtils.h"
#include "TetraHoming.h"
#include <math.h>

using namespace std;

//---------------------------------------------------------
// Constructor

TetraHoming::TetraHoming() : m_valid_std_azimuth(7*M_PI/180), m_valid_std_range(10), m_loiter_x(0), m_loiter_y(0)
{
  m_target_x = m_loiter_x;
  m_target_y = m_loiter_y;
  m_std_azimuth = M_PI;
  m_std_range = 500;
  m_deploy_mission = "OTHER";
  m_mission_state = "OTHER";
  m_nav_x = 0;
  m_nav_y = 0;
  m_nav_speed = 1;
  m_lock_time = 0;
  m_stale_lock = 120;
  m_iterations = 0;
  m_timewarp   = 1;
}

//---------------------------------------------------------
// Destructor

TetraHoming::~TetraHoming()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool TetraHoming::OnNewMail(MOOSMSG_LIST &NewMail)
{
  MOOSMSG_LIST::iterator p;

  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;

    if (MOOSStrCmp(msg.GetKey(), "NAV_SPEED")) {
        m_nav_speed = msg.GetDouble();
     }

     else if (MOOSStrCmp(msg.GetKey(), "NAV_X")) {
        m_nav_x = msg.GetDouble();
     }

     else if (MOOSStrCmp(msg.GetKey(), "NAV_Y")) {
        m_nav_y = msg.GetDouble();
     }

     else if (MOOSStrCmp(msg.GetKey(), "DEPLOY_MISSION")) {
        m_deploy_mission = msg.GetString();
     }

     else if (MOOSStrCmp(msg.GetKey(), "MISSION_STATE")) {
        m_mission_state = msg.GetString();
     }

     else if (MOOSStrCmp(msg.GetKey(), "S-MOCAB_TGT_X")) {
        m_target_x = msg.GetDouble() + m_nav_x;
     }

     else if (MOOSStrCmp(msg.GetKey(), "S-MOCAB_TGT_Y")) {
        m_target_y = msg.GetDouble() + m_nav_y;
     }

     else if (MOOSStrCmp(msg.GetKey(), "S-MOCAB_STD_AZIMUTH")) {
        m_std_azimuth = msg.GetDouble();
     }

     else if (MOOSStrCmp(msg.GetKey(), "S-MOCAB_STD_RANGE")) {
        m_std_range = msg.GetDouble();
     }
     else if (MOOSStrCmp(msg.GetKey(), "BEACON_UPDATE")) {
        string update = msg.GetString();
        string beacon_x, beacon_y;
        beacon_x = tokStringParse(update, "beacon_x", ',', '=');
        beacon_y = tokStringParse(update, "beacon_y", ',', '=');
        if (!beacon_x.empty()) m_aco_beacon_x = atof(beacon_x.c_str());
        if (!beacon_y.empty()) m_aco_beacon_y = atof(beacon_y.c_str());
     }


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
   }

   return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer

bool TetraHoming::OnConnectToServer()
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

bool TetraHoming::Iterate()
{
  if ((m_mission_state == "DEPLOY") &&
      (m_deploy_mission == "RETURN" || m_deploy_mission == "RECOVERY"))
    {
      cout << "azimuth std. dev. = " << m_std_azimuth*180/M_PI << " must be <= " << m_valid_std_azimuth*180/M_PI << endl;
      cout << "range std. dev. = " << m_std_range << " must be <= " << m_valid_std_range << endl;
      if ((m_std_azimuth <= m_valid_std_azimuth) && (m_std_range <= m_valid_std_range))
	{
	  // Good homing estimate
	  m_loiter_x = m_target_x;
	  m_loiter_y = m_target_y;
	  Notify("HOMING_LOCK","TRUE");
	  m_lock_time = MOOSTime();
	}
      else if (MOOSTime()-m_lock_time > m_stale_lock)
	{
	  // else  if stale use best current estimate from pLamssMissionManager
	  m_loiter_x = m_aco_beacon_x;
	  m_loiter_y = m_aco_beacon_y;
	  Notify("HOMING_LOCK","FALSE");
	}
      stringstream ss;
      ss << "center_assign = " << m_loiter_x << "," << m_loiter_y;
      Notify(m_return_update_var, ss.str());
      Notify("HOMING_X",m_loiter_x);
      Notify("HOMING_Y",m_loiter_y);
      std::stringstream wpt;
      wpt << "point="
	  << m_loiter_x
	  << ","
	  << m_loiter_y;
      Notify("HOMING_STATION",wpt.str());
    }
  else if (m_mission_state == "ABORT") {
    cout << "azimuth std. dev. = " << m_std_azimuth*180/M_PI << " must be <= " << m_valid_std_azimuth*180/M_PI << endl;
    cout << "range std. dev. = " << m_std_range << " must be <= " << m_valid_std_range << endl;
    if ((m_std_azimuth <= m_valid_std_azimuth) && (m_std_range <= m_valid_std_range))
      {
	double heading = radToHeading(atan2(m_target_y - m_nav_y, m_target_x - m_nav_x));
	double range = sqrt(pow(m_target_x - m_nav_x, 2) + pow(m_target_y - m_nav_y, 2));
	double duration = range/m_nav_speed;
	stringstream ss;
	ss << "heading=" << heading << "#duration=" << duration;
	Notify(m_abort_update_var, ss.str());
	// Good homing estimate
	Notify("HOMING_LOCK","TRUE");
	m_lock_time = MOOSTime();
      }
    else if (MOOSTime()-m_lock_time > m_stale_lock)
      {
	Notify("HOMING_LOCK","FALSE");
      }
  }

  m_iterations++;
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool TetraHoming::OnStartUp()
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

  if (!m_MissionReader.GetConfigurationParam("VALID_AZIMUTH_STDDEV", m_valid_std_azimuth)) {
    cerr << "No VALID_AZIMUTH_STDDEV specified! Using default value, 7.0 degrees" << endl;
  } else {
    m_valid_std_azimuth = m_valid_std_azimuth*M_PI/180;
  }

  if (!m_MissionReader.GetConfigurationParam("VALID_RANGE_STDDEV", m_valid_std_range)) {
    cerr << "No VALID_RANGE_STDDEV specified! Using default value, 10.0 meters" << endl;
  }

  if (!m_MissionReader.GetConfigurationParam("LOITER_X", m_loiter_x)) {
    cerr << "No default LOITER_X specified! Using default value, 0.0" << endl;
  }
  m_target_x = m_loiter_x;

  if (!m_MissionReader.GetConfigurationParam("LOITER_Y", m_loiter_y)) {
    cerr << "No default LOITER_Y specified! Using default value, 0.0" << endl;
  }
  m_target_y = m_loiter_y;

  if (!m_MissionReader.GetConfigurationParam("RETURN_UPDATE_VAR", m_return_update_var)) {
    cerr << "No default RETURN_UPDATE_VAR specified! Using default value, 'HOMING_CONFIG'" << endl;
    m_return_update_var = "HOMING_CONFIG";
  }

  if (!m_MissionReader.GetConfigurationParam("ABORT_UPDATE_VAR", m_abort_update_var)) {
    cerr << "No default ABORT_UPDATE_VAR specified! Using default value, 'ABORT_HEADING'" << endl;
    m_abort_update_var = "ABORT_HEADING";
  }

  m_timewarp = GetMOOSTimeWarp();

  RegisterVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: RegisterVariables

void TetraHoming::RegisterVariables()
{
  // Register("FOOBAR", 0);
  m_Comms.Register("NAV_SPEED");
  m_Comms.Register("NAV_X");
  m_Comms.Register("NAV_Y");
  m_Comms.Register("DEPLOY_MISSION");
  m_Comms.Register("MISSION_STATE");
  m_Comms.Register("S-MOCAB_TGT_X");
  m_Comms.Register("S-MOCAB_TGT_Y");
  m_Comms.Register("S-MOCAB_STD_AZIMUTH");
  m_Comms.Register("S-MOCAB_STD_RANGE");
  m_Comms.Register("BEACON_UPDATE");
}

