/************************************************************/
/*    NAME: Nick Rypkema                                    */
/*    ORGN: MIT                                             */
/*    FILE: SimAcoBeaconRngBrg.cpp                          */
/*    DATE: April 2017                                      */
/************************************************************/

#include <iterator>
#include "SimAcoBeaconRngBrg.h"
#include "GeomUtils.h"  //needed for projectPoint (heading-range to x-y)
#include "MBUtils.h"
#include "AngleUtils.h"

#include "goby/moos/moos_protobuf_helpers.h"

using namespace std;

//---------------------------------------------------------
// Constructor

SimAcoBeaconRngBrg::SimAcoBeaconRngBrg() : m_sensor_range(1500.0), m_sensor_swath(180.0), m_sensor_range_stddev(0.5),
                                           m_sensor_swath_stddev(4.0), m_random_walk_stddev(1.0), m_sensor_probability(0.95),
                                           m_sensor_period(1.0), m_num_particles(250)
{
  m_nav_x = 0.0;
  m_nav_y = 0.0;
  m_nav_heading = 0.0;
  m_target_found = false;
  m_nav_set = false;
  m_prev_time = -1;
  m_dt_accum = 0;
  m_usbl_enabled = false;
  m_iterations = 0;
  m_timewarp   = 1;
}

//---------------------------------------------------------
// Destructor

SimAcoBeaconRngBrg::~SimAcoBeaconRngBrg()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool SimAcoBeaconRngBrg::OnNewMail(MOOSMSG_LIST &NewMail)
{
  MOOSMSG_LIST::iterator p;

  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;

    if (MOOSStrCmp(msg.GetKey(), "IFS_DATA_IN"))
      {
	handle_ifs_data(msg);
      }
    /*
    if (MOOSStrCmp(msg.GetKey(), "NAV_HEADING")) {
        m_nav_heading = msg.GetDouble();
     }

     else if (MOOSStrCmp(msg.GetKey(), "NAV_X")) {
        m_nav_x = msg.GetDouble();
        if (!m_nav_set) {
          m_nav_set = true;
        }
     }

     else if (MOOSStrCmp(msg.GetKey(), "NAV_Y")) {
        m_nav_y = msg.GetDouble();
        if (!m_nav_set) {
          m_nav_set = true;
        }
     }
    */
    else if (MOOSStrCmp(msg.GetKey(), "USBL_ENABLED"))
       {
        string usbl = msg.GetString();
        if (usbl == "TRUE") m_usbl_enabled = true;
        else m_usbl_enabled = false;
       }

    else if (MOOSStrCmp(msg.GetKey(), m_update_var))
      {
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

bool SimAcoBeaconRngBrg::OnConnectToServer()
{
   // register for variables here
   // possibly look at the mission file?
   // m_MissionReader.GetConfigurationParam("Name", <string>);
   // m_Comms.Register("VARNAME", 0);

   RegisterVariables();
   return(true);
}

//---------------------------------------------------------
// Handle IFS Data
void SimAcoBeaconRngBrg::handle_ifs_data(const CMOOSMsg& msg)
{
    goby::middleware::frontseat::protobuf::InterfaceData data;
    parse_for_moos(msg.GetString(), &data);

    if(data.has_node_status())
    {
        last_nav_ = data.node_status();
        
        m_nav_x = last_nav_.local_fix().x();
        m_nav_y = last_nav_.local_fix().y();
        m_nav_heading = last_nav_.pose().heading();
        if (!m_nav_set) {
          m_nav_set = true;
        }
    }
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool SimAcoBeaconRngBrg::Iterate()
{
  if(m_prev_time == -1) m_prev_time = MOOSTime();
  double dt = MOOSTime() - m_prev_time;
  m_prev_time = MOOSTime();

  if (m_usbl_enabled) {
    m_dt_accum += dt;

    if (m_dt_accum >= m_sensor_period) {
      m_dt_accum = 0;
      bool status = CheckSensor();
      if (status) cout << "new measurement, time: " << m_prev_time << endl;
    }

    if (!m_target_found and m_nav_set) {
      m_target_x = 0;
      m_target_y = 0;
    }

    m_Comms.Notify("S-MOCAB_TGT_X", m_target_x - m_nav_x);
    m_Comms.Notify("S-MOCAB_TGT_Y", m_target_y - m_nav_y);
    m_Comms.Notify("S-MOCAB_STD_AZIMUTH", m_std_azimuth);
    m_Comms.Notify("S-MOCAB_STD_RANGE", m_std_range);

    stringstream viewpoint;
    for (unsigned int i = 0; i < m_num_particles; i++) {
      viewpoint.clear();
      viewpoint.str(std::string());
      viewpoint << "x=" << m_particles_x[i] << ",y=" << m_particles_y[i] << ",label=P" << i << ",vertex_size=5,vertex_color=red";
      m_Comms.Notify("VIEW_POINT", viewpoint.str());
    }
    viewpoint.clear();
    viewpoint.str(std::string());
    viewpoint << "x=" << m_target_x << ",y=" << m_target_y << ",label=T,vertex_size=15,vertex_color=white";
    m_Comms.Notify("VIEW_POINT", viewpoint.str());

    //draw arc of sensor -> note: use the utility function from XYFormatUtilsPoly.cpp to convert from string to polygon,
    //then set the remaining attributes of the polygon directly, and finally request the specs using get_spec()
    //note: we can only draw 'convex' pie wedges -> to draw 'concave' ones, we have to add two addition wedges to left and right aft (TODO)
    double half_swath = m_sensor_swath/2.0;
    stringstream m_sensor_swath_draw_ss;
    m_sensor_swath_draw_ss << "type=piewedge, ";
    m_sensor_swath_draw_ss << "x=" << m_nav_x << ", ";
    m_sensor_swath_draw_ss << "y=" << m_nav_y << ", ";
    m_sensor_swath_draw_ss << "lang=" << (m_nav_heading-half_swath) << ", ";
    m_sensor_swath_draw_ss << "rang=" << (m_nav_heading+half_swath) << ", ";
    m_sensor_swath_draw_ss << "range=" << m_sensor_range/10 << ", ";
    m_sensor_swath_draw_ss << "pts=5, ";
    m_sensor_swath_draw_ss << "label=sensor_swath";
    m_sensor_arc = stringPieWedge2Poly(m_sensor_swath_draw_ss.str());
    m_sensor_arc.set_color("edge", "white");
    m_sensor_arc.set_color("fill", "white");
    m_sensor_arc.set_msg("sensor_swath");
    m_sensor_arc.set_vertex_size(0);
    m_sensor_arc.set_edge_size(1);
    m_sensor_arc.set_transparency(0.15);
    Notify("VIEW_POLYGON", m_sensor_arc.get_spec());
  }

  m_iterations++;
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool SimAcoBeaconRngBrg::OnStartUp()
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

  if (!m_MissionReader.GetConfigurationParam("BEACON_X", m_aco_beacon_x)) {
    cerr << "No acoustic beacon x-pos specified! Using default x-pos, 0.0" << endl;
    m_aco_beacon_x = 0.0;
  }

  if (!m_MissionReader.GetConfigurationParam("BEACON_Y", m_aco_beacon_y)) {
    cerr << "No acoustic beacon y-pos specified! Using default y-pos, 0.0" << endl;
    m_aco_beacon_y = 0.0;
  }

  if (!m_MissionReader.GetConfigurationParam("SENSOR_RANGE", m_sensor_range)) {
    cerr << "No sensor range specified! Using default sensor range, 1500.0" << endl;
  }

  if (!m_MissionReader.GetConfigurationParam("SENSOR_SWATH", m_sensor_swath)) {
    cerr << "No sensor swath specified! Using default sensor swath, 180.0" << endl;
  }

  if (!m_MissionReader.GetConfigurationParam("SENSOR_RANGE_STDDEV", m_sensor_range_stddev)) {
    cerr << "No sensor range std. dev. specified! Using default sensor range std. dev., 0.5" << endl;
  }

  if (!m_MissionReader.GetConfigurationParam("SENSOR_SWATH_STDDEV", m_sensor_swath_stddev)) {
    cerr << "No sensor swath std. dev. specified! Using default sensor swath std. dev., 4.0" << endl;
  }

  if (!m_MissionReader.GetConfigurationParam("RANDOM_WALK_STDDEV", m_random_walk_stddev)) {
    cerr << "No random walk std. dev. specified! Using default random walk std. dev., 1.0" << endl;
  }

  if (!m_MissionReader.GetConfigurationParam("SENSOR_PROBABILITY", m_sensor_probability)) {
    cerr << "No sensor probability specified! Using default sensor probability, 0.95" << endl;
  }

  if (!m_MissionReader.GetConfigurationParam("SENSOR_PERIOD", m_sensor_period)) {
    cerr << "No sensor period specified! Using default sensor period, 1.0" << endl;
  }

  if (!m_MissionReader.GetConfigurationParam("NUM_PARTICLES", m_num_particles)) {
    cerr << "No number of particles specified! Using default number of particles, 250" << endl;
  }

  if (!m_MissionReader.GetConfigurationParam("UPDATES", m_update_var)) {
    m_update_var = "";
  }

  cout << "UPDATE VAR " << m_update_var << endl;

  m_particles_x.resize(m_num_particles);
  m_particles_y.resize(m_num_particles);
  m_particles_a.resize(m_num_particles);
  m_particles_r.resize(m_num_particles);
  for (unsigned int i = 0; i < m_num_particles; i++) {
    m_particles_x[i] = Randn()*500;
    m_particles_y[i] = Randn()*500;
  }

  m_timewarp = GetMOOSTimeWarp();

  RegisterVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: RegisterVariables

void SimAcoBeaconRngBrg::RegisterVariables()
{
  // Register("FOOBAR", 0);
  // HS 2 Feb. 2020: Changed to IFS for actual vehicle position with hydroman nav
  // in ICEX
  m_Comms.Register("IFS_DATA_IN");
  //  m_Comms.Register("NAV_HEADING");
  //  m_Comms.Register("NAV_X");
  //  m_Comms.Register("NAV_Y");
  m_Comms.Register("USBL_ENABLED");
  if (!m_update_var.empty()) {
    m_Comms.Register(m_update_var);
  }
}

double SimAcoBeaconRngBrg::Randn()
//use the first form of the Box-Muller transform to generate a random
//variable with normal distribution from one with a uniform distribution
{
   double u1_ = rand()/(double)RAND_MAX;  //two uniform random numbers [0,1]
   double u2_ = rand()/(double)RAND_MAX;
   double R_ = sqrt(-2.0*log(u1_));
   double theta_ = 2.0*PI*u2_;

   double z0_ = R_*cos(theta_);           //can use either of these as a normal RV
   //double z1_ = R_*sin(theta_);

   return(z0_);
}

bool SimAcoBeaconRngBrg::CheckSensor()
//check if sensor detects the acoustic beacon
{
  cout << m_aco_beacon_x << ", " << m_aco_beacon_y << endl;
  double dx = m_aco_beacon_x - m_nav_x;
  double dy = m_aco_beacon_y - m_nav_y;
  double lm_bearing = radToHeading(atan2(dy, dx));

  bool in_swath = (abs(dx) < m_sensor_range) && (abs(dy) < m_sensor_range) &&
                  (lm_bearing > (m_nav_heading-(m_sensor_swath/2))) &&
                  (lm_bearing < (m_nav_heading+(m_sensor_swath/2))) &&
                  ((pow(dx, 2.0) + pow(dy, 2.0)) < pow(m_sensor_range, 2.0));

  if (in_swath) {
    // get noisy acoustic beacon position particles
    double r = ((double) rand() / (RAND_MAX));
    if (r < m_sensor_probability) {
      for (unsigned int i = 0; i < m_num_particles; i++) {
        if (!m_target_found) m_target_found = true;
        double range = sqrt(pow(dx, 2.0) + pow(dy, 2.0)) + Randn()*m_sensor_range_stddev;
        double bearing = atan2(dy, dx) + Randn()*(m_sensor_swath_stddev*PI/180.0);
        m_particles_r[i] = range;
        m_particles_a[i] = bearing;
        m_particles_x[i] = range*cos(bearing) + m_nav_x;
        m_particles_y[i] = range*sin(bearing) + m_nav_y;
      }
      MeanParticles();
      StdDevParticles();
      return(true);
    }
  }
  // otherwise, random walk our acoustic beacon particles
  for (unsigned int i = 0; i < m_num_particles; i++) {
    m_particles_x[i] += Randn()*m_random_walk_stddev;
    m_particles_y[i] += Randn()*m_random_walk_stddev;
    double bearing = atan2(m_particles_y[i] - m_nav_y, m_particles_x[i] - m_nav_x);
    double range = sqrt(pow(m_particles_x[i] - m_nav_x, 2.0) + pow(m_particles_y[i] - m_nav_y, 2.0));
    m_particles_r[i] = range;
    m_particles_a[i] = bearing;
  }
  MeanParticles();
  StdDevParticles();
  return(false);
}

void SimAcoBeaconRngBrg::MeanParticles()
{
  m_target_x = 0;
  m_target_y = 0;
  for (unsigned int i = 0; i < m_num_particles; i++) {
    m_target_x += m_particles_x[i];
    m_target_y += m_particles_y[i];
  }
  m_target_x /= m_num_particles;
  m_target_y /= m_num_particles;
  m_target_a = atan2(m_target_y - m_nav_y, m_target_x - m_nav_x);
  m_target_r = sqrt(pow(m_target_x - m_nav_x, 2.0) + pow(m_target_y - m_nav_y, 2.0));
  cout << "Mean: Range=" << m_target_r << " | Azimuth=" << m_target_a*180/PI << endl;
}

void SimAcoBeaconRngBrg::StdDevParticles()
{
  double std_p_i;
  double std_r_i;
  m_std_azimuth = 0;
  m_std_range = 0;
  for (unsigned int i = 0; i < m_num_particles; i++) {
    std_p_i = abs(fmod((m_particles_a[i] - m_target_a + PI), 2.0*PI) - PI);
    std_p_i = pow(std_p_i, 2.0);
    m_std_azimuth += std_p_i;
    std_r_i = abs(m_particles_r[i] - m_target_r);
    std_r_i = pow(std_r_i, 2.0);
    m_std_range += std_r_i;
  }
  m_std_azimuth /= m_num_particles;
  m_std_azimuth = sqrt(m_std_azimuth);
  m_std_range /= m_num_particles;
  m_std_range = sqrt(m_std_range);
  cout << "Std. Dev: Range=" << m_std_range << " | Azimuth=" << m_std_azimuth*180/PI << endl;
}
