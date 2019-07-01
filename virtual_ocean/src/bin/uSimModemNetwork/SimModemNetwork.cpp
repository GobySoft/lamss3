// copyright 2010 H. Schmidt henrik@mit.edu
// 
// this file is part of lamss-shared-shared, a collection of MOOS
// processes for underwater acoustic simulation
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.

#include "SimModemNetwork.h"
#include <sstream>
#include <cstring>
#include <boost/lexical_cast.hpp>
#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/math/special_functions.hpp>


#include "goby/moos/moos_string.h"
using goby::moos::val_from_string;
#include "goby/time/legacy.h"

using namespace std;

using namespace goby::util::logger;
using namespace goby::util::logger_lock;
namespace gpb = goby::moos::protobuf;
using goby::common::goby_time;
using goby::glog;

const double pi = 3.14159;

boost::shared_ptr<SimModemNetworkConfig> master_config;
SimModemNetwork* SimModemNetwork::inst_ = 0;
void* SimModemNetwork::driver_library_handle_= 0;

int main(int argc, char * argv[])
{
    return goby::moos::run<SimModemNetwork>(argc, argv);
}

SimModemNetwork* SimModemNetwork::get_instance()
{
    if(!inst_)    {
        master_config.reset(new SimModemNetworkConfig);
        inst_ = new SimModemNetwork(*master_config);
    }
    return inst_;
}

SimModemNetwork::SimModemNetwork(SimModemNetworkConfig& cfg)
  : goby::moos::GobyMOOSApp(&cfg),
    cfg_(cfg),
    max_number_bounces(cfg_.max_number_bounces()),
    mseas_svp_updates(cfg_.mseas_svp_updates()), 
    mseas_update_interval(cfg_.mseas_update_interval()), 
    svp_requested(false),
    svp_range_resolution(cfg_.svp_range_resolution()), 
    svp_depth_resolution(cfg_.svp_depth_resolution()),
    sound_speed(1500.0),
    acomms_frequency(cfg_.acomms_frequency()),
    replica_path(cfg_.test_replica()),
    noise_profile(false),
    noise_direct(false),
    ISOTROPIC(false)
{

  set_ignore_stale(false);
  
  // set up subscriptions before starting separate thread
  subscribe("BELLHOP_RESPONSE", &SimModemNetwork::handle_bellhop_response, this);    
  subscribe("SVP_RESPONSE", &SimModemNetwork::handle_svp_response, this);    
  subscribe("NODE_REPORT", &SimModemNetwork::handle_node_report, this);
  subscribe_pb("NETSIM_NAV_UPDATE", &SimModemNetwork::handle_netsim_nav_update, this);
  subscribe("IMPULSE_REQUEST", &SimModemNetwork::handle_impulse_request, this);
  subscribe("TRANSMIT_TEST", &SimModemNetwork::handle_transmit_test, this);
  subscribe("AMBIENT_NL_DEPTH", &SimModemNetwork::handle_ambient_nl_depth, this);    
  subscribe("NOISE_VERT_DIR", &SimModemNetwork::handle_noise_vert_dir, this);    

  //
  glog.is(VERBOSE) && glog << cfg_.DebugString() << std::endl;
  //
  // Initialize
  //
  
  m_Geodesy.Initialise(cfg_.common().lat_origin(),cfg_.common().lon_origin());

  // Clear ray buffer
  eig.clear();
  
  time_last = 0;
  frame_number = 0;
  
  debug = false;
  debug2 = false;

  // prepare Bessel functions for noise covariances

   compCov.besgen.initBessel(2,40*M_PI);

   compCov.spherical_besgen.initSphBes(2,40*M_PI);

  start_time = MOOSTime();

  // Initilize nodes and acoustic contacts connections)
  // fixed to 2 nodes, generalize for dynamic allocation
  Nodes.resize(cfg_.node_size());
  acoustic_contacts.clear();
  for (int i=0; i<Nodes.size(); i++ ) 
    {
      Nodes[i].name = cfg_.node(i); 
      Nodes[i].ok_info = false;
      for (int j=0; j<Nodes.size() ; j++ )
	{
	  if (i != j)
	    {
	      CAcousticContact New;
	      New.ownship = cfg_.node(i);
	      New.contact = cfg_.node(j);
	      New.ready = false;
	      New.svp_ready = false;
	      New.new_svp_requested = false;
	      New.last_request = 0;
	      New.last_svp_request = 0;
	      acoustic_contacts.push_back(New);
	      glog.is(VERBOSE) && glog << "Creating contact " << New.contact
				       << "->" << New.ownship << endl;
	    }
	} 
    }

  svp_requests.clear();
  svp_request_id.clear();
  slow_loop_period = std::min(mseas_update_interval/acoustic_contacts.size(),
			      10.0); 

}

void SimModemNetwork::handle_transmit_test(const CMOOSMsg& msg)
{
  int cn = msg.GetDouble();
  if (cn < acoustic_contacts.size())
    {
      TransmitRequest transmit_request;
      transmit_request.set_source(acoustic_contacts[cn].ownship);
      transmit_request.set_receiver(acoustic_contacts[cn].contact);
      transmit_request.set_pingtime(MOOSTime());
      //      transmit_request.set_replica_path("../.tmp/test.dat");
      transmit_request.set_replica_path(replica_path);
      std::string serialized_transmit_request;
      serialize_for_moos(&serialized_transmit_request,transmit_request);
      m_Comms.Notify("TRANSMIT_REQUEST", serialized_transmit_request);
    }
}

void SimModemNetwork::handle_bellhop_response(const CMOOSMsg& msg)
{
  iBellhopResponse bellhop_response;
  std::string serialized_bellhop_response = msg.GetString();
  parse_for_moos(serialized_bellhop_response, &bellhop_response);
  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      bool my_resp = 
	(bellhop_response.requestor() == "uSimModemNetwork") &&
	(bellhop_response.request_number() == acoustic_contacts[i].bellhop_request_id) ;
      
      if (my_resp)
	{      
	  if (bellhop_response.has_output_file())
	    {
	      std::string bellhop_in = bellhop_response.output_file();
	       glog.is(VERBOSE) && glog << "Bellhop response " 
			<< acoustic_contacts[i].bellhop_request_id 
			<< " --- " << bellhop_in << std::endl;
	      
	      acoustic_contacts[i].ready = 
		acoustic_contacts[i].ray_table.readEigenRayTable(bellhop_in, 
								 max_number_bounces);
	      if (acoustic_contacts[i].ready)
		{
		  //		  number_depths = eigenray_table.getDepths(depth);
		  // Clear request id so we don't reprocess later
		  acoustic_contacts[i].bellhop_request_id = 0;
		}
	      else
		 glog.is(VERBOSE) && glog << "error reading eigenray table " << std::endl;
	      
	    }
	}
    }
}

void SimModemNetwork::handle_svp_response(const CMOOSMsg& msg)
{
  glog << "Received SVP_RESPONSE" << endl;
  svp_requested = !handleSVPResponse(msg); // reset flag to be ready for next request
}

void SimModemNetwork::handle_node_report(const CMOOSMsg& msg)
{
  std::string node_report = msg.GetString();
  std::string node_name;
  MOOSValFromString(node_name,node_report,"NAME");
  for (int i=0; i<Nodes.size(); i++)
    {
      if ( MOOSStrCmp(tolower(node_name),tolower(Nodes[i].name)) )
	{
	  CAcousticNode update;
	  update.name = tolower(node_name);
	  bool got_node_utc = MOOSValFromString(update.utc,node_report,"UTC_TIME") ||
	    MOOSValFromString(update.utc,node_report,"TIME");
	  bool got_node_x = MOOSValFromString(update.x,node_report,"X");
	  bool got_node_y = MOOSValFromString(update.y,node_report,"Y");
	  bool got_node_dep = MOOSValFromString(update.depth,node_report,"DEPTH") ||
	    MOOSValFromString(update.depth,node_report,"DEP");
	  bool got_node_spd = MOOSValFromString(update.speed,node_report,"SPD");
	  bool got_node_hdg = MOOSValFromString(update.heading,node_report,"HDG");
	  if (!MOOSValFromString(update.pitch,node_report,"PITCH"))
	    update.pitch = 0.0;
	  
	  update.ok_info = got_node_utc && got_node_x && got_node_y && got_node_dep && got_node_spd && got_node_hdg;
	  // Make sure it is latest and full node report (ignore fill-ins)
	  if (update.ok_info && update.utc > Nodes[i].utc)
	    {
	      Nodes[i] = update;
	      glog.is(DEBUG1) && glog << "Got info for node: " 
				  << node_name << endl;
	    }
	}
    }
}

void SimModemNetwork::handle_netsim_nav_update(const NavUpdate& nav)
{
    std::string node_name = goby::util::as<std::string>(nav.modem_tcp_port());

    double x,y;
    if(m_Geodesy.LatLong2LocalUTM(nav.lat(), nav.lon(), y, x))
    {
	std::stringstream nr;
	nr << "NAME=" << nav.modem_tcp_port() << "," <<
	    "UTC_TIME=" << std::setprecision(std::numeric_limits<double>::digits10) << nav.time() << "," <<
	    "X=" << x << "," <<
	    "Y=" << y << "," <<
	    "LAT=" << nav.lat() << "," << 
	    "LON=" << nav.lon() << "," << 
	    "SPD=" << nav.speed() << "," << 
	    "HDG=" << nav.heading() << "," <<
	    "DEPTH=" << nav.depth();
	
	publish("NODE_REPORT", nr.str());
    }
    else
    {
	glog.is(DEBUG1) && glog << "Failed to convert lat,lon to x,y for update: " << nav.ShortDebugString() << std::endl;
    }
    
}


void SimModemNetwork::handle_svp_info(const CMOOSMsg& msg)
{
  SVPInfoResponse svp_info;
  std::string serialized = msg.GetString();
  parse_for_moos(serialized, &svp_info);
}

void SimModemNetwork::handle_impulse_request(const CMOOSMsg& msg)
{
  ImpulseRequest request;
  std::string serialized_impulse_request = msg.GetString();
  parse_for_moos(serialized_impulse_request, &request);
  int con = findAcousticContact(request.receiver(),request.source()); 
  bool got_info = getNodeInfo(acoustic_contacts[con].ownship,ownship_x,ownship_y,
				  ownship_depth,ownship_heading,ownship_speed) &&
    getNodeInfo(acoustic_contacts[con].contact,contact_x,contact_y,
		contact_depth,contact_heading,contact_speed) ; 
  if (got_info && generate_eigenray_table(acoustic_contacts[con]))  
    {
      // Set variables needed for noise covariances
      sampling_frequency = request.sampling_frequency();
      nfft = request.nfft();
      
      glog.is(DEBUG1) && glog << "Preparing IMPULSE_RESPONSE" << endl;
      ImpulseResponse message;
      message.set_source(acoustic_contacts[con].contact);
      message.set_receiver(acoustic_contacts[con].ownship);
      if(request.has_request_id()) message.set_request_id(request.request_id());
      if(request.has_request_time()) message.set_request_time(request.request_time());
      
      std::vector<std::vector<double> >delay;
      std::vector<double> amplitude;
      std::vector<double> doppler;
      std::vector<double> elevation;
      std::vector<int> surface_bounces;
      std::vector<int> bottom_bounces;
    // ael moved here from modem_ray_arrivals for use in noise cvar computation
      std::vector<double> ael_x;
      std::vector<double> ael_y;
      std::vector<double> ael_z;
      modem_ray_arrivals(acoustic_contacts[con].ray_table,
			 delay,amplitude,doppler,elevation,
			 surface_bounces,bottom_bounces,
			 ael_x,ael_y,ael_z);
      // delay[num_rays][num_elem]
      if (delay.size() > 0)
	{
	  ImpulseResponse::RayTrace* raytrace = 0;
	  for (int j = 0; j < delay.size(); j++)
	    {
	      raytrace = message.add_raytrace();
	      //raytrace -> set_delay(delay[0][j]);
	      ImpulseResponse::RayTrace::ArrayElementDelay* element_delay = 0;
	      for (int i=0; i<delay[j].size(); i++)
		{
		  element_delay = raytrace -> add_element();
		  element_delay -> set_delay(delay[j][i]);
		}
	      raytrace -> set_amplitude(amplitude[j]);
	      raytrace -> set_doppler(doppler[j]);
	      raytrace -> set_elevation(elevation[j]*180.0/M_PI);
	      raytrace -> set_surface_bounces(surface_bounces[j]);
	      raytrace -> set_bottom_bounces(bottom_bounces[j]);
	    }
	  message.set_number_array_elements(delay[0].
					    size());
	}

      // The receiver is in the last range sector
      bellhop::protobuf::Environment::WaterColumn* water_column =
	acoustic_contacts[con].svp_response.mutable_water_column(acoustic_contacts[con].svp_response.water_column_size()-1);

      // Compute Noise covariance matrix if noise is correlated
      if (delay[0].size() > 1)
	{
	  if (computeNoiseCovariance(acoustic_contacts[con].noise_level,
				     get_ssp_value(water_column,ownship_depth),
				     ael_x,ael_y,ael_z) )
	    {
	      std::string covmat_path =  cfg_.base_dir_name() +
		acoustic_contacts[con].ownship + "_" + 
		goby::common::goby_file_timestamp() + ".cov";
	      write_covmat_file(covmat_path,acoustic_contacts[con].noise_level);
	      message.set_covariance_file(covmat_path);
	    }
	}
      
      message.set_noise_level(acoustic_contacts[con].noise_level);

      message.set_surface_sound_speed(get_ssp_value(water_column,0));
      message.set_bottom_sound_speed(get_ssp_value(water_column,
						   water_column->bathymetry()));
      message.set_receiver_sound_speed(get_ssp_value(water_column,
						     ownship_depth));
      std::string serialized_impulse_response;
      serialize_for_moos(&serialized_impulse_response,message);
      m_Comms.Notify("IMPULSE_RESPONSE", serialized_impulse_response);
    }
  else
    {
      glog.is(VERBOSE) && glog << "Insufficient info for impulse response "
			       << acoustic_contacts[con].contact 
			       << "->" 
			       << acoustic_contacts[con].ownship << endl;
      ImpulseResponse message;
      message.set_source(acoustic_contacts[con].contact);
      message.set_receiver(acoustic_contacts[con].ownship);
      if(request.has_request_id()) message.set_request_id(request.request_id());
      if(request.has_request_time()) message.set_request_time(request.request_time());
      std::string serialized_impulse_response;
      serialize_for_moos(&serialized_impulse_response,message);
      m_Comms.Notify("IMPULSE_RESPONSE", serialized_impulse_response);
    }

}

void SimModemNetwork::handle_ambient_nl_depth(const CMOOSMsg& msg)
{
  // noise depth profile
  noise_profile = parseNoiseProfile(msg.m_sVal, 
				    depth_nl,
				    noise_db);
}

void SimModemNetwork::handle_noise_vert_dir(const CMOOSMsg& msg)
{
  // noise depth profile
  noise_direct = parseNoiseDirectionality(msg.m_sVal, 
					  dom_noise_dir,
					  noise_dir_db);
  if (noise_direct)
    {
      glog << "noise_direct=true" << endl;
      noise_dir_theta.resize(dom_noise_dir.size());
      noise_dir_power.resize(dom_noise_dir.size());
      
      for (int i=0; i < dom_noise_dir.size(); i++)
	{
	  noise_dir_theta[i] = dom_noise_dir[i]*M_PI/180.0;
	  noise_dir_power[i] = pow(10.0,noise_dir_db[i]/10);
	}
    }
  else
    glog << "noise_direct=false" << endl;
  
}

double SimModemNetwork::get_ambient_noise(double depth, 
					  vector<double> dom_nl,
					  vector<double> noise_db)
{
  double nl;

  if (noise_profile)
    {
      int m = dom_nl.size();
      
      if (depth <= dom_nl[0]) 
	nl = noise_db[0];
      else if (depth >= dom_nl[m-1]) 
	nl= noise_db[m-1];
      else
	{
	  for (int j = 1; j < m; j++)
	    {
	      if (depth <= dom_nl[j] && depth > dom_nl[j-1])
		nl = noise_db[j-1] 
		  + (depth - dom_nl[j-1])/(dom_nl[j]-dom_nl[j-1])
		  *(noise_db[j] - noise_db[j-1]); 
	    }
	}
    }
  else
    nl = cfg_.noise_level();

  return(nl);
}

void SimModemNetwork::loop()
{
  
  double curr_time = MOOSTime();
  
  // request BELLHOP from queue: last in first out!
  if (bellhop_requests.size() > 0)
    {
      m_Comms.Notify("BELLHOP_REQUEST", bellhop_requests[bellhop_requests.size()-1]);
      bellhop_requests.pop_back();
    }  

  // process some events at a slower rate
  //const int slow_loop_period = 10;

  if(curr_time > last_slow_loop_t_ + slow_loop_period)
    slow_loop(curr_time);
  
}

void SimModemNetwork::slow_loop(double curr_time)
{
  // request SVP from queue: last in first out!
  if (svp_requests.size() > 0 && !svp_requested)
    {
      m_Comms.Notify("SVP_REQUEST", svp_requests[0]);
      glog << "Published SVP_REQUEST id " << (int) svp_request_id[0] << endl;
      for (int i=0; i<svp_requests.size()-1; i++)
	{
	  svp_requests[i] = svp_requests[i+1];
	  svp_request_id[i] = svp_request_id[i+1];
	}
      svp_requests.pop_back();
      svp_request_id.pop_back();
      svp_requested = true;
    }  

  // Update all acoustic contacts and extract requested impulse responses 

  for (int i=0; i< acoustic_contacts.size(); i++)
    {
      bool got_info = getNodeInfo(acoustic_contacts[i].ownship,ownship_x,ownship_y,
				  ownship_depth,ownship_heading,ownship_speed) &&
	getNodeInfo(acoustic_contacts[i].contact,contact_x,contact_y,
		    contact_depth,contact_heading,contact_speed) ; 
      if (got_info && generate_eigenray_table(acoustic_contacts[i]))  
	{
	  acoustic_contacts[i].noise_level = get_ambient_noise(ownship_depth,
							       depth_nl,noise_db);
	  glog.is(VERBOSE) && glog << "Updated ray table for " 
				   << acoustic_contacts[i].contact
				   << "->" << acoustic_contacts[i].ownship << endl;
	}
      else
	{
	  glog.is(DEBUG3) && glog << "Waiting for info for " 
				   << acoustic_contacts[i].contact
				   << "->" << acoustic_contacts[i].ownship << endl;
	}
    }

  // Here we should add slow loop actions.
  last_slow_loop_t_ = curr_time;
}

bool SimModemNetwork::generate_eigenray_table(CAcousticContact& acoustic_contact)
{
  bool eigenrays_ready = false;
  
  double range_x = (ownship_x - contact_x);
  double range_y = (ownship_y - contact_y);
  double range_req = sqrt(range_x*range_x + range_y*range_y);
  double depth_req = ownship_depth;
  
  
  // first get the ray table for this contact
  if (getAcousticContact(acoustic_contact.ownship,
			 acoustic_contact.contact,
			 eigenray_table))
    {
      CEigenRay eig;
      // Get the closest eigenray bundle
      eigenray_table.getEigenRays(range_req, depth_req, range_ref, depth_ref, eig);
      
      glog.is(VERBOSE) && glog << "Contact (" << acoustic_contact.contact << ") range = " << range_req
			       << " Raytracing range = " << eig.range
			       << endl;
      glog.is(VERBOSE) && glog << "Ownship (" << acoustic_contact.ownship << ") depth = " << depth_req
			       << " Raytracing depth = "
			       << eig.receiver_depth << endl;
      glog.is(VERBOSE) && glog << "Number of eigenrays: " 
			       << eig.num_rays << endl;
      tl_est = 1e-30;
      for (int iray = 0; iray < eig.num_rays; iray++)
	{
	  tl_est += pow(eig.bundle[iray].amp,2);
	}
      tl_est = - 10.0 * log10(tl_est/eig.num_rays);
      glog.is(VERBOSE) && glog << "Incoherent TL = " << tl_est << endl;
      
      if (fabs(range_req - acoustic_contact.last_range) > 2*cfg_.raytracing_range_box())
	{
	  double range_min;
	  double range_max;
	  double depth_min;
	  double depth_max;
	  int num_ranges;
	  int num_depths;
	  defineRayTracingBox(acoustic_contact,
			      range_min,range_max,
			      depth_min,depth_max,
			      num_ranges,num_depths);
	  requestRayTrace(acoustic_contact.ownship,acoustic_contact.contact, 
			  range_min, range_max, num_ranges, 
			  depth_min, depth_max, num_depths,
			  acomms_frequency);
	}
      // if mseas_svp_updates, request SVP
      if ( mseas_svp_updates && 
	   (MOOSTime() - acoustic_contact.last_svp_request >= 
	    mseas_update_interval ) )
	{
	  requestSVPUpdate(acoustic_contact.ownship,acoustic_contact.contact);
	}
      eigenrays_ready = true;
    }
  else if (MOOSTime() - acoustic_contact.last_request > cfg_.raytracing_interval())
    {
      double range_min;
      double range_max;
      double depth_min;
      double depth_max;
      int num_ranges;
      int num_depths;
      defineRayTracingBox(acoustic_contact,
			  range_min,range_max,
			  depth_min,depth_max,
			  num_ranges,num_depths);
      requestRayTrace(acoustic_contact.ownship, acoustic_contact.contact, 
		      range_min, range_max, num_ranges, 
		      depth_min, depth_max, num_depths,
		      acomms_frequency);
		  
      // if mseas_svp_updates, request SVP
      if (mseas_svp_updates &&
	  (MOOSTime() - acoustic_contact.last_svp_request >= 
	   mseas_update_interval) )
	{
	  requestSVPUpdate(acoustic_contact.ownship,acoustic_contact.contact);
	}
		  
    }

  return(eigenrays_ready);

}

bool SimModemNetwork::defineRayTracingBox(CAcousticContact& acoustic_contact,
					  double& r_min,
					  double& r_max,
					  double& dep_min,
					  double& dep_max,
					  int& num_r,
					  int& num_dep)
 {
   r_min = 1e8;
   r_max = -1e8;
   dep_min = 1e8;
   dep_max = -1e8;
   double spacing=1e8;
   
   double range_x = (ownship_x - contact_x);
   double range_y = (ownship_y - contact_y);
   double range = sqrt(range_x*range_x + range_y*range_y) ;
   double depth = ownship_depth; 
   double delta_t = MOOSTime() - acoustic_contact.last_rate_update; 
   r_min = std::min(range,r_min);
   r_max = std::max(range,r_max);
   dep_min = std::min(depth,dep_min);
   dep_max = std::max(depth,dep_max);

   double range_box = fabs(delta_t*acoustic_contact.last_rate);
   if (acoustic_contact.last_rate <= 0)
     r_min -= range_box;
   if (acoustic_contact.last_rate >= 0)
     r_max += range_box;

   if (cfg_.raytracing_range_box() > 0)
     num_r = 1+ceil((r_max-r_min)/cfg_.raytracing_range_box());
   else
     num_r = 3;
      
   // Raytracing resolution less critical for horizontal variability 
   num_dep = 3;
   spacing = (dep_max-dep_min)/(num_dep-1);

   glog.is(DEBUG1) && glog << "Ray Tracing box: rmin: " << r_min 
			   << " r_max: " << r_max
			   << " spacing: " << spacing
			   << " num ranges: " << num_r
			   << ", num depths: " << num_dep << std::endl;
   if (acoustic_contact.last_rate_update > 0)
     {
       acoustic_contact.last_rate = (range-acoustic_contact.last_range)/delta_t;	    
       glog.is(DEBUG1) && glog << endl << ">>> rate_update= " 
			       << acoustic_contact.last_rate << endl; 
       glog.is(DEBUG1) && glog  << ">>> delta_t = " << delta_t << endl << endl ;
     }
   else
     acoustic_contact.last_rate = 0;
   acoustic_contact.last_range = range;
   acoustic_contact.last_rate_update = MOOSTime();
   return(true);
   
}
 
 
 void SimModemNetwork::modem_ray_arrivals(CEigenRayTable eigenray_table,
					  std::vector<std::vector<double> >& delay,
					  std::vector<double> & amplitude,
					  std::vector<double> & doppler,
					  std::vector<double> & elevation,
					  std::vector<int> & surface_bounces,
					  std::vector<int> & bottom_bounces,
					  std::vector<double>& ael_x,
					  std::vector<double>& ael_y,
					  std::vector<double>& ael_z
)
{
  // Project absolute vs response onto physical orientation
  double deg2rad = M_PI/180;
  double hdg_ang = ownship_heading*deg2rad;
  double pitch_ang = ownship_pitch;
  double vs_costh = cos(hdg_ang);
  double vs_sinth = sin(hdg_ang);
  double vs_cosel = cos(pitch_ang);
  double vs_sinel = sin(pitch_ang);
      
  double range_x = (ownship_x - contact_x);
  double range_y = (ownship_y - contact_y);
  double range = sqrt(range_x*range_x + range_y*range_y);

  // Modem receiving array
  //  std::vector<double> local_x, local_y, local_z, ael_x, ael_y, ael_z, ael_range, ael_depth;
  // ael arrays moved to header for use in computeNoiseCovar
  std::vector<double> local_x, local_y, local_z, ael_range, ael_depth;
  int nelm = cfg_.array_element_size();
  if (nelm >0)
    {
      local_x.resize(nelm);
      local_y.resize(nelm);
      local_z.resize(nelm);
      ael_range.resize(nelm);
      ael_depth.resize(nelm);
      for (int i=0; i<nelm; i++)
	{
	  local_x[i] = cfg_.array_element(i).x_offset();
	  local_y[i] = cfg_.array_element(i).y_offset();
	  local_z[i] = cfg_.array_element(i).z_offset();
	}

      array_locations(ownship_x, ownship_y, -ownship_depth, hdg_ang, pitch_ang,
		      local_x,local_y,local_z,ael_x,ael_y,ael_z);
      for (int i=0; i<nelm; i++)
	{
	  range_x = (ael_x[i] - contact_x);
	  range_y = (ael_y[i] - contact_y);
	  ael_range[i] = sqrt(range_x*range_x + range_y*range_y);
          ael_depth[i] = - ael_z[i];
	}
    } 
  else
    {
      nelm = 1;
      ael_range.resize(nelm);
      ael_depth.resize(nelm);
      ael_range[0] = range;
      ael_depth[0] = ownship_depth;
    }     
  // Get eigenrays closesst to phone
  double range_ref, depth_ref;
  CEigenRay eig;
      
  double depth_req = ownship_depth;
  eigenray_table.getEigenRays(range, depth_req, 
			      range_ref, depth_ref, eig);
  double range_ray = eig.range;
  double depth_ray = eig.receiver_depth;
  int num_rays     = eig.num_rays;
  glog.is(DEBUG1) && glog << "Source range= " << range
			  << " Raytracing range = " << eig.range << endl;
  glog.is(DEBUG1) && glog << "Receiver depth= " << depth_req 
			  << " Raytracing depth = " << eig.receiver_depth
			  << endl;
  glog.is(DEBUG1) && glog << "Number of eigenrays: " << eig.num_rays << endl;
	
  //  double delta_r = range - range_ray;
  //  double delta_dep = ownship_depth - depth_ray;  
  //  double delta_z = - delta_dep;

  // Computing vector components in absolute coordinates
  //double costh = cos(atan2(range_y, range_x));
  //double sinth = sin(atan2(range_y, range_x));
  // Get eigenray delays for sensor
  std::vector<std::vector<double> > ray_delays;
  std::vector<double> ray_amplitudes;
  std::vector<double> rcv_ray_cos;
  std::vector<double> src_ray_cos;

  ray_arrivals(eig, 
	       ael_range,
	       ael_depth,
	       src_ray_cos,
	       rcv_ray_cos,
	       ray_delays, 
	       ray_amplitudes); 
  // doppler_corrections
  double vr_S;
  double vr_R;
  ray_doppler_vel(contact_speed, (90.0-contact_heading)*M_PI/180.0, 
		  contact_pitch,
		  ownship_speed, (90.0-ownship_heading)*M_PI/180.0, 
		  ownship_pitch,
		  vr_S,vr_R);
  std::vector<double> ray_doppler;
  ray_doppler.resize(eig.num_rays);
  
  double oneossp = 1/sound_speed;
      
  for (int i_ray=0; i_ray < eig.num_rays; i_ray++)
    {
      for (int i=0; i<nelm; i++)
	ray_delays[i_ray][i] *=
	  (1.0+vr_R*rcv_ray_cos[i_ray]*oneossp);
      ray_doppler[i_ray] = 1 + (src_ray_cos[i_ray]*vr_S - 
				rcv_ray_cos[i_ray]*vr_R)*oneossp;
    }

  // ray receiver elevations
  std::vector<double> ray_elevations;
  std::vector<int> ray_surface_bounces;
  std::vector<int> ray_bottom_bounces;
  ray_elevations.resize(eig.num_rays);
  ray_surface_bounces.resize(eig.num_rays);
  ray_bottom_bounces.resize(eig.num_rays);
  for (int i_ray=0; i_ray < eig.num_rays; i_ray++)
    {
      ray_elevations[i_ray] = eig.bundle[i_ray].dip_receiver;
      ray_surface_bounces[i_ray] = eig.bundle[i_ray].surface_bounces;
      ray_bottom_bounces[i_ray] = eig.bundle[i_ray].bottom_bounces;
    }

  delay = ray_delays;
  amplitude = ray_amplitudes;
  doppler= ray_doppler;
  elevation= ray_elevations;
  surface_bounces = ray_surface_bounces;
  bottom_bounces = ray_bottom_bounces;
}

 
 bool SimModemNetwork::ray_arrivals(CEigenRay eig, 
				    vector<double> rcv_range,
				    vector<double> rcv_depth, 
				   std::vector<double> & cos_Src,
				   std::vector<double> & cos_Rcv,
				    std::vector<std::vector<double> > & delays, 
				   std::vector<double> & amplitudes)
{
  int num_eig_rays = eig.num_rays;
  //1. calculate velocities of each array elements in x, y, and z directions
  cos_Src.resize(num_eig_rays);
  cos_Rcv.resize(num_eig_rays);
  std::vector<double>sin_Rcv;
  sin_Rcv.resize(num_eig_rays);
  std::vector<double>amp;
  amp.resize(num_eig_rays);
  delays.resize(num_eig_rays);
  amplitudes.resize(num_eig_rays);

  if (num_eig_rays>0)
    {
      // Vertical ray angles
      for(int i_ray=0; i_ray<num_eig_rays; i_ray++)
	{ cos_Src[i_ray] = cos(eig.bundle[i_ray].dip_source);
	  cos_Rcv[i_ray] = cos(eig.bundle[i_ray].dip_receiver);
	  sin_Rcv[i_ray] = sin(-eig.bundle[i_ray].dip_receiver);
	  amp[i_ray] = eig.bundle[i_ray].amp;
	}
      
      double ref_range = eig.range;
      double ref_depth = eig.receiver_depth;

      double oneossp = 1/sound_speed;
      int nelm = rcv_range.size();
      for(int ray_iter=0; ray_iter<num_eig_rays; ray_iter++)
	{
	  delays[ray_iter].resize(rcv_range.size());
	  for (int i=0; i<nelm; i++)
	    {
	      double dz = rcv_depth[i] - ref_depth;
	      double dr = rcv_range[i] - ref_range;
	      delays[ray_iter][i] = (eig.bundle[ray_iter].tau + 
				     (((dr*cos_Rcv[ray_iter]) - 
				       (dz*sin_Rcv[ray_iter]))*
				      oneossp));
	    }
	  amplitudes[ray_iter] = amp[ray_iter];    
      // free surface reflection handled by change of sign
	  if ( cos(eig.bundle[ray_iter].phase) < -0.99) 
	    amplitudes[ray_iter] = - amp[ray_iter];    
	  else if ( cos(eig.bundle[ray_iter].phase) < 0.99) 
	    // turning points by time shift
	    for (int i=0; i<nelm; i++)
	      delays[ray_iter][i] -= 1.0/eig.freq * eig.bundle[ray_iter].phase/(2*M_PI);
	}
      return(true);
    }
  return(false);
}

void SimModemNetwork::ray_doppler_vel(double v_S, double a_S,double e_S, 
				      double v_R, double a_R, double e_R, 
				      double& vr_S, double& vr_R )
{
  //
  // Computes the projected doppler velocities for an eigenray
  //
  double dt=0;

  double vx_S=v_S*cos(e_S)*cos(a_S);
  double vy_S=v_S*cos(e_S)*sin(a_S);
  double xp_S=contact_x+vx_S*dt;
  double yp_S=contact_y+vy_S*dt;
  // and current receiver locations
  double vx_R=v_R*cos(e_R)*cos(a_R);
  double vy_R=v_R*cos(e_R)*sin(a_R);
  double xp_R=ownship_x+vx_R*dt;
  double yp_R=ownship_y+vy_R*dt;
  
  // projected relative velocities
  double dist_TR = sqrt(pow((xp_R - xp_S),2) +pow((yp_R - yp_S),2));
  vr_S = ((vx_S*(xp_R - xp_S)) + (vy_S*(yp_R - yp_S)))/dist_TR;
  vr_R = ((vx_R*(xp_R - xp_S)) + (vy_R*(yp_R - yp_S)))/dist_TR;
}

bool SimModemNetwork::getNodeInfo(std::string node_name,
				  double& node_x ,
				  double& node_y ,
				  double& node_depth , 
				  double& node_heading ,
				  double& node_speed )
{
  for (int i =  0; i < Nodes.size(); i++) 
    if (MOOSStrCmp(node_name,Nodes[i].name) && Nodes[i].ok_info)
      {
	node_name = Nodes[i].name;
	node_x = Nodes[i].x;
	node_y = Nodes[i].y;
	node_depth = Nodes[i].depth;
	node_heading = Nodes[i].heading;
	node_speed = Nodes[i].speed;
	double utc = MOOSTime();
	node_x = node_x+node_speed*sin(node_heading*M_PI/80.0)*(utc-Nodes[i].utc);
	node_y = node_y+node_speed*cos(node_heading*M_PI/80.0)*(utc-Nodes[i].utc);
	return true;
      }
  return false;
}

int SimModemNetwork::findAcousticContact(std::string os,
					 std::string cn)
{
  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      if (MOOSStrCmp(acoustic_contacts[i].ownship,os) &&
	  MOOSStrCmp(acoustic_contacts[i].contact,cn))
	{
	  return(i); 
	}
    }
  // not found, something wrong, return -1
  return(-1);
}

bool SimModemNetwork::putAcousticContactSVP(SVP::Response svp_response)
{
  if (svp_response.request_id() == 0)
    {
      // initial water column published by iBellhop
      for (int i = 0; i < acoustic_contacts.size(); i++)
	{
	  glog.is(DEBUG1) && glog << "Initializing water column for all contacts " 
				  << endl;
	  acoustic_contacts[i].svp_response = svp_response;
	  acoustic_contacts[i].svp_ready = true;
	  acoustic_contacts[i].new_svp_requested = false;
	}
      return(true);
    }
  else
    {
      for (int i = 0; i < acoustic_contacts.size(); i++)
	{
	  glog.is(DEBUG1) && glog << "Checking " << (int) svp_response.request_id() << endl;
	  glog.is(DEBUG1) && glog << "request_id " << (int) acoustic_contacts[i].svp_request_id << endl; 
	  if (svp_response.request_id() == (int) acoustic_contacts[i].svp_request_id)
	    {
	      acoustic_contacts[i].svp_response = svp_response;
	      acoustic_contacts[i].svp_ready = true;
	      acoustic_contacts[i].new_svp_requested = false;
	      return(true); 
	    }
	}
    }
  return(false);
}

bool SimModemNetwork::getAcousticContact(std::string os, 
					 std::string cn, 
					 CEigenRayTable& eig_table )
{
  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      if (MOOSStrCmp(acoustic_contacts[i].ownship,os) &&
	  MOOSStrCmp(acoustic_contacts[i].contact,cn) &&
	  acoustic_contacts[i].ready)
	{
	  eig_table = acoustic_contacts[i].ray_table;
	  if (eig_table.num_ranges > 0 && eig_table.num_depths > 0)
	    return(true);
	  else
	    return(false);
	}
    }
  return(false);
}

bool SimModemNetwork::getAcousticContactSVP(std::string os, 
					    std::string cn, 
					    SVP::Response& svp_response )
{
  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      if (MOOSStrCmp(acoustic_contacts[i].ownship,os) &&
	  MOOSStrCmp(acoustic_contacts[i].contact,cn))
	{
	  if(acoustic_contacts[i].svp_ready)
	    {
	      svp_response = acoustic_contacts[i].svp_response;
	      return(true);
	    }
	  else
	    return(false);
	}
    }
  return(false);
}

bool SimModemNetwork::readyAcousticContact(std::string os, std::string cn)
{
  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      if (MOOSStrCmp(acoustic_contacts[i].ownship,os) &&
	  MOOSStrCmp(acoustic_contacts[i].contact,cn))
	{
	  return(acoustic_contacts[i].ready);
	}
    }
  return(false);
}

bool SimModemNetwork::readyAcousticContactSVP(std::string os, std::string cn)
{
  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      if (MOOSStrCmp(acoustic_contacts[i].ownship,os) &&
	  MOOSStrCmp(acoustic_contacts[i].contact,cn))
	{
	  return(acoustic_contacts[i].svp_ready);
	}
    }
  return(false);
}

bool SimModemNetwork::requestRayTrace(std::string os, std::string cn,  
				       double r_min, double r_max, int num_r, 
				       double dep_min, double dep_max, int num_dep,
				       double freq)
{

  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      if (MOOSStrCmp(acoustic_contacts[i].ownship,os) &&
	  MOOSStrCmp(acoustic_contacts[i].contact,cn))
	{
	  // if we waited more than twice the interval, iBellhop must have missed request
	  if (readyAcousticContact(os,cn) || (MOOSTime() - acoustic_contacts[i].last_request) > 2*cfg_.raytracing_interval())
	    {
	      // Request raytracing
	      string acoustic_contact = cn;

	      //	      double direct_theta = atan2(depth,range)*180.0/M_PI;
	      double theta_max = 60.0;
	      //	      if (direct_theta > 0.5*theta_max)
	      //		theta_max = 0.5*(direct_theta+90.0);

	      iBellhopRequest request;

	      bellhop::protobuf::Environment::AdaptiveInfo* 
		adaptive_info = request.mutable_env()->mutable_adaptive_info();
	      adaptive_info->set_contact(cn);
	      //  adaptive_info->set_ownship(receiver);
	      adaptive_info->set_ownship(os);
	      adaptive_info->set_look_ahead_seconds(1);
	      adaptive_info->set_read_shd(false);
	      adaptive_info->set_auto_receiver_ranges(false);
	      adaptive_info->set_auto_source_depth(true);
	      bellhop::protobuf::Environment::Output* output = request.mutable_env()->mutable_output();
	      output->set_type(bellhop::protobuf::Environment::Output::ARRIVAL_TIMES);
	      bellhop::protobuf::Environment::Receivers* receivers = request.mutable_env()->mutable_receivers();
	      receivers->set_number_in_depth(num_dep);
	      receivers->set_number_in_range(num_r);
	      bellhop::protobuf::Environment::Receivers::ReceiverPosition* first = request.mutable_env()->mutable_receivers()->mutable_first();
	      first->set_range(r_min);
	      first->set_depth(dep_min);
	      bellhop::protobuf::Environment::Receivers::ReceiverPosition* last = request.mutable_env()->mutable_receivers()->mutable_last();
	      last->set_range(r_max);
	      last->set_depth(dep_max);
	      bellhop::protobuf::Environment::Beams* beams = request.mutable_env()->mutable_beams();
	      beams->set_theta_min(int(-theta_max));
	      beams->set_theta_max(int(theta_max));
	      beams->set_number(3500);
	      bellhop::protobuf::Environment* env = request.mutable_env();
	      env->set_freq(acomms_frequency);
	      // Generate random request id
	      int request_id =   int(1e9 * rand()/RAND_MAX);
	      acoustic_contacts[i].bellhop_request_id  = request_id;
	       glog.is(VERBOSE) && glog << "Bellhoprequest " << request_id << std::endl;
	      request.set_request_number(acoustic_contacts[i].bellhop_request_id);
	      // mseas svp updates
	      if (mseas_svp_updates && acoustic_contacts[i].svp_ready)
		{
		  request.mutable_env()->mutable_water_column()->
		    CopyFrom(acoustic_contacts[i].svp_response.water_column());
		  request.set_water_column_action(iBellhopRequest::OVERWRITE);
		}
	      std::string serialized_bellhop_request;
	      serialize_for_moos(&serialized_bellhop_request,request);
	      // Queue request for slow loop
	      bellhop_requests.push_back(serialized_bellhop_request);

	      // Wait for Bellhop before making next request
	      
	      //	      acoustic_contacts[i].ready = false;
	      acoustic_contacts[i].last_request = MOOSTime();
	    } 
	  return(true);
	}
    }
  return(false);
}

bool SimModemNetwork::requestSVPUpdate(std::string os, std::string cn)
{

  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      if (MOOSStrCmp(acoustic_contacts[i].ownship,os) &&
	  MOOSStrCmp(acoustic_contacts[i].contact,cn))
	{
	  if (readyAcousticContactSVP(os,cn) || acoustic_contacts[i].last_svp_request <= 0 )
	    {
	      // if an update has been requested, don't request a new one
	      if (!acoustic_contacts[i].new_svp_requested)
		{
		  SVP::Request svp_request;
		  std::string serialized_svp_request;
		  double ownship_lat, ownship_lon, contact_lat, contact_lon;
		  int request_number = int(1e9 * rand()/RAND_MAX);
		  acoustic_contacts[i].svp_request_id = request_number;
		  svp_request.set_request_id(request_number);
		  svp_request.set_utc_time(MOOSTime());
		  m_Geodesy.UTM2LatLong(ownship_x,ownship_y,ownship_lat,ownship_lon); 
		  m_Geodesy.UTM2LatLong(contact_x,contact_y,contact_lat,contact_lon); 
		  
		  svp_request.set_receiver_lat(ownship_lat);
		  svp_request.set_receiver_lon(ownship_lon);
		  svp_request.set_source_lat(contact_lat);
		  svp_request.set_source_lon(contact_lon);
		  svp_request.set_delta_range(svp_range_resolution);
		  svp_request.set_delta_depth(svp_depth_resolution);
		  
		  
		  serialize_for_moos(&serialized_svp_request, svp_request);
		  //m_Comms.Notify("SVP_REQUEST", serialized_svp_request);
		  // Queue request for slow loop
		  svp_requests.push_back(serialized_svp_request);
		  svp_request_id.push_back(request_number);
		  // if not ready use old one
		  acoustic_contacts[i].new_svp_requested = true;
		  acoustic_contacts[i].last_svp_request = MOOSTime();	    
		  glog << "Queued SVP request id " << request_number 
		       << " " << cn << "->" << os << endl;
		} 
	    }
	  return(true);
	}
    }
  return(false);
}

bool SimModemNetwork::handleSVPResponse(const CMOOSMsg& msg)
{
  std::string serialized_svp_response = msg.GetString();
  SVP::Response svp_response;
  parse_for_moos(serialized_svp_response, &svp_response);
  if (putAcousticContactSVP(svp_response))
    {
      glog << "Got SVP update for request id " << svp_response.request_id() 
	   << endl; 
      return(true);
    }
  else
    return(false);
  
}

bool SimModemNetwork::parseNoiseProfile( string noise_in, 
					 vector<double>& dom_nl,
					 vector<double>& noise_db)
{
  // read the ambient noise profile
  dom_nl.clear();
  noise_db.clear();

  std::string depths_nl, nls;    
  val_from_string(depths_nl, noise_in, "depths");

  bool delta;
  if (val_from_string(nls, noise_in, "NL"))
    {
      delta = false;
    }
  else if (val_from_string(nls, noise_in, "delta"))
    {
      delta = true;
    }
  else
    {
      glog << "Error parsing Noise Depth Profile" << endl;
      return(false);
    }


  std::vector<std::string> depth_nl_vec;
  std::vector<std::string> nl_vec;
  boost::split(depth_nl_vec, depths_nl, boost::is_any_of(","));
  boost::split(nl_vec, nls, boost::is_any_of(","));
  
  try
    {
      
      BOOST_FOREACH(const std::string& s, depth_nl_vec)
        {
	  if(s=="inf")
	    dom_nl.push_back(std::numeric_limits<double>::infinity());
	  else
	    dom_nl.push_back(boost::lexical_cast<double>(s));
        }
      
      double max_noise = 0;
      BOOST_FOREACH(const std::string& s, nl_vec)
        {
	  if(s=="inf")
	    noise_db.push_back(std::numeric_limits<double>::infinity());
	  else
            {
	      double d = boost::lexical_cast<double>(s);
	      if (delta) 
		d += cfg_.noise_level();
	      if(d > max_noise)
		max_noise = d;
	      noise_db.push_back(d);
            }
        }
      
      // remove infinity values in favor of MAX non infinite TL.
      BOOST_FOREACH(double& d, noise_db)
        {
	  if(d == std::numeric_limits<double>::infinity())
	    d = max_noise;
        }
    }
  catch (boost::bad_lexical_cast&)
    { return false; }
  return(true);
}    

bool SimModemNetwork::parseNoiseDirectionality( string noise_in, 
					  vector<double>& dom_nl,
					  vector<double>& noise_db)
{
  bool elev;
  // read the ambient noise profile
  dom_nl.clear();
  noise_db.clear();

  std::string depths_nl, nls;    
  if ( val_from_string(depths_nl, noise_in, "theta") )
    elev = false;
  else if ( val_from_string(depths_nl, noise_in, "elevation") )
      elev=true;
  else
    return(false);

  bool got_it = false;
  if ( val_from_string(nls, noise_in, "S")  )
    {
      got_it = true;
    }
  else if ( val_from_string(nls, noise_in, "delta") )
    {
      got_it = true;
    }
  else 
    {
      got_it = false;
      glog << "Error parsing Noise Directionality" << endl;
      return(false);
    }

  std::vector<std::string> depth_nl_vec;
  std::vector<std::string> nl_vec;
  boost::split(depth_nl_vec, depths_nl, boost::is_any_of(","));
  boost::split(nl_vec, nls, boost::is_any_of(","));


  try
    {
      
      BOOST_FOREACH(const std::string& s, depth_nl_vec)
        {
	  if(s=="inf")
	    dom_nl.push_back(std::numeric_limits<double>::infinity());
	  else
	    dom_nl.push_back(boost::lexical_cast<double>(s));
        }
      
      double max_noise = 0;
      BOOST_FOREACH(const std::string& s, nl_vec)
        {
	  if(s=="inf")
	    noise_db.push_back(std::numeric_limits<double>::infinity());
	  else
            {
	      double d = boost::lexical_cast<double>(s);
	      if(d > max_noise)
		max_noise = d;
	      noise_db.push_back(d);
            }
        }
      
      // remove infinity values in favor of MAX non infinite TL.
      BOOST_FOREACH(double& d, noise_db)
        {
	  if(d == std::numeric_limits<double>::infinity())
	    d = max_noise;
        }
    }
  catch (boost::bad_lexical_cast&)
    { return false; }

  // If directionality specified in elevation, convert to polar angle
  if (elev)
    {
      for (unsigned int i=0; i<dom_nl.size(); i++)
	dom_nl[i] = 90 - dom_nl[i];
    }
  return(true);
}    

double SimModemNetwork::get_ssp_value(bellhop::protobuf::Environment::WaterColumn* water_column, double depth)
{

  int m_coef = water_column->eof_coef_size();
  std::vector<double> eof_coef;
  eof_coef.resize(m_coef);
  for (int j = 0; j < m_coef; j++)
    eof_coef[j] = water_column->eof_coef(j);

  for(int i = 0, n = water_column->sample_size(); i < n-1; ++i)
    {
      if(depth >= water_column->sample(i).depth() && 
	 depth <= water_column->sample(i+1).depth())
	{
	  double cp    = water_column->sample(i).cp();  
	  int m = water_column->sample(i).eof_size();
	  double dep_up = water_column->sample(i).depth();  
	  double ssp_up = cp ;
	  for (int j = 0; j < std::min(m,m_coef); j++)
	    ssp_up += water_column->sample(i).eof(j) * eof_coef[j];

	  cp = water_column->sample(i+1).cp();  
	  double dep_dn = water_column->sample(i+1).depth();  
	  double ssp_dn = cp ;
	  for (int j = 0; j < std::min(m,m_coef); j++)
	    ssp_dn += water_column->sample(i+1).eof(j) * eof_coef[j];

	  double ssp = ssp_up + 
	    (depth - dep_up) * (ssp_dn - ssp_up) / (dep_dn - dep_up);
	  return(ssp);
	}
    }
  return(0.0);
}

void SimModemNetwork::array_locations(double nav_x, 
				      double nav_y, 
				      double nav_z,
				      double nav_yaw, 
				      double nav_pitch,
				      vector<double> local_x,
				      vector<double> local_y,
				      vector<double> local_z,
				      vector<double>& ael_x,
				      vector<double>& ael_y,
				      vector<double>& ael_z
				      )
{

  int m_nelm = local_x.size();

  ael_x.resize(m_nelm);
  ael_y.resize(m_nelm);
  ael_z.resize(m_nelm);

  // Transform to absolute coordinates. Ignoring roll
  // Assumes up pos MOOS coordinates and yaw,pitch in radians.
  // nav_yaw = nav_heading * M_PI/180;
  double cy = cos(nav_yaw);
  double sy = sin(nav_yaw);
  double cp = cos(nav_pitch);
  double sp = sin(nav_pitch);

  for (int i=0; i<m_nelm; i++)
    {
      ael_x[i] =  nav_x + ( local_x[i]*cy + (local_y[i]-local_z[i]*sp)*sy)*cp;
      ael_y[i] =  nav_y + (-local_x[i]*sy + (local_y[i]-local_z[i]*sp)*cy)*cp;
      ael_z[i] =  nav_z +  local_z[i]*cp+local_y[i]*sp;
    }

}

bool SimModemNetwork::computeNoiseCovariance(double nl,
					     double water_c,
					     std::vector<double> array_x,
					     std::vector<double> array_y,
					     std::vector<double> array_z
					     )
{

  if (noise_direct || ISOTROPIC)
    {

      double p_intens = pow(10.0,nl/10) * sampling_frequency * 0.5 * nfft;
      double rho = 1000;
      // Compute correlated noise only for spectrum of 
      // interest
      double num_freqs = nfft/2+1;
      double delta_f = sampling_frequency/nfft;
      minimum_frequency = cfg_.acomms_frequency()-0.5*cfg_.acomms_bandwidth();
      maximum_frequency = cfg_.acomms_frequency()+0.5*cfg_.acomms_bandwidth();
      indx_first = 0;
      if (minimum_frequency >= 0)
	indx_first = int(minimum_frequency/delta_f);
      indx_last = num_freqs;
      if (maximum_frequency < 0.5*sampling_frequency)
	indx_last = int(maximum_frequency/delta_f);

      glog << "Building correlated noise covariances, indx_first, indx_last= "
	   << indx_first << "," << indx_last << endl;
      
      int num_elem = array_x.size();
      
      std::vector<double> delta_r;
      std::vector<double> delta_z;
      
      std::vector<double> delta_y;
      std::vector<double> delta_x;
      std::vector<double> delta_R;
      for (int j = 0; j < num_elem; j++)
	for (int i = 0; i < num_elem; i++)
	  {
	    delta_z.push_back(array_z[i] - array_z[j]);
	    
	    if (ISOTROPIC)
	      {
		delta_x.push_back(array_x[i] - array_x[j]);
		delta_y.push_back(array_y[i] - array_y[j]);
		//Distance between sensor i and sensor j
		delta_R.push_back(sqrt(pow(array_x[i]-array_x[j],2) 
				       + pow(array_y[i]-array_y[j],2) 
				       + pow(array_z[i]-array_z[j],2))); 
	      }
	    else
	      {
		//Horizontal distance between sensor i and sensor j
		delta_r.push_back(sqrt(pow(array_x[i]-array_x[j],2) 
				       + pow(array_y[i]-array_y[j],2))); 
	      }
	  }
      // Clear noise covariances
      noise_covar.clear();
      
      for (int k = indx_first; k<= indx_last; k++)
	{
	  double freq = k*delta_f;
	  
	  std::vector<std::complex<double> > cov;
	  CovMat newcov;
	  
	  cov.clear();
	  cov = vecUtils.zzeros(num_elem,num_elem);
	  if (k > 0)
	    {
	      compCov.initialize(freq,water_c,noise_dir_theta,noise_dir_power);
	      double norm = 0;
	      for (int i = 0; i < num_elem; i++)
		{
		  for (int j = 0; j <= i; j++)
		    {
		      int indx = i + num_elem * j;
		      std::complex<double> hyd_cov;
		      if (ISOTROPIC)
			compCov.hydIsotropicCovariance(delta_R[indx], hyd_cov);
		      else
			compCov.hydCovariance(delta_r[indx], delta_z[indx],hyd_cov);
		      cov[indx] = hyd_cov;
		    }
		  norm += cov[i*(1+num_elem)].real();
		}
	      norm /= num_elem;
	      // now scale covariances
	      double scale = p_intens/norm;
	      
	      for (int i=0; i<num_elem*num_elem; i++)
		cov[i] *= scale;
	      
	    }
	  newcov.putCovMat(num_elem, freq, cov);
	  noise_covar.push_back(newcov);
	}	  
      glog <<"Built covariance matrices" << endl;
      return(true);
    }
  else
    return(false);
  
}

bool SimModemNetwork::write_covmat_file(std::string filename,
					       double nl_now)
{

  FILE * cov_data_file;
  if ( (cov_data_file = fopen(filename.c_str(), "wb")) == NULL)
    {
      glog << "Unable to open a file:" << filename << endl;
      exit(EXIT_FAILURE);
    }
  else
    {
      // Compute correlated noise only for spectrum of 
      // interest
      double delta_f = sampling_frequency/nfft;
      double t_now = MOOSTime();

      fwrite(&t_now, sizeof(double), 1 , cov_data_file);
      fwrite(&nl_now, sizeof(double), 1 , cov_data_file);
      fwrite(&minimum_frequency, sizeof(double), 1 , cov_data_file);
      fwrite(&minimum_frequency, sizeof(double), 1 , cov_data_file);
      fwrite(&delta_f, sizeof(double), 1 , cov_data_file);
      fwrite(&indx_first, sizeof(int), 1 , cov_data_file);
      fwrite(&indx_last, sizeof(int), 1 , cov_data_file);

      
      int num_sensors = 1;
      if (cfg_.array_element_size() > 0)
	num_sensors = cfg_.array_element_size();

      fwrite(&num_sensors, sizeof(int), 1 , cov_data_file);

      for (int f_iter=indx_first; f_iter <= indx_last; f_iter++)
	{
	  
	  vector<complex<double> > covmat;
	  vector<complex<double> > chollow;
	  double f;

	  noise_covar[f_iter-indx_first].getCovLow(num_sensors, f, covmat);
	  noise_covar[f_iter-indx_first].getCholLow(num_sensors, f, chollow);
	  fwrite(&f, sizeof(double), 1 , cov_data_file);

	  int num = num_sensors*(num_sensors+1)/2;
	  for (int i=0; i<num; i++)
	    {
	      float rr = float(covmat[i].real()); 
	      float ri = float(covmat[i].imag()); 
	      fwrite(&rr, sizeof(float), 1 , cov_data_file);
	      fwrite(&ri, sizeof(float), 1 , cov_data_file);
	    }
	  for (int i=0; i<num; i++)
	    {
	      float rr = float(chollow[i].real()); 
	      float ri = float(chollow[i].imag()); 
	      fwrite(&rr, sizeof(float), 1 , cov_data_file);
	      fwrite(&ri, sizeof(float), 1 , cov_data_file);
	    }
	}

      fclose (cov_data_file);
      glog << "Wrote covatiance file: "<< filename << endl;
    }
  return(true);  
}

