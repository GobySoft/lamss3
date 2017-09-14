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

#include "SimPassiveSonar.h"
#include <sstream>
#include <cstring>
#include <boost/lexical_cast.hpp>
#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/math/special_functions.hpp>


#include "goby/version.h"
#if GOBY_VERSION_MAJOR >= 2
#include "goby/moos/moos_string.h"
using goby::moos::val_from_string;
#else
#include "goby/util/string.h"
using goby::util::val_from_string;
#endif

//#include "goby/util/time.h"

using namespace std;

using namespace goby::common::logger;
using namespace goby::common::logger_lock;
namespace gpb = goby::moos::protobuf;
using goby::common::goby_time;
using goby::glog;

const double pi = 3.14159;

boost::shared_ptr<SimPassiveSonarConfig> master_config;
SimPassiveSonar* SimPassiveSonar::inst_ = 0;
void* SimPassiveSonar::driver_library_handle_= 0;

int main(int argc, char * argv[])
{
    return goby::moos::run<SimPassiveSonar>(argc, argv);
}

SimPassiveSonar* SimPassiveSonar::get_instance()
{
    if(!inst_)    {
        master_config.reset(new SimPassiveSonarConfig);
        inst_ = new SimPassiveSonar(*master_config);
    }
    return inst_;
}

SimPassiveSonar::SimPassiveSonar(SimPassiveSonarConfig& cfg)
  : GobyMOOSApp(&cfg),
    cfg_(cfg),
    max_number_bounces(cfg_.max_number_bounces()),
    num_array_elements(cfg_.num_array_elements()),
    c_0(cfg_.svp_surface()),
    c_grad(cfg_.svp_gradient()),
    z_max(cfg_.max_rap_depth()),
    water_c(cfg_.svp_surface()),
    water_depth(cfg_.water_depth()),
    bottom_loss(cfg_.bottom_loss()),
    ray_acoustics(cfg_.ray_acoustics()),
    rap_acoustics(cfg_.rap_acoustics()),
    sampling_frequency(cfg_.sampling_frequency()),
    noise_level(cfg_.noise_level()),
    mseas_svp_updates(cfg_.mseas_svp_updates()), 
    mseas_update_interval(cfg_.mseas_update_interval()), 
    svp_range_resolution(cfg_.svp_range_resolution()), 
    svp_depth_resolution(cfg_.svp_depth_resolution()),
    base_dir_name(cfg_.base_dir_name()), 
    acoustic_filename_prefix(cfg_.acoustic_filename_prefix()), 
    acoustic_filename_suffix(cfg_.acoustic_filename_suffix()), 
    nonacoustic_filename_prefix(cfg_.nonacoustic_filename_prefix()), 
    nonacoustic_filename_suffix(cfg_.nonacoustic_filename_suffix()), 
    sync_path(cfg_.sync_path()), 
    sync_password(cfg_.sync_password())
{

  set_ignore_stale(false);
  
  // set up subscriptions before starting separate thread
  subscribe("AEL_HEADING", &SimPassiveSonar::handle_ael_heading, this);    
  subscribe("AEL_PITCH", &SimPassiveSonar::handle_ael_pitch, this);    
  subscribe("NAV_SPEED", &SimPassiveSonar::handle_nav_speed, this);    
  subscribe("ARRAY_X", &SimPassiveSonar::handle_array_x, this);    
  subscribe("ARRAY_Y", &SimPassiveSonar::handle_array_y, this);    
  subscribe("ARRAY_Z", &SimPassiveSonar::handle_array_z, this);    
  subscribe("ARRAY_DIR_X", &SimPassiveSonar::handle_array_dir_x, this);    
  subscribe("ARRAY_DIR_Y", &SimPassiveSonar::handle_array_dir_y, this);    
  subscribe("ARRAY_DIR_Z", &SimPassiveSonar::handle_array_dir_z, this);    
  subscribe("ARRAY_HEADING", &SimPassiveSonar::handle_array_heading, this);    
  subscribe("ARRAY_PITCH", &SimPassiveSonar::handle_array_pitch, this);    
  subscribe("TARGET_SIM_LIST", &SimPassiveSonar::handle_target_sim_list, this);    
  subscribe("AMBIENT_NL_DEPTH", &SimPassiveSonar::handle_ambient_nl_depth, this);    
  subscribe("NOISE_VERT_DIR", &SimPassiveSonar::handle_noise_vert_dir, this);    
  subscribe("COVARIANCE_FILE", &SimPassiveSonar::handle_covariance_file, this);    
  subscribe("SVP_RESPONSE", &SimPassiveSonar::handle_svp_response, this);    
  subscribe("VEHICLE_NAME", &SimPassiveSonar::handle_vehicle_name, this);    
  subscribe("SOUND_SPEED", &SimPassiveSonar::handle_sound_speed, this);    
  subscribe("SOUND_SPEED_GRAM", &SimPassiveSonar::handle_sound_speed, this);    
  subscribe("LAMSS_PINGER_COMMAND_IN", &SimPassiveSonar::handle_pinger_command, this);    

  //
  glog.is(VERBOSE) && glog << cfg_.DebugString() << std::endl;
  //
  // Initialize
  //
  
  // intialize MOOSGeodesy
  
  m_Geodesy.Initialise(cfg_.common().lat_origin(),cfg_.common().lon_origin());

  // Clear ray buffer
  eig.clear();
  
  source_status = false; // default to the source being off.
  
  time_last = 0;
  frame_number = 0;
  
  debug = false;
  debug2 = false;

  // Initialize
  t_control = false;
  multi_target = false;
  number_of_targets = 0;
  raytracing_interval = 30;
  vsa = false;
  ael_x = 0;
  ael_y = 0;
  ael_z = 0;
  source_bb_spl = 0;
  source_bb_amp = 0;
  source_bb_freq = 0;
  source_bb_bw = 0;
  icnt_rot = 0;
  ok_array_heading = false;
  ok_array_pitch = false;
  ok_towx = false;
  ok_towy = false;
  ok_towz = false;
  ok_hydx = false;
  ok_hydy = false;
  ok_hydz = false;
  got_array_info = false;
  ok_dir_x = false;
  ok_dir_y = false;
  ok_dir_z = false;
  got_array_dir_info = false;
  ok_pinger_x = false;
  ok_pinger_y = false;
  ok_source_bb_spl = false;
  ok_source_bb_freq = false;
  ok_source_bb_bw = false;
  got_source_bb_info = false;
  sync = false; 
  sound_speed = cfg_.svp_surface();
  // setup the random number generator
  gsl_rng_type_1 = gsl_rng_default;
  gsl_rng_1 = gsl_rng_alloc (gsl_rng_type_1);
  //  gsl_rng_set(gsl_rng_1, (long unsigned int) MOOSTime());
  gsl_rng_set(gsl_rng_1, (long unsigned int) 1);
  gsl_rng_tgt.clear();

  start_time = MOOSTime();
  frame_number = 0;
  noise_profile = false;
  noise_direct = false;
  //Flag indicating whether to use 3D Isotropic Ambient Noise Model. Default false
  ISOTROPIC=false;

  // random number seed
  iseed = -123456;
  array_prev_pitch = -1000;

  minimum_frequency = 0;
  maximum_frequency = 1e20;
  noise_cov_built = false;
  noise_std = pow(10.0, noise_level/20.0)*pow(10.0, -6.0)*sqrt(sampling_frequency*0.5);
  ownship = "me";
 
  // If active pinger exists, create acoustic contact
  // Must be moved to handle_pinger_command

  if (cfg_.array_type() == SimPassiveSonarConfig::ARRAY_TYPE_VSA)
    {
      array_type = "VSA";
      vsa = true;
    }
  else if (cfg_.array_type() == SimPassiveSonarConfig::ARRAY_TYPE_DURIP)
    {
      array_type = "DURIP";
      vsa = false;
    }
  else
    {
      array_type = "DURIP";
      vsa = false;
    }
  glog.is(VERBOSE) && glog << "Array Type: " << array_type << std::endl;
  
  glog.is(VERBOSE) && glog << "num_array_elements = " << num_array_elements << std::endl;
  // Initialize some vectors now that the we know the number of elements
  array_x.assign(num_array_elements,0);
  array_y.assign(num_array_elements,0);
  array_z.assign(num_array_elements,0);
  array_heading.assign(num_array_elements,0);
  array_pitch.assign(num_array_elements,0);
  
  hydrophone_gain = pow(10.0, cfg_.hydrophone_gain()/20.0);
  accel_gain = pow(10.0, cfg_.accelerometer_gain()/20.0);
  noise_std = pow(10.0, cfg_.noise_level()/20.0)*pow(10.0, -6.0)
    *sqrt(sampling_frequency*0.5);

  // Sampling
  num_samples = cfg_.num_samples();
  file_interval_time = double(num_samples)/sampling_frequency;
  // Find an fft length (round to next highest power of 2)
  unsigned int const v = num_samples; 
  // Put the result here. (So v=3 -> r=4; v=8 -> r=8)
  float f = (float)(v - 1);  
  nfft = 1 << ((*(unsigned int*)(&f) >> 23) - 126);
  glog.is(VERBOSE) && glog << "nfft = " << nfft << std::endl;
  // Build the dft of the signal in a format similar to 
  // matlab (same as gsl), described in
  // http://www.gnu.org/software/gsl/manual/html_node/Overview-of-complex-data-FFTs.html
  num_freqs = nfft/2 + 1; 
  glog.is(VERBOSE) && glog << "Each file represents about " << file_interval_time
			   << "seconds of data" << std::endl;
  
  // Now that we have some parameters, allocate some matricies
  sum_of_timeseries = gsl_matrix_calloc (num_array_elements,num_samples);
  if (vsa)
    {
      //Vector components
      sum_of_timeseries_x = gsl_matrix_calloc (num_array_elements,num_samples);
      sum_of_timeseries_y = gsl_matrix_calloc (num_array_elements,num_samples);
      sum_of_timeseries_z = gsl_matrix_calloc (num_array_elements,num_samples);
    }
  sync = (sync_path != "");
  svp_requests.clear();
  slow_loop_period = mseas_update_interval; 

}

void SimPassiveSonar::handle_sound_speed(const CMOOSMsg& msg)
{
  sound_speed = msg.m_dfVal;
}

void SimPassiveSonar::handle_ael_heading(const CMOOSMsg& msg)
{
  ael_heading = msg.m_dfVal;
  ael_yaw = -ael_heading*pi/180.0;
}

void SimPassiveSonar::handle_ael_pitch(const CMOOSMsg& msg)
{
  ael_pitch = msg.m_dfVal;
}

void SimPassiveSonar::handle_nav_speed(const CMOOSMsg& msg)
{
  ael_speed = msg.m_dfVal;
}

void SimPassiveSonar::handle_array_x(const CMOOSMsg& msg)
{
  string s_hydrophone_x = msg.m_sVal;
  int nx=str2dbl_vec(s_hydrophone_x, array_x_rel);
  if (nx != num_array_elements)
    {
      glog << "Array inconsistency ARRAY_X - FAIL" << std::endl;
      exit(EXIT_FAILURE);
    }	
  ael_x = vecUtils.mean(array_x_rel);
  ok_hydx = true;
}

void SimPassiveSonar::handle_array_y(const CMOOSMsg& msg)
{
  string s_hydrophone_y = msg.m_sVal;
  int ny=str2dbl_vec(s_hydrophone_y, array_y_rel);
  if (ny != num_array_elements)
    {
      glog << "Array inconsistency ARRAY_Y - FAIL" << std::endl;
      exit(EXIT_FAILURE);
    }	
  ael_y = vecUtils.mean(array_y_rel);
  ok_hydy = true;
}

void SimPassiveSonar::handle_array_z(const CMOOSMsg& msg)
{
  string s_hydrophone_z = msg.m_sVal;
  int nz=str2dbl_vec(s_hydrophone_z, array_z_rel);
  if (nz != num_array_elements)
    {
      glog << "Array inconsistency ARRAY_Z - FAIL" << std::endl;
      exit(EXIT_FAILURE);
    }	
  ael_z = vecUtils.mean(array_z_rel);
  ok_hydz = true;
}

void SimPassiveSonar::handle_array_dir_x(const CMOOSMsg& msg)
{
  string s_dir_x = msg.m_sVal;
  int nx=str2dbl_vec(s_dir_x, array_dir_x);
  if (nx != num_array_elements)
    {
      glog << "Array inconsistency ARRAY_DIR_X - FAIL" << std::endl;
      exit(EXIT_FAILURE);
    }	
  ok_dir_x = true;
}

void SimPassiveSonar::handle_array_dir_y(const CMOOSMsg& msg)
{
  string s_dir_y = msg.m_sVal;
  int ny=str2dbl_vec(s_dir_y, array_dir_y);
  if (ny != num_array_elements)
    {
      glog << "Array inconsistency ARRAY_DIR_Y - FAIL" << std::endl;
      exit(EXIT_FAILURE);
    }	
  ok_dir_y = true;
}

void SimPassiveSonar::handle_array_dir_z(const CMOOSMsg& msg)
{
  string s_dir_z = msg.m_sVal;
  int nz=str2dbl_vec(s_dir_z, array_dir_z);
  if (nz != num_array_elements)
    {
      glog << "Array inconsistency ARRAY_DIR_Z - FAIL" << std::endl;
      exit(EXIT_FAILURE);
    }	
  ok_dir_z = true;
}

void SimPassiveSonar::handle_array_heading(const CMOOSMsg& msg)
{
  string s_array_heading = msg.m_sVal;
  int nz=str2dbl_vec(s_array_heading, array_heading);
  if (nz != num_array_elements)
    {
      glog << "Array inconsistency ARRAY_HEADING - FAIL" << std::endl;
      exit(EXIT_FAILURE);
    }
  ok_array_heading = true;
}

void SimPassiveSonar::handle_array_pitch(const CMOOSMsg& msg)
{
  string s_array_pitch = msg.m_sVal;
  int nz=str2dbl_vec(s_array_pitch, array_pitch);
  if (nz != num_array_elements)
    {
      glog << "Array inconsistency ARRAY_HEADING - FAIL" << std::endl;
      exit(EXIT_FAILURE);
    }
  double sum = 0;
  for (int i = 0; i< nz; i++)
    sum += array_pitch[i];
  array_avg_pitch = sum/nz;
  ok_array_pitch = true;
}

void SimPassiveSonar::handle_ambient_nl_depth(const CMOOSMsg& msg)
{
  // noise profile
  noise_profile = parseNoiseProfile(msg.m_sVal, 
					  depth_nl,
					  noise_db);
}

void SimPassiveSonar::handle_noise_vert_dir(const CMOOSMsg& msg)
{
	// noise profile. Disabled until debugged

  glog.is(VERBOSE) && glog << "Received NOISE_VERT_DIR..." << endl;
  
  noise_direct = parseNoiseDirectionality(msg.m_sVal, theta_deg, S_dB);
  
  if (noise_direct)
    glog.is(VERBOSE) && glog << "noise_direct=true" << endl;
  else
    glog.is(VERBOSE) && glog << "noise_direct=false" << endl;

}

void SimPassiveSonar::handle_covariance_file(const CMOOSMsg& msg)
{
  //	if (noise_profile && noise_direct && noise_cov_built)
  if (noise_direct && noise_cov_built)
    {
      string covmat_file = msg.m_sVal;
      double t_req;
      glog.is(VERBOSE) && glog << "Reading Covariance" << endl;
      read_covmat_file(covmat_file, noise_level, t_req);
      glog.is(VERBOSE) && glog << "Got Covariance, delay = " << MOOSTime()-t_req
			       << " seconds" << endl;
    }
  else
    glog.is(VERBOSE) && glog << "Noise covariance received, but not used" << endl; 
}

void SimPassiveSonar::handle_svp_response(const CMOOSMsg& msg)
{
  handleSVPResponse(msg);
}

void SimPassiveSonar::handle_vehicle_name(const CMOOSMsg& msg)
{
  ownship = msg.m_sVal;
}

void SimPassiveSonar::handle_target_sim_list(const CMOOSMsg& msg)
{
  multi_target = true;
  vector<int> old_targets;
  old_targets.clear();
  
  for (int i=0; i< m_target_number.size(); i++)
    old_targets.push_back(m_target_number[i]);
  int old_number_of_targets = old_targets.size();
  
  number_of_targets = str2int_vec(msg.m_sVal,m_target_number);
  glog.is(VERBOSE) && glog << "Number of targets = " << number_of_targets << endl;
  
  CTarget Empty;
  if (number_of_targets != old_number_of_targets )
    {
      Targets.clear();
      ok_target_info.clear();
      
      for (int i = 0; i< number_of_targets; i++)
	{
	  glog.is(VERBOSE) && glog << "Target TGT_" << m_target_number[i] << endl;
	  Targets.push_back(Empty);
	  ok_target_info.push_back(false);
	  Targets[i].target_number = m_target_number[i];
	}
      glog.is(DEBUG1) && glog << "Done resetting " << endl;
    }
  else
    glog.is(DEBUG1) && glog << "No resetting " << endl;
  
  for (int i = 0; i< number_of_targets; i++)
    {
      bool register_done = false;
      for (int j=0; j< old_targets.size() && !register_done ; j++)
	register_done =  (old_targets[j] == m_target_number[i]);
      
      if ( !register_done )
	{
	  string target_id = Targets[i].target_id();
	  
	  string var =target_id+"_SIM_STATE";
	  //	  m_Comms.Register(var,0);
	  subscribe(var, &SimPassiveSonar::handle_target_sim_state, this);    

	  glog.is(DEBUG1) && glog << "Variable " << var << " registered" << endl;
	  glog.is(DEBUG1) && glog << "Target_id " << target_id << " registered" << endl;
	  
	  // Initialize random number generator with seed=target_number
	  gsl_rng_tgt.push_back(gsl_rng_alloc (gsl_rng_type_1));
	  gsl_rng_set(gsl_rng_tgt[gsl_rng_tgt.size()-1], (long unsigned int) m_target_number[i]);
	  
	  if (ray_acoustics)
	    {
	      
	      string acoustic_contact = Targets[i].target_id() 
		+"_SIM";
	      
	      var = acoustic_contact + "_ARR_FILE";
	      //	      m_Comms.Register(var,0);
	      subscribe(var, &SimPassiveSonar::handle_target_arrival_file, this);    

	      glog.is(DEBUG1) && glog << "Variable " << var << " registered" << endl;
	      
	      CEigenRayTable buffer;
	      buffer.clear();
	      putAcousticContact(Targets[i].target_id(),
				 Targets[i].target_x,
				 Targets[i].target_y,
				 Targets[i].target_dep,
				 buffer);
	      glog.is(DEBUG1) && glog << "Target " << Targets[i].target_id()
				       << " assigned acoustic contact number "
				       << findAcousticContact(Targets[i].target_id()) << std::endl;
	    }
	}
    }
  slow_loop_period = mseas_update_interval/(number_of_targets+1);
}

void SimPassiveSonar::handle_target_sim_state(const CMOOSMsg& msg)
{
  for (int i = 0; i<number_of_targets; i++)
    {
      string target_id = Targets[i].target_id();;
      glog.is(DEBUG2) && glog << "Looking for sim state of " << target_id << endl;
      
      string var =target_id+"_SIM_STATE";
      if(msg.m_sKey == var)
	{
	  vector<double> values;
	  int num_val = str2dbl_vec(msg.m_sVal,values);
	  int tgt_num = int(values[0]);
	  if (tgt_num != Targets[i].target_number)
	    {
	      glog << "Target number inconsistency - Aborting " << endl;
		    exit(EXIT_FAILURE);
	    }
	  bool tgt_on = (int(values[1]) == 1);
	  for (int j = 0; j < num_val-2;j++)
	    values[j]=values[j+2];
	  values.pop_back();
	  values.pop_back();
	  Targets[i].setTargetValues(tgt_num, tgt_on, values);
	  // Targets[i].printTargetValues();
	  //MOOSTrace("got %s\n", msg.m_sKey.c_str());
	  // if first target info request svp from mseas
	  int cn_num = findAcousticContact(Targets[i].target_id());
	  if ( mseas_svp_updates && 
	       (MOOSTime() - acoustic_contacts[cn_num].last_svp_request >= 
		mseas_update_interval ))
	    {
	      requestSVPUpdate(acoustic_contacts[cn_num].contact,
			       cn_num);
	    }
	  ok_target_info[i] = true;
	}
    }
}

void SimPassiveSonar::handle_target_arrival_file(const CMOOSMsg& msg)
{
  for (int i = 0; i<number_of_targets; i++)
    {
      string target_id = Targets[i].target_id();;
      glog.is(DEBUG2) && glog << "Looking for arr file for " << target_id << endl;
      
      string arr_file =target_id+"_SIM_ARR_FILE";
      
      if (msg.m_sKey == arr_file)
	{
	  eigenray_table.readEigenRayTable(msg.m_sVal, max_number_bounces);
	  putAcousticContact(target_id, 
			     Targets[i].target_x,
			     Targets[i].target_y,
			     Targets[i].target_dep,
			     eigenray_table);
	}	
    }
}

void SimPassiveSonar::handle_pinger_arrival_file(const CMOOSMsg& msg)
{
      glog.is(DEBUG2) && glog << "Looking for arr file for " << pinger_id << endl;
      
      string arr_file =pinger_id+"_SIM_ARR_FILE";
      
      if (msg.m_sKey == arr_file)
	{
	  eigenray_table.readEigenRayTable(msg.m_sVal, max_number_bounces);
	  putAcousticContact(pinger_id, 
			     pinger_x,
			     pinger_y,
			     pinger_depth,
			     eigenray_table);
	}	
}

void SimPassiveSonar::loop()
{

  double curr_time = MOOSTime();
  
  got_array_info = (ok_hydx && ok_hydy 
		    && ok_hydz);
  
  got_array_dir_info = (ok_dir_x && ok_dir_y && ok_dir_z);
  
  if (got_array_dir_info)
    {
      array_dir.resize(num_array_elements);
      // set directionality logic
      for (unsigned int i = 0; i < num_array_elements; i++)
	array_dir[i] = (pow(array_dir_x[i],2)
			+pow(array_dir_y[i],2)
			+pow(array_dir_z[i],2) > 1e-6);
    }  
  else
    {
      array_dir.resize(num_array_elements);
      for (unsigned int i = 0; i < num_array_elements; i++)
	array_dir[i] = false;
    }
  
  double t_now = MOOSTime();
  // Note that this timing scheme isn't perfect, but the processing chain
  // should be robust enough to handle it.
  if ( (t_now - time_last) > file_interval_time)
    {
      if (time_last < 1)
	timestamp = floor(t_now);
      else
	timestamp = timestamp + file_interval_time;

      time_last = timestamp;

      if (pinger_enabled)
	getPingerInfo();
      else if (multi_target && number_of_targets >0)
	getTargetInfo(0);
      else
	{
	  got_source_bb_info = (ok_source_bb_spl && ok_source_bb_freq && 
				ok_source_bb_bw);
	  //	  got_source_loc = (ok_pinger_x && ok_pinger_y);
	  got_source_loc = pinger_enabled;
	}
      
      // Output error messages 
      if(!got_array_info)
	{
	  glog.is(VERBOSE) && glog << "ARRAY_[X, Y or Z] *not* in the MOOSDB. - Waiting" << endl;
	  return;
	}
      else
	{
	  // If we have the array's information, generate timeseries and write files 
	  //Set all timeseries matricies to zero.
	  gsl_matrix_set_zero(sum_of_timeseries); 
	  
	  if (vsa)
	    {
	      gsl_matrix_set_zero(sum_of_timeseries_x);
	      gsl_matrix_set_zero(sum_of_timeseries_y); 
	      gsl_matrix_set_zero(sum_of_timeseries_z); 
	    }
	  // calculate the hydrophone positions every iteration in case tow position
	  // and relative hydrophone locations are published asychronously.
	  for (int i=0; i < num_array_elements; i++)
	    {
	      array_x.at(i) = array_x_rel.at(i);
	      array_y.at(i) = array_y_rel.at(i);
	      array_z.at(i) = array_z_rel.at(i);
	    }
	  
	  // get the noise level and covariances if available
	  
	  double nl = getAmbientNoise(-ael_z, 
				      depth_nl,noise_db); 
	  glog.is(VERBOSE) && glog << "Depth: " << -ael_z 
				   << " NL: " << nl << endl;
	  //noise_std = pow(10.0, nl/20.0)*pow(10.0, -6.0);
	  //Correction AJP 3 NOV 2010 (Generate background noise with the correct spectral level.)
	  noise_std = pow(10.0, nl/20.0)*pow(10.0, -6.0)*sqrt(sampling_frequency*0.5); 	  
	  
	  // initialize spectra
	  initialize_spectrum();
	  
	  // generate corrrelated  noise spectrum
	  generate_corr_noise_spectrum();
	  
	  // Passive targets are added in the spectral domain
	  for (int i=0; i<number_of_targets; i++)
	    {
	      glog.is(VERBOSE) && glog << "Handling target " << i <<endl;
	      
	      bool ok_tgt_info = getTargetInfo(i);
	      if (!ok_tgt_info)
		{
		  glog.is(VERBOSE) && glog << "Waiting for info for TGT_"
					   << m_target_number[i] << endl;
		  return;
		}
	      
	      glog.is(VERBOSE) && glog << "Got info for TGT_" << m_target_number[i]
				       << endl;;
	      int cn_num = findAcousticContact(Targets[i].target_id());
	      if (generate_eigenray_table(cn_num,acoustic_contacts[cn_num]))
		{
		  generate_signal_spectrum(i);  
		  glog.is(VERBOSE) && glog << "Target signal spectrum done -  target "
					   << Targets[i].target_id() << endl;
		  icnt_rot = int( (t_now-start_time)/7 )%number_of_targets;
		  if(got_source_loc && i == icnt_rot)
		    {
		      // Publish simulated target position rotating through targets
		      m_Comms.Notify("TARGET_XPOS",source_pos_x);
		      m_Comms.Notify("TARGET_YPOS",source_pos_y);
		    }
		}

	    }
	  
	  // here we add the PINGER timeseries in time domain
	  if (pinger_enabled && pinger_active)
	    {
       	      glog.is(VERBOSE) && glog << "Handling Pinger " <<endl;
	  
	      bool ok_pinger_info = getPingerInfo();
	      
	      if (!ok_pinger_info)
		{
		  glog.is(VERBOSE) && glog << "Waiting for info for PINGER"
					   << endl;
		  return;
		}
	      glog.is(VERBOSE) && glog << "Got info for PINGER" 
				       << endl;;
	      if (add_pinger_timeseries())
		{
		  glog.is(VERBOSE) && glog << "Added PINGER timeseries" 
				       << endl;;
		}
	    }
	  else
	    {
	      save_spectral_timeseries();
	    }

	  glog.is(VERBOSE) && glog << "Writing data files " << endl;
	  write_nas_file(timestamp);
	  write_nas_file_verbose(timestamp);
	  write_hyd_file();
	  write_eigenray_file();
	  m_Comms.Notify("SONAR_DIR", base_dir_name);
	  m_Comms.Notify("SONAR_FILE_NUM", double(frame_number));
	  
	  frame_number++;
	}
    }
  
  // process some events at a slower rate
  //const int slow_loop_period = 10;
  if(curr_time > last_slow_loop_t_ + slow_loop_period)
    slow_loop(curr_time);
}

void SimPassiveSonar::slow_loop(double curr_time)
{
  // Here we should add slow loop actions.
  // request SVP from queue: last in first out!
  if (svp_requests.size() > 0)
    {
      m_Comms.Notify("SVP_REQUEST", svp_requests[svp_requests.size()-1]);
      svp_requests.pop_back();
    }  

  last_slow_loop_t_ = curr_time;
}

bool SimPassiveSonar::generate_eigenray_table(int cn_num, 
					      CAcousticContact& acoustic_contact)
{
  bool eigenrays_ready = false;

  double range_x = (ael_x - source_pos_x);
  double range_y = (ael_y - source_pos_y);
  double range_req = sqrt(range_x*range_x + range_y*range_y);
  double depth_req = -ael_z;
	      
  if (!ray_acoustics)
    {
      range_ref = range_req;
      depth_ref = depth_req;
      water_c = c_0 + depth_ref * c_grad;
      ph_shift_ref = 0;
      if (rap_acoustics)
	{
	  Rap.setEnv(c_0, c_grad, water_depth, z_max);
	  Rap.rapRay(ael_x, ael_y, depth_ref,
		     source_pos_x, source_pos_y, source_dep,
		     dip_ref, p_ref, tau_ref , turn_ref);
	  tl_est = - 20.0 * log10(p_ref);
	  if (turn_ref)
	    ph_shift_ref = M_PI/2;
		      
	  double z_rap, r_rap;
	  Rap.rapDepth(source_pos_x, source_pos_y, source_dep, 
		       ael_x, ael_y, z_rap);
	  Rap.rapRange(ael_x, ael_y, depth_ref, 
		       source_dep, dip_ref, r_rap);
	}
      else
	{
	  tau_ref = range_ref/c_0;
	  double delta_k = bottom_loss/8686;
	  double atten = exp(-delta_k*range_ref);
	  p_ref = atten * (1/water_depth) * (1/sqrt(range_ref)); 
	  tl_est = - 20.0 * log10(p_ref);
	  // for waveguide acoustics, assume horizontal propagation.
	  dip_ref = 0;
	}
		  
      CRay ray;
      ray.amp = p_ref;
      ray.phase = ph_shift_ref;
      ray.tau = tau_ref;
      ray.dip_source = 0;
      ray.dip_receiver = - dip_ref *M_PI/180.0;
      ray.surface_bounces = 0;
      ray.bottom_bounces = 0;
      eigenray_table.putEigenRay(source_dep, depth_ref,source_bb_freq, range_ref,ray);
      putAcousticContact(acoustic_contact.contact,
			 source_pos_x,
			 source_pos_y,
			 source_dep,
			 eigenray_table);
    }
  // first get the ray table for this contact
  if (getAcousticContact(acoustic_contact.contact,eigenray_table))
    {
      CEigenRay eig;
      // Get the closest eigenray bundle
      eigenray_table.getEigenRays(range_req, depth_req, range_ref, depth_ref, eig);
      if (ray_acoustics)
	{
	  glog.is(VERBOSE) && glog << "Source range = " << range_req
				   << " Raytracing range = " << eig.range
				   << endl;
	  glog.is(VERBOSE) && glog << "Receiver depth = " << depth_req
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
		      
	  if (fabs(range_req - acoustic_contact.last_range) > raytracing_interval)
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
	      requestRayTrace(acoustic_contact.contact, 
			      range_min, range_max, num_ranges, 
			      depth_min, depth_max, num_depths,
			      source_bb_freq, cn_num);
	    }
	  // if mseas_svp_updates, request SVP
	  if ( mseas_svp_updates && 
	       (MOOSTime() - acoustic_contact.last_svp_request >= 
		mseas_update_interval ) )
	    {
	      requestSVPUpdate(acoustic_contact.contact,
			       cn_num);
	    }
	}
      eigenrays_ready = true;
    }
  else if (ray_acoustics && (MOOSTime() - acoustic_contact.last_request > 10))
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
      requestRayTrace(acoustic_contact.contact, 
		      range_min, range_max, num_ranges, 
		      depth_min, depth_max, num_depths,
		      source_bb_freq, cn_num);
		  
      // if mseas_svp_updates, request SVP
      if (mseas_svp_updates &&
	  (MOOSTime() - acoustic_contact.last_svp_request >= 
	   mseas_update_interval) )
	{
	  requestSVPUpdate(acoustic_contact.contact,
			   cn_num);
	}
		  
    }

  return(eigenrays_ready);

}

bool SimPassiveSonar::defineRayTracingBox(CAcousticContact& target,
					   double& r_min,
					   double& r_max,
					   double& dep_min,
					   double& dep_max,
					   int& num_r,
					   int& num_dep)
{
  if (got_source_loc && got_array_info)
    {
      r_min = 1e8;
      r_max = -1e8;
      dep_min = 1e8;
      dep_max = -1e8;
      double spacing=1e8;
      // array element loop
      for (int i=0; i < num_array_elements; i++)
	{
	  double range_x = (array_x.at(i) - source_pos_x);
	  double range_y = (array_y.at(i) - source_pos_y);
	  double range = sqrt(range_x*range_x + range_y*range_y) ;
	  double depth = - array_z.at(i); 
	  r_min = std::min(range,r_min);
	  r_max = std::max(range,r_max);
	  dep_min = std::min(depth,dep_min);
	  dep_max = std::max(depth,dep_max);
	  for (int j=0; j < i; j++)
	    {
	      double spacing_ij = sqrt(pow(array_x.at(j)-array_x.at(i),2)
				      + pow(array_y.at(j)-array_y.at(i),2)
				       + pow(array_z.at(j)-array_z.at(i),2));
	      if (spacing_ij > 0)
		spacing = std::min(spacing_ij,spacing);
	      else
		glog.is(VERBOSE) && glog << "Warning: array elements " 
					 << i << " and " << j 
					 << " are co-located" << std::endl;
	    }
	}
      double range = 0.5*(r_min+r_max);
      if (target.last_rate <= 0)
	r_min -= 2*raytracing_interval;
      if (target.last_rate >= 0)
	r_max += 2*raytracing_interval;
      //      r_min += target.last_rate * (MOOSTime()-target.last_rate_update);
      //      r_max += target.last_rate * (MOOSTime()-target.last_rate_update);

      // Raytracing resolution less critical for horizontal variability 
      num_r = int((r_max-r_min)/(10*spacing)) + 3;
      num_dep = 2*int((dep_max-dep_min)/(2*spacing)) + 3;
      glog.is(DEBUG1) && glog << "Ray Tracing box: rmin: " << r_min 
			      << " r_max: " << r_max
			      << " spacing: " << spacing
			      << " num ranges: " << num_r
			      << ", num depths: " << num_dep << std::endl;
      double delta_t = MOOSTime() - target.last_rate_update; 
      if (target.last_rate_update > 0)
	{
	  target.last_rate = (range-target.last_range)/delta_t;	    
	  glog.is(DEBUG1) && glog << endl << ">>> rate_update= " << target.last_rate << endl; 
	  glog.is(DEBUG1) && glog  << ">>> delta_t = " << delta_t << endl << endl ;
	}
      else
	target.last_rate = 0;
      target.last_range = range;
      target.last_rate_update = MOOSTime();
      return(true);
    }
  return(false);
}

bool SimPassiveSonar::generate_signal_spectrum(int tgt_count)
{
  bool debug = false;
  if (got_source_loc && source_status && got_source_bb_info)
    {
      double f_upper = source_bb_freq + source_bb_bw/2;
      double f_lower = source_bb_freq - source_bb_bw/2;
      
      // limit f_upper and f_lower if it's something weird. 
      if (f_upper > sampling_frequency/2)
	f_upper = sampling_frequency/2;
      if (f_lower < 0 )
	f_lower = 0;
      
      int f_lo = int(f_lower*nfft/sampling_frequency);
      int f_up = int(f_upper*nfft/sampling_frequency);

      vector<double> source_spect_mag(num_freqs,0); 
      vector<double> source_spect_phase(num_freqs,0); 
      
      source_spect_mag.at(0) = 0;
      source_spect_phase.at(0) = 0;
      
      for(int f_iter=0; f_iter < num_freqs; f_iter++)
	{
	  double f = f_iter/(nfft/sampling_frequency);
	  if (f <= f_upper && f >= f_lower)
	    {
	      // Scale by nfft, because of the definition of gsl's inverse fft.
	      //     	      source_spect_mag.at(f_iter) = source_bb_amp*nfft; 
	      // I don't think that's right should be sqrt(nfft/f_s). HS
	      // source_spect_mag.at(f_iter) = source_bb_amp*sqrt((double) nfft/sampling_frequency);
	      // or 
	      //source_spect_mag.at(f_iter) = source_bb_amp*sqrt((double) nfft); 

	      //Correction AJP 3 NOV 2010 (Generate broadband source with the correct spectral level.)
  	      source_spect_mag.at(f_iter) = source_bb_amp*sqrt(((double) nfft)*sampling_frequency*0.5); 
	      source_spect_phase.at(f_iter) = 0;
	    }
	  else
	    {
	      source_spect_mag.at(f_iter) = 0;
	      source_spect_phase.at(f_iter) = 0;
	    }
	  
	  //  Add a random phase for all freq componants except f=0, and f=nyquist
	  // (whose freq componant must have a phase of zero if the time domain 
	  // signal is to be real).
	  if(f_iter != 0 && f_iter != num_freqs-1)
	    {
	      //	      source_spect_phase.at(f_iter) = 2*M_PI*gsl_rng_uniform(gsl_rng_1);
	      source_spect_phase.at(f_iter) = 2*M_PI*gsl_rng_uniform(gsl_rng_tgt[tgt_count]);
	    }
	}
      
      // Now we must now take the source spectrum and add the appropriate phase 
      // These fourier_coeff matricies are the spectrums for each hydrophone. 
      

      // Snapshot starting time for tone continuity
      double t0 =  frame_number*num_samples/sampling_frequency;
      double delta_f = sampling_frequency/nfft;
      double tone_fac = nfft*0.5;

      // array element loop
      for (int phone_iter=0; phone_iter < num_array_elements; phone_iter++)
	{
	  // Project absolute vs response onto physical orientation
	  double deg2rad = M_PI/180;
	  double hd_ang = array_heading.at(phone_iter)*deg2rad;
	  double el_ang = array_pitch.at(phone_iter);
	  double vs_costh = cos(hd_ang);
	  double vs_sinth = sin(hd_ang);
	  double vs_cosel = cos(el_ang);
	  double vs_sinel = sin(el_ang);
      
	  double range_x = (array_x.at(phone_iter) - source_pos_x);
	  double range_y = (array_y.at(phone_iter) - source_pos_y);
	  double range = sqrt(range_x*range_x + range_y*range_y);

	  // Get eigenrays closesst to phone
	  double range_ref, depth_ref;
	  CEigenRay eig;

	  double depth_req = -array_z.at(phone_iter);
	  eigenray_table.getEigenRays(range, depth_req, 
				      range_ref, depth_ref, eig);


	  double range_ray = eig.range;
	  double depth_ray = eig.receiver_depth;
	  int num_rays     = eig.num_rays;
	  glog.is(DEBUG1) && glog << "Receiver " << phone_iter << endl;
	  glog.is(DEBUG1) && glog << "Target range= " << range
				  << " Raytracing range = " << eig.range << endl;
	  glog.is(DEBUG1) && glog << "Receiver depth= " << depth_req 
				  << " Raytracing depth = " << eig.receiver_depth
				  << endl;
	  glog.is(DEBUG1) && glog << "Number of eigenrays: " << eig.num_rays << endl;
	
          double delta_r = range - range_ray;
	  double delta_dep = - array_z.at(phone_iter) - depth_ray;  
	  double delta_z = - delta_dep;

	  // Computing vector components in absolute coordinates
	  double costh = cos(atan2(range_y, range_x));
	  double sinth = sin(atan2(range_y, range_x));

	  CSensor hyd;
	  CSensor vec_x;
	  CSensor vec_y;
	  CSensor vec_z;

	  hyd.initSpectrum(nfft);
	  if (vsa)
	    {
	      vec_x.initSpectrum(nfft);
	      vec_y.initSpectrum(nfft);
	      vec_z.initSpectrum(nfft);
	    }

	  for (int i_ray=0; i_ray<num_rays; i_ray++)
	    {
	      // Elevation angle projection
	      double dip_ray = eig.bundle[i_ray].dip_receiver;
	      double p_ray = eig.bundle[i_ray].amp;
	      double ph_ray = eig.bundle[i_ray].phase;
	      double tau_ray = eig.bundle[i_ray].tau;
	      double cosel = cos(dip_ray);
	      // Positive dip angle for downgoing wave => k_z positive
	      double sinel = - sin(dip_ray);
	      double rhow = 1000;
	      // double g = 9.806; //Removed g. AJP 11/16/2010
	      
	      double fac_dir = 1;
	      if (array_dir[phone_iter])
		{
		  fac_dir = costh*array_dir_x[phone_iter] 
		    + sinth*array_dir_y[phone_iter] 
		    + sinel*array_dir_z[phone_iter];
		  if (fac_dir < 0)
		    fac_dir = 0;
		}
	      // frequency loop
	      for (int f_iter=f_lo; f_iter <= f_up; f_iter++)
		{
		  double f = f_iter*delta_f;
		  double omega = 2*M_PI * f;
		  double k = omega/water_c;
		  double k_r = k * cosel;
		  double k_z = k * sinel;
		  
		  // double v_ref = k * p_ray/(rhow*g);
		  double v_ref = k * p_ray/(rhow); //Removed g. AJP 11/16/2010
		  double x_ref = v_ref * costh*cosel;
		  double y_ref = v_ref * sinth*cosel;
		  double z_ref = v_ref * sinel;
		  
		  double phase_pres = ph_ray + k_r*delta_r + k_z*delta_z + tau_ray*omega;
		  //double phase_vec = phase_pres + M_PI_2;
		  double phase_vec = phase_pres - M_PI_2; //Changed phase from +M_PI_2 to -M_PI_2. AJP 11/16/2010
		  
		  double p_mag = p_ray * source_spect_mag.at(f_iter);
		  double p_pha = source_spect_phase.at(f_iter) -phase_pres;
		  
		  double x_mag = x_ref * source_spect_mag.at(f_iter);
		  double x_pha = source_spect_phase.at(f_iter) -phase_vec;
		  
		  double y_mag = y_ref * source_spect_mag.at(f_iter);
		  double y_pha = source_spect_phase.at(f_iter) -phase_vec;
		  
		  double z_mag = z_ref * source_spect_mag.at(f_iter);
		  double z_pha = source_spect_phase.at(f_iter) -phase_vec;
		  
		  
		  hyd.spectrum[f_iter]        += fac_dir * p_mag * cos(p_pha);
		  hyd.spectrum[nfft-f_iter]   += fac_dir * p_mag * sin(p_pha);
		  if (vsa)
		    {
		      // vector components
		      vec_x.spectrum[f_iter]      += x_mag * cos(x_pha);
		      vec_x.spectrum[nfft-f_iter] += x_mag * sin(x_pha);
		      vec_y.spectrum[f_iter]      += y_mag * cos(y_pha);
		      vec_y.spectrum[nfft-f_iter] += y_mag * sin(y_pha);
		      vec_z.spectrum[f_iter]      += z_mag * cos(z_pha);
		      vec_z.spectrum[nfft-f_iter] += z_mag * sin(z_pha);
		    }
		  // Add tone if in current bin
		  for (int tone_iter=0; tone_iter < tone_freqs.size(); tone_iter++)
		    {
		      if (int(tone_freqs[tone_iter]/delta_f) == f_iter)
			{
			  p_mag = p_ray * tone_amps_Pa[tone_iter]*tone_fac;
			  p_pha = - (omega * t0) - phase_pres;
			  if (phone_iter == 0 && i_ray == 0)
			    {
			      glog.is(DEBUG2) && glog << "adding tone f= " 
						      << tone_freqs[tone_iter] 
						      << " ,level=" << 20*log10(p_mag)
						      << endl;
			    }
			  if (vsa)
			    {
			      x_mag = x_ref * tone_amps_Pa[tone_iter]*tone_fac;
			      x_pha = -(omega * t0) - phase_vec;
			      y_mag = y_ref * tone_amps_Pa[tone_iter]*tone_fac;
			      y_pha = -(omega * t0) - phase_vec;
			      z_mag = z_ref * tone_amps_Pa[tone_iter]*tone_fac;
			      z_pha = -(omega * t0) - phase_vec;
			    }
			  hyd.spectrum[f_iter]        += p_mag * cos(p_pha);
			  hyd.spectrum[nfft-f_iter]   += p_mag * sin(p_pha);
			  if (vsa)
			    {
			      // vector components
			      vec_x.spectrum[f_iter]      += x_mag * cos(x_pha);
			      vec_x.spectrum[nfft-f_iter] += x_mag * sin(x_pha);
			      vec_y.spectrum[f_iter]      += y_mag * cos(y_pha);
			      vec_y.spectrum[nfft-f_iter] += y_mag * sin(y_pha);
			      vec_z.spectrum[f_iter]      += z_mag * cos(z_pha);
			      vec_z.spectrum[nfft-f_iter] += z_mag * sin(z_pha);
			    }
			}
		    }
		}
	    }
// Add to sensor class arrays
	  hydrophones[phone_iter].addToSpectrum(hyd.getSpectrum());
	  if (vsa)
	    {
	      for (int f_iter=f_lo; f_iter <= f_up; f_iter++)
		{
		  // projection of vector components onto physical orientations
		  double sig_x = vec_x.spectrum[f_iter];
		  double sig_y = vec_y.spectrum[f_iter];
		  double sig_z = vec_z.spectrum[f_iter];
		  
		  vec_x.spectrum[f_iter]= sig_x*vs_costh - sig_y*vs_sinth;
		  vec_y.spectrum[f_iter]= (sig_x*vs_sinth + sig_y*vs_costh)*vs_cosel 
		    + sig_z*vs_sinel;
		  vec_z.spectrum[f_iter]= sig_z*vs_cosel 
		    - (sig_x*vs_sinth + sig_y*vs_costh)*vs_sinel;
		  
		  sig_x = vec_x.spectrum[nfft-f_iter];
		  sig_y = vec_y.spectrum[nfft-f_iter];
		  sig_z = vec_z.spectrum[nfft-f_iter];
		  
		  vec_x.spectrum[nfft-f_iter]= sig_x*vs_costh - sig_y*vs_sinth;
		  vec_y.spectrum[nfft-f_iter]= (sig_x*vs_sinth + sig_y*vs_costh)*vs_cosel 
		    + sig_z*vs_sinel;
		  vec_z.spectrum[nfft-f_iter]= sig_z*vs_cosel 
		    - (sig_x*vs_sinth + sig_y*vs_costh)*vs_sinel;
		}	  
	      x_accelerometers[phone_iter].addToSpectrum(vec_x.getSpectrum());
	      y_accelerometers[phone_iter].addToSpectrum(vec_y.getSpectrum());
	      z_accelerometers[phone_iter].addToSpectrum(vec_z.getSpectrum());
	    }
	}
    }
  
  return true;
}

void SimPassiveSonar::array_ray_arrivals(std::vector<std::vector<double> > & delays,
					 std::vector<std::vector<double> > & amplitudes,
					 std::vector<std::vector<double> > & doppler)
{
  if (got_source_loc && source_status)
    {
      // Snapshot starting time for tone continuity
      double t0 =  frame_number*num_samples/sampling_frequency;
      double delta_f = sampling_frequency/nfft;
      double tone_fac = nfft*0.5;
      delays.resize(num_array_elements);
      amplitudes.resize(num_array_elements);
      doppler.resize(num_array_elements);

     // Save eigenray tables for IMP file
	  array_ray_table.clear();

       // array element loop
      for (int phone_iter=0; phone_iter < num_array_elements; phone_iter++)
	{
	  // Project absolute vs response onto physical orientation
	  double deg2rad = M_PI/180;
	  double hd_ang = array_heading.at(phone_iter)*deg2rad;
	  double el_ang = array_pitch.at(phone_iter);
	  double vs_costh = cos(hd_ang);
	  double vs_sinth = sin(hd_ang);
	  double vs_cosel = cos(el_ang);
	  double vs_sinel = sin(el_ang);
      
	  double range_x = (array_x.at(phone_iter) - source_pos_x);
	  double range_y = (array_y.at(phone_iter) - source_pos_y);
	  double range = sqrt(range_x*range_x + range_y*range_y);

	  // Get eigenrays closesst to phone
	  double range_ref, depth_ref;
	  CEigenRay eig;

	  double depth_req = -array_z.at(phone_iter);
	  eigenray_table.getEigenRays(range, depth_req, 
				      range_ref, depth_ref, eig);
	  double range_ray = eig.range;
	  double depth_ray = eig.receiver_depth;
	  int num_rays     = eig.num_rays;
	  glog.is(DEBUG1) && glog << "Receiver " << phone_iter << endl;
	  glog.is(DEBUG1) && glog << "Source range= " << range
				  << " Raytracing range = " << eig.range << endl;
	  glog.is(DEBUG1) && glog << "Receiver depth= " << depth_req 
				  << " Raytracing depth = " << eig.receiver_depth
				  << endl;
	  glog.is(DEBUG1) && glog << "Number of eigenrays: " << eig.num_rays << endl;
	
	  // push array eigenray table. Delay adjusted later.

	  array_ray_table.push_back(eig);


          double delta_r = range - range_ray;
	  double delta_dep = - array_z.at(phone_iter) - depth_ray;  
	  double delta_z = - delta_dep;

	  // Computing vector components in absolute coordinates
	  double costh = cos(atan2(range_y, range_x));
	  double sinth = sin(atan2(range_y, range_x));
	  // Get eigenray delays for sensor
	  std::vector<double> ray_delays;
	  std::vector<double> ray_amplitudes;
	  std::vector<double> rcv_ray_cos;
	  std::vector<double> src_ray_cos;
	  ray_arrivals(eig, 
                       range,
		       depth_req,
		       src_ray_cos,
		       rcv_ray_cos,
		       ray_delays, 
		       ray_amplitudes); 
	  // doppler_corrections
	  double vr_S;
	  double vr_R;
	  ray_doppler_vel(source_spd, (90.0-source_hdg)*M_PI/180.0, 0.0,
			   ael_speed, ael_yaw+M_PI/2, -ael_pitch,
			   vr_S,vr_R);
	  std::vector<double> ray_doppler;
	  ray_doppler.resize(eig.num_rays);
	  double oneossp = 1/sound_speed;

	  for (int i_ray=0; i_ray < eig.num_rays; i_ray++)
	    {
	      ray_delays[i_ray] = ray_delays[i_ray]*
		(1.0+vr_R*rcv_ray_cos[i_ray]*oneossp);
	      ray_doppler[i_ray] = 1 + (src_ray_cos[i_ray]*vr_S - 
					rcv_ray_cos[i_ray]*vr_R)*oneossp;
	  // adjust array eigenray delay
	      array_ray_table[phone_iter].bundle[i_ray].tau = ray_delays[i_ray]; 
	
	    }
	  delays[phone_iter] = ray_delays;
	  amplitudes[phone_iter] = ray_amplitudes;
	  doppler[phone_iter] = ray_doppler;
	}
    }
}

bool SimPassiveSonar::ray_arrivals(CEigenRay eig, 
				   double rcv_range,
				   double rcv_depth, 
				   std::vector<double> & cos_Src,
				   std::vector<double> & cos_Rcv,
				   std::vector<double> & delays, 
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
  if (num_eig_rays>0)
    {
      // Vertical ray angles
      for(int i_ray=0; i_ray<num_eig_rays; i_ray++)
	{ cos_Src[i_ray] = cos(eig.bundle[i_ray].dip_source);
	  cos_Rcv[i_ray] = cos(eig.bundle[i_ray].dip_receiver);
	  sin_Rcv[i_ray] = sin(-eig.bundle[i_ray].dip_receiver);
	  // free surface reflection handled by change of sign
	  if(abs(abs(eig.bundle[i_ray].phase)-M_PI)<=0.1) 
	    amp[i_ray] = -eig.bundle[i_ray].amp;
	  else
	    amp[i_ray] = eig.bundle[i_ray].amp;
	}
      
      double ref_range = eig.range;
      double ref_depth = eig.receiver_depth;

      double dz = rcv_depth - ref_depth;
      double dr = rcv_range - ref_range;
      double oneossp = 1/sound_speed;
      delays.resize(num_eig_rays);
      amplitudes.resize(num_eig_rays);
      for(int ray_iter=0; ray_iter<num_eig_rays; ray_iter++)
	{
	      delays[ray_iter] = (eig.bundle[ray_iter].tau + 
				  (((dr*cos_Rcv[ray_iter]) + 
				    (dz*sin_Rcv[ray_iter]))*
				   oneossp));
	      amplitudes[ray_iter] = amp[ray_iter];    
	}
      return(true);
    }
  return(false);
}

void SimPassiveSonar::ray_doppler_vel(double v_S, double a_S,double e_S, double v_R, double a_R, double e_R, double& vr_S, double& vr_R )
{
  //
  // Computes the projected doppler velocities for an eigenray
  //
  double dt=0;

  double vx_S=v_S*cos(e_S)*cos(a_S);
  double vy_S=v_S*cos(e_S)*sin(a_S);
  double xp_S=source_pos_x+vx_S*dt;
  double yp_S=source_pos_y+vy_S*dt;
  // and current receiver locations
  double vx_R=v_R*cos(e_R)*cos(a_R);
  double vy_R=v_R*cos(e_R)*sin(a_R);
  double xp_R=ael_x+vx_R*dt;
  double yp_R=ael_y+vy_R*dt;
  
  // projected relative velocities
  double dist_TR = sqrt(pow((xp_R - xp_S),2) +pow((yp_R - yp_S),2));
  vr_S = ((vx_S*(xp_R - xp_S)) + (vy_S*(yp_R - yp_S)))/dist_TR;
  vr_R = ((vx_R*(xp_R - xp_S)) + (vy_R*(yp_R - yp_S)))/dist_TR;
}

//function to create Waveform and replica files
void SimPassiveSonar::generate_doppler_waveform(std::vector<double>& source_waveform,
double dop_fac)
{
  // doppler_fac > 1 diverging src/rcv, < 1 converging
  //--first create time vector
  double dop_pulsewidth = dop_fac*ping_pulsewidth;
  std::vector<double> t_src;
  int num_samples = int(dop_pulsewidth*sampling_frequency);
  t_src.resize(num_samples); 
  double delta_t = 1.0/sampling_frequency;
  for(int k=0; k < num_samples; k++)
    t_src[k] = k*delta_t;

  std::vector<double> source_window;
  if (pinger_command.replica_window() == LAMSS_PINGER_COMMAND::WINDOW_HANN)
    {
      source_window = vecUtils.hann(t_src.size());
    }
  else if (pinger_command.replica_window() == LAMSS_PINGER_COMMAND::WINDOW_TUKEY)
    {
      double raised_cosine_ratio;
      source_window = vecUtils.tukey(t_src.size(),raised_cosine_ratio);
    }
  else
    {
      source_window.clear();
      for (int k=0; k<t_src.size(); k++)
	{
          source_window.push_back(1);
        }
    }

  //--create waveform depending on type ---
  source_waveform.clear();
  double source_strength = pow(10.0,pinger_db/20.0);
  glog.is(DEBUG2) && glog << "source strength in Pa: " << source_strength << endl;
  glog.is(DEBUG2) && glog << "t_src.size() : " << t_src.size() << endl;

  source_waveform.resize(t_src.size());

  double dop_center_frequency = ping_center_frequency/dop_fac;
  double dop_bandwidth = ping_bandwidth/dop_fac;

  if (pinger_command.waveform() == LAMSS_PINGER_COMMAND::WAVEFORM_CW)
    {
      glog.is(DEBUG1) && glog <<"Generating CW waveform" << endl;
      for( int k=0;k<t_src.size(); k++)
	{
	  source_waveform[k] = sin(2*M_PI*dop_center_frequency*t_src[k])*
				    source_strength*source_window[k];
	}
    }
  else if (pinger_command.waveform() == LAMSS_PINGER_COMMAND::WAVEFORM_LFM)
    {
      glog.is(DEBUG1) && glog << "Generating LFM waveform" << endl;
      double max_lfm_frequency = dop_center_frequency + 0.5* dop_bandwidth;
      double min_lfm_frequency = dop_center_frequency - 0.5* dop_bandwidth;
      for( int k=0;k<t_src.size();k++)
	{ 
	  double K = (max_lfm_frequency-min_lfm_frequency)/dop_pulsewidth;
	  source_waveform[k] = sin(2*M_PI*t_src[k]*(min_lfm_frequency+(K*t_src[k]*0.5)))*
	    source_strength*source_window[k];
	}
    }
  else if (pinger_command.waveform() == LAMSS_PINGER_COMMAND::WAVEFORM_HFM)
    {
      double dop_max_hfm_frequency = dop_center_frequency + 0.5* dop_bandwidth;
      double dop_min_hfm_frequency = dop_center_frequency - 0.5* dop_bandwidth;
      double r = (dop_max_hfm_frequency-dop_min_hfm_frequency)/dop_pulsewidth;
      double p1 = dop_min_hfm_frequency*dop_max_hfm_frequency/r;
      for( int k=0;k<t_src.size();k++)
	{ 
	  source_waveform[k] = sin(2*PI*(-p1*log(1-r/dop_max_hfm_frequency*t_src[k])))*
				 source_strength*source_window[k];
	}
    }
  else if (pinger_command.waveform() == LAMSS_PINGER_COMMAND::WAVEFORM_IMP)
    {
      double dop_max_hfm_frequency = dop_center_frequency + 0.5* dop_bandwidth;
      double dop_min_hfm_frequency = dop_center_frequency - 0.5* dop_bandwidth;
      double a = 0.5*dop_bandwidth;
      for( int k=0;k<t_src.size();k++)
	{ 
	  double w;
	  if (k==0)
	    w = 2*a;
	  //        w = 1;
	  else
	    w = sin(2*PI*a*t_src[k])/(PI*t_src[k]);
	  //	    w = sin(2*PI*a*t_src[k])/(2*PI*a*t_src[k]);
	  
	  source_waveform[k] = w*cos(2*PI*(dop_center_frequency*t_src[k]))*
	    source_strength;
	}
    }
  else
    {
      // Here we need to read the waveform from file and stretch by dop_fac
    }
}

bool SimPassiveSonar::generate_corr_noise_spectrum()
{
  int sensors_per_element;
  int num_sensors;
  if (vsa) 
    sensors_per_element = 4;
  else
    sensors_per_element = 1;
  num_sensors = sensors_per_element*num_array_elements;
  
  // set dc covarince to 0.0

  vector<complex<double> > noise_realization = vecUtils.zzeros(num_sensors,1);
  
  glog.is(VERBOSE) && glog << "Realization generation started, num covars="
			   << noise_covar.size() << endl;
  for (int f_iter=1; f_iter < num_freqs; f_iter++)
    {
             
      vector<complex<double> > realization;
      noise_covar[f_iter].realization(realization);
      
      for (int sensor_iter=0; sensor_iter< num_sensors; sensor_iter++)
	{
	    noise_realization.push_back(realization[sensor_iter]);
	}
    }
  glog.is(DEBUG1) && glog << "Realizations done" << endl;
 
  // array element loop
  for (int elem_iter=0; elem_iter < num_array_elements; elem_iter++)
    {
      // Project absolute vs response onto physical orientation
      double deg2rad = M_PI/180;
      double hd_ang = array_heading.at(elem_iter)*deg2rad;
      double el_ang = array_pitch.at(elem_iter);
      double vs_costh = cos(hd_ang);
      double vs_sinth = sin(hd_ang);
      double vs_cosel = cos(el_ang);
      double vs_sinel = sin(el_ang);

      CSensor hyd;
      CSensor vec_x;
      CSensor vec_y;
      CSensor vec_z;
      
      hyd.initSpectrum(nfft);
      if (vsa)
	{
	  vec_x.initSpectrum(nfft);
	  vec_y.initSpectrum(nfft);
	  vec_z.initSpectrum(nfft);
	}
      
      for (int f_iter=1; f_iter < num_freqs; f_iter++)
	{
	  int indx = elem_iter*sensors_per_element + num_sensors * f_iter;
	  hyd.spectrum[f_iter]        = noise_realization[indx].real();
	  hyd.spectrum[nfft-f_iter]   = noise_realization[indx].imag();
	  if (vsa )
	    {
	      // x component
	      indx = elem_iter*sensors_per_element + 1 + num_sensors * f_iter;
	      vec_x.spectrum[f_iter]        = noise_realization[indx].real();
	      vec_x.spectrum[nfft-f_iter]   = noise_realization[indx].imag();
	      // y component
	      indx = elem_iter*sensors_per_element + 2 + num_sensors * f_iter;
	      vec_y.spectrum[f_iter]        = noise_realization[indx].real();
	      vec_y.spectrum[nfft-f_iter]   = noise_realization[indx].imag();
	      // z component
	      indx = elem_iter*sensors_per_element + 3 + num_sensors * f_iter;
	      vec_z.spectrum[f_iter]        = noise_realization[indx].real();
	      vec_z.spectrum[nfft-f_iter]   = noise_realization[indx].imag();

	    }
	  // projection of vector components onto physical orientations
	  // For noise only vertical rotation. Already in relative azimuth coordinates
	  if (vsa)
	    {
	      double sig_x = vec_x.spectrum[f_iter];
	      double sig_y = vec_y.spectrum[f_iter];
	      double sig_z = vec_z.spectrum[f_iter];
	      
	      vec_x.spectrum[f_iter]= sig_x;
	      vec_y.spectrum[f_iter]= sig_y*vs_cosel + sig_z*vs_sinel;
	      vec_z.spectrum[f_iter]= sig_z*vs_cosel - sig_y*vs_sinel;
	      sig_x = vec_x.spectrum[nfft-f_iter];
	      sig_y = vec_y.spectrum[nfft-f_iter];
	      sig_z = vec_z.spectrum[nfft-f_iter];
	      
	      vec_x.spectrum[nfft-f_iter]= sig_x;
	      vec_y.spectrum[nfft-f_iter]= sig_y*vs_cosel + sig_z*vs_sinel;
	      vec_z.spectrum[nfft-f_iter]= sig_z*vs_cosel - sig_y*vs_sinel;
	    }
	}
      
      // Add to sensor class arrays
      hydrophones[elem_iter].addToSpectrum(hyd.getSpectrum());
      if (vsa)
	{
	  x_accelerometers[elem_iter].addToSpectrum(vec_x.getSpectrum());
	  y_accelerometers[elem_iter].addToSpectrum(vec_y.getSpectrum());
	  z_accelerometers[elem_iter].addToSpectrum(vec_z.getSpectrum());
	}
    }
  return true;
}

bool SimPassiveSonar::add_pinger_timeseries()
{
  int cn_num = findAcousticContact(pinger_id);
  if (generate_eigenray_table(cn_num,acoustic_contacts[cn_num]))
    {
      // Here goes the timeseries generation for pinger
      // First compute all ray delays, amplitudes and doppler
      std::vector<vector<double> > arr_delays;
      std::vector<vector<double> > arr_amplitudes;
      std::vector<vector<double> > arr_doppler;
      array_ray_arrivals(arr_delays,
			 arr_amplitudes,
			 arr_doppler);
      // Here goes the merging of the waveform response (Kristen)
      // Global variables:
      // vector<double> waveform: contains waveform updated 
      // when new pinger_command is received
      // timestamp: UTC of current file start
      // pingtime: UTC of ping transmission.
      // sum_of_timeseries(num_array_elements,num_samples): Total hydrophone timeseries
      double max_delay = 0;
      double file_duration = num_samples/sampling_frequency;
      for (int i_elem=0; i_elem < arr_delays.size(); i_elem++)
	for (int i_ray=0; i_ray < arr_delays[i_elem].size(); i_ray++)
	  max_delay = std::max(max_delay,arr_delays[i_elem][i_ray]);
      
      if ((pingtime+ping_pulsewidth >= timestamp) &&
	  (max_delay+pingtime < timestamp + file_duration))
	{ 
	  for (int i_elem=0; i_elem < arr_delays.size(); i_elem++)
	    {
	      std::vector<double> ts_hyd = hydrophones[i_elem].timeseries();
	      for (int i_ray=0; i_ray < arr_delays[i_elem].size(); i_ray++)
		{
		  // doppler scaled waveform, sampled at sampling_frequency
		  generate_doppler_waveform(waveform, 
					    arr_doppler[i_elem][i_ray]);
		  // add to sum_timeseries if inside current time frame.
		  double t1 = pingtime + arr_delays[i_elem][i_ray] - 
		    timestamp;
		  int indx_1 = t1*sampling_frequency;
		  int indx_2 = indx_1 + waveform.size()-1;
		  if (indx_1 < num_samples && indx_2 >=0)
		    {
		      int indx_offset = -indx_1;
		      indx_1 = std::max(indx_1,0);
		      indx_2 = std::min(indx_2,num_samples-1);
		      for (int i=indx_1; i<=indx_2; i++)
			ts_hyd[i] += waveform[i+indx_offset];
		    }
		}
	      for(int i_samp=0; i_samp < num_samples; i_samp++)
		{
		  gsl_matrix_set(sum_of_timeseries, i_elem, i_samp, 
				 ts_hyd[i_samp]);
		}
	    }
	}
      return(true);
    }
  else
    return(false);
}

bool SimPassiveSonar::save_spectral_timeseries()
{
  // array element loop
  for (int elem_iter=0; elem_iter < num_array_elements; elem_iter++)
    {
      vector<double> ts_hyd;
      vector<double> ts_x;
      vector<double> ts_y;
      vector<double> ts_z;

      ts_hyd = hydrophones[elem_iter].timeseries();
      if (vsa)
	{
	  ts_x = x_accelerometers[elem_iter].timeseries();
	  ts_y = y_accelerometers[elem_iter].timeseries();
	  ts_z = z_accelerometers[elem_iter].timeseries();
	}
  
      for(int sample_iter=0; sample_iter < num_samples; sample_iter++)
	{
	  gsl_matrix_set(sum_of_timeseries, elem_iter, sample_iter, 
		     ts_hyd[sample_iter]);
	  if (vsa)
	    {
	      gsl_matrix_set(sum_of_timeseries_x, elem_iter, sample_iter, 
			     ts_x[sample_iter]);
	      gsl_matrix_set(sum_of_timeseries_y, elem_iter, sample_iter, 
			     ts_y[sample_iter]);
	      gsl_matrix_set(sum_of_timeseries_z, elem_iter, sample_iter, 
			     ts_z[sample_iter]);
	    }
	}
    }
  
  return true;
}

bool SimPassiveSonar::initialize_spectrum()
{

  if (hydrophones.size() == num_array_elements && hydrophones[0].spectrum.size() == nfft)
    {
      for (int elem_iter=0; elem_iter < num_array_elements; elem_iter++)
	{
	  hydrophones[elem_iter].clearSpectrum();
	  if (vsa)
	    {
	      x_accelerometers[elem_iter].clearSpectrum();
	      y_accelerometers[elem_iter].clearSpectrum();
	      z_accelerometers[elem_iter].clearSpectrum();
	    }
	}
    }
  else
    { 
      // array element loop
      CSensor Empty;
      Empty.initSpectrum(nfft);
      hydrophones.clear();
      if (vsa)
	{
	  x_accelerometers.clear();
	  y_accelerometers.clear();
	  z_accelerometers.clear();
	}
      for (int elem_iter=0; elem_iter < num_array_elements; elem_iter++)
	{
	  hydrophones.push_back(Empty);
	  if (vsa)
	    {
	      x_accelerometers.push_back(Empty);
	      y_accelerometers.push_back(Empty);
	      z_accelerometers.push_back(Empty);
	    }
	}
    }
  return(true);
}

bool SimPassiveSonar::print_vec(vector<double> temp)
{
  for (int i=0; i < temp.size(); i++)
    glog.is(DEBUG2) && glog << "i:" << i << " temp(i):" << temp.at(i) << endl;

  //  MOOSTrace("--done--\n");
  return(true);
}

bool SimPassiveSonar::print_array(double a[], int l)
{
  for (int i=0; i < l; i++)
    glog.is(DEBUG2) && glog << "i:" << i << " a(i):" << a[i] << endl;
  //    MOOSTrace("%02d: %f\n",i, a[i]);

  //  MOOSTrace("--done--\n");
  return(true);
}


bool SimPassiveSonar::write_hyd_file()
{
  double deg2rad = M_PI/180;
  double rad2deg = 180.0/M_PI;
  char acoustic_filename_buff[1000]; 
  char hydrophone_filename_buff[1000]; 

  std::string acoustic_filename;
  // Sure hope nobody wants a filename longer than 1000 chars
  sprintf(acoustic_filename_buff,"%s/%s%09d%s",base_dir_name.c_str(),
	  acoustic_filename_prefix.c_str(),frame_number,
	  acoustic_filename_suffix.c_str()); 
  sprintf(hydrophone_filename_buff,"%s/HYD%09d%s",base_dir_name.c_str(),
	  frame_number, acoustic_filename_suffix.c_str()); 

  // Got this filename stuff from CpBearingTrack
  
  FILE * data_file;
  if ( (data_file = fopen(acoustic_filename_buff, "wb")) == NULL)  
    {
      glog << "Unable to open a file: " <<  acoustic_filename_buff << endl;
      glog << "Does the directory exist?" << endl;
      glog << "Does base_dir_name begin and end with a forward slash?" << endl;
      glog << "Do you have permission to write to that directory?" << endl;
      exit(EXIT_FAILURE);
    }
  
  else
    {
      //      if(array_type == "VSA")
      //	{
      //	  unsigned long zero = 0; // Timestamp is set to zero. 
      //	  fwrite(&zero,sizeof(unsigned long),1,data_file);
      //	  fwrite(&zero,sizeof(unsigned long),1,data_file);
      //	}
      
      // Now write all of the acoustic data
      
      // I think this can be made faster by writing the entire time series 
      // with one fwrite. I think gsl has a function to do that.

      FILE * hydrophone_file;
      if (array_type == "VSA")
	{
	  // Hydrophone only file
	  hydrophone_file = fopen(hydrophone_filename_buff, "wb");  
	}
      for (int phone_iter=0; phone_iter < num_array_elements; phone_iter++)
	{  
	  // First the hydrophone timeseries
	  for (int sample_iter=0; sample_iter < num_samples; sample_iter++)
	    {
	      float val = float(hydrophone_gain*
				gsl_matrix_get(sum_of_timeseries,phone_iter, sample_iter));
	      fwrite(&val, sizeof(float), 1 , data_file);
	      if (array_type == "VSA")
		{
		  // first write pressures to hydrophones only file
		  fwrite(&val, sizeof(float), 1 , hydrophone_file);
		}
	    }
	  if (array_type == "VSA")
	    {
	      vector<float> val_x, val_y, val_z;
	      val_x.reserve(num_samples);
	      val_y.reserve(num_samples);
	      val_z.reserve(num_samples);
	      
	      for (int sample_iter=0; sample_iter < num_samples; sample_iter++)
		{
		  double sig_x = accel_gain*gsl_matrix_get(sum_of_timeseries_x,
								 phone_iter, sample_iter);
		  double sig_y = accel_gain*gsl_matrix_get(sum_of_timeseries_y,
								  phone_iter, sample_iter);
		  double sig_z = accel_gain*gsl_matrix_get(sum_of_timeseries_z,
								  phone_iter, sample_iter);
		  val_x.push_back(float(sig_x));
		  val_y.push_back(float(sig_y));
		  val_z.push_back(float(sig_z));
		}

	      for (int sample_iter=0; sample_iter < num_samples; sample_iter++)
		{
		  fwrite(&val_x[sample_iter], sizeof(float), 1 , data_file);
		}	
	      for (int sample_iter=0; sample_iter < num_samples; sample_iter++)
		{
		  fwrite(&val_y[sample_iter], sizeof(float), 1 , data_file);
		}
	      for (int sample_iter=0; sample_iter < num_samples; sample_iter++)
		{
		  fwrite(&val_z[sample_iter], sizeof(float), 1 , data_file);
		}
	    }
	}
      if (array_type == "VSA")
	fclose (hydrophone_file);
    }
  
  fclose (data_file);
  
  glog.is(VERBOSE) && glog << "Wrote file: " << acoustic_filename_buff << endl;
 
  // The following was added by HS 111102 to eliminate the need for
  // the frame_count.inp file and iDAS in simulation, 
  // and preparing for compatibility with consolidated acquisition 
  // software by tes for DURIP array etc.   
  //

  // Now transfer file to client if on separate host
  if (sync)
    {
      /*
	std::string acoustic_filename(acoustic_filename_buff);
	std::string cmd = "scp " + acoustic_filename + " " + sync_path;
	glog.is(DEBUG1) && glog << "Synching with: " << cmd << endl;
	system(cmd.c_str());
	glog.is(VERBOSE) && glog << "Synched with: " << cmd << endl;
      */
      std::string sync_cmd = "bash -c 'RSYNC_PASSWORD=" + sync_password 
	+ " rsync "
	+ acoustic_filename_buff + " " + sync_path + "/'";
      glog.is(DEBUG1) && glog << "Synching with: " << sync_cmd.c_str() << endl;
      system(sync_cmd.c_str());
    }

  m_Comms.Notify("SONAR_FILENAME",acoustic_filename_buff);
/*
  char frame_filename_buff[1000]; 
  sprintf(frame_filename_buff,"%s/frame_count.inp",base_dir_name.c_str());
  
  // Write frame file
  FILE * frame_file;
  if ( (frame_file = fopen(frame_filename_buff, "wt")) == NULL)
    {
      MOOSTrace("Error opening frame_count file %s\n",frame_filename_buff);
    }
  else 
    {
      fprintf (frame_file, "%d\n",frame_number);
      fprintf (frame_file, "%s\n", base_dir_name.c_str());
      fprintf (frame_file, "%d\n", 0);
      fclose (frame_file);
    }
*/
  return true;
}

bool SimPassiveSonar::write_eigenray_file()
{
  char filename_buff[1000]; 

  std::string filename;
  // Sure hope nobody wants a filename longer than 1000 chars
  sprintf(filename_buff,"%s/%s%09d%s",base_dir_name.c_str(),
	  "RAY",frame_number,
	  acoustic_filename_suffix.c_str()); 
  
  FILE * data_file;
  if ( (data_file = fopen(filename_buff, "wb")) == NULL)  
    {
      glog << "Unable to open a file: " <<  filename_buff << endl;
      glog << "Does the directory exist?" << endl;
      glog << "Does base_dir_name begin and end with a forward slash?" << endl;
      glog << "Do you have permission to write to that directory?" << endl;
      exit(EXIT_FAILURE);
    }
  else
    {
      double timestamp = MOOSTime();
      fwrite(&timestamp, sizeof(double), 1 , data_file);
      int num_phones = array_ray_table.size();
      fwrite(&num_phones, sizeof(int), 1 , data_file);

      for (int i = 0; i<num_phones; i++)
	{
	  int num_rays = array_ray_table[i].num_rays;
	  fwrite(&num_rays, sizeof(int), 1 , data_file);
	  for (int j=0; j<num_rays; j++)
	    {
	      fwrite(&array_ray_table[i].bundle[j].tau, sizeof(double), 1 , data_file);
	      fwrite(&array_ray_table[i].bundle[j].amp, sizeof(double), 1 , data_file);
	      fwrite(&array_ray_table[i].bundle[j].phase, sizeof(double), 1 , data_file);
	      fwrite(&array_ray_table[i].bundle[j].surface_bounces, sizeof(int), 1 , data_file);
	      fwrite(&array_ray_table[i].bundle[j].bottom_bounces, sizeof(int), 1 , data_file);
	    }
	}
      fclose(data_file);
    }
  return(true);
}

bool SimPassiveSonar::write_nas_file(double timestamp)
{
  double deg2rad = M_PI/180;
  char nonacoustic_filename_buff[1000]; 
  // Sure hope nobody wants a filename longer than 1000 chars
  sprintf(nonacoustic_filename_buff,"%s/%s%09d%s",base_dir_name.c_str(),
	  nonacoustic_filename_prefix.c_str(),frame_number,
	  nonacoustic_filename_suffix.c_str()); 
  // Got this filename stuff from CpBearingTrack and SealabDataSim2.m
  
  FILE * nas_data_file;
  if ( (nas_data_file = fopen(nonacoustic_filename_buff, "wt")) == NULL)
    {
      glog << "Unable to open a file: " <<  nonacoustic_filename_buff << endl;
      glog << "Does the directory exist?" << endl;
      glog << "Does base_dir_name begin and end with a forward slash?" << endl;
      glog << "Do you have permission to write to that directory?" << endl;
      exit(EXIT_FAILURE);
    }
  else
    {
      // The following was copied from SealabDataSim2.m
      fprintf (nas_data_file, "%f\n", timestamp);              // Current time
      
      // The format string used to be "%f\n", but this results in some random
      // number being written to the non acoustic data file. Changed this to
      // "%d\n", now the number is written out alright. AHV
      fprintf (nas_data_file, "%d\n", frame_number);         // Frame_Number
      
      if(array_type == "VSA")
	{
	  fprintf (nas_data_file, "%d\n", num_array_elements);        // Number of NAD sensors
	}
      if(array_type == "DURIP")
	{
	  fprintf (nas_data_file, "2\n");        // Number of NAD sensors
	}
      
      fprintf (nas_data_file, "1\n");                           // Number of NAD frames (samples)/file
      fprintf (nas_data_file, "1\n");                           // Frame Time increment (sec)
      fprintf (nas_data_file, "1\n");                           // downsample factor
      fprintf (nas_data_file, "1\n");                           // Frame increment
      
      // Note that array_heading and array_pitch are initialized to zero, so that will be their values if 
      // the values from the MOOSDB have not yet been gotten.
      if(array_type == "VSA")
	{
	  vector<double> hdz (num_array_elements,0);  
	  vector<double> hdy (num_array_elements,0);  
	  vector<double> hdx (num_array_elements,0);  
	  vector<double> prz (num_array_elements,0);  
	  vector<double> pry (num_array_elements,0);  
	  vector<double> prx (num_array_elements,0);  
	  
	  for(int jj= 0; jj<num_array_elements; jj++)
	    { 
	      // For VSA, print out all sensors' info.
	      hdz.at(jj) = cos(array_pitch.at(jj)) * cos(array_heading.at(jj)*deg2rad) - 
		sin(array_pitch.at(jj));
	      hdy.at(jj) = sin (array_heading.at(jj)*deg2rad);
	      hdx.at(jj) = sin (array_pitch.at(jj)) * cos (array_heading.at(jj)*deg2rad) 
		+ cos (array_pitch.at(jj));
	      prz.at(jj) = -sin (array_pitch.at(jj));
	      pry.at(jj) = 0;
	      prx.at(jj) = -cos (array_pitch.at(jj));
	      
	      fprintf (nas_data_file, "%8.4f %8.4f %8.4f %8.4f %8.4f %8.4f 0.0 0.0 %8.4f\n",
		       hdx.at(jj), hdy.at(jj), hdz.at(jj), prx.at(jj), pry.at(jj), prz.at(jj), fabs(array_z.at(jj)));
	    }
	}
      
      if(array_type == "DURIP" || array_type == "HYD")
	{
	  // For DURIP, print out first and last sensors' info only.
	  fprintf (nas_data_file, "%8.4f %8.4f 0.0 %8.4f\n", array_heading.at(0), array_pitch.at(0), fabs(array_z.at(0)));
	  fprintf (nas_data_file, "%8.4f %8.4f 0.0 %8.4f\n", array_heading.at(num_array_elements-1), 
		   array_pitch.at(num_array_elements-1), fabs(array_z.at(num_array_elements-1)));


	  // Also, print out the uptime, and the x,y,z positions of the sensors
	  fprintf (nas_data_file, "%f\n", (timestamp-start_time));
	  for (int jj=0; jj < num_array_elements; jj++)
	    {
	      fprintf(nas_data_file, "%8.4f %8.4f %8.4f\n",
		      array_x.at(jj), array_y.at(jj), fabs(array_z.at(jj)));
	    }
	}
      
      fclose (nas_data_file);
      glog.is(DEBUG1) && glog << "Wrote file: " << nonacoustic_filename_buff << endl;
      if (sync)
	{

	  std::string nonacoustic_filename(nonacoustic_filename_buff);
	  std::string sync_cmd = "bash -c 'RSYNC_PASSWORD=" + sync_password 
	    + " rsync "
	    + nonacoustic_filename + " " + sync_path + "/'";
	  glog.is(DEBUG1) && glog << "Synching with: " << sync_cmd.c_str() << endl;
	  system(sync_cmd.c_str());
	}
    }
  
}


bool SimPassiveSonar::write_nas_file_verbose(double timestamp) {
	double deg2rad = M_PI / 180;
	char nonacoustic_filename_buff[1000];
	// Sure hope nobody wants a filename longer than 1000 chars
	sprintf(nonacoustic_filename_buff, "%s/%s_%s_%09d%s", base_dir_name.c_str(), nonacoustic_filename_prefix.c_str(),"verbose", frame_number, nonacoustic_filename_suffix.c_str());

	FILE *nas_data_file;
	if ((nas_data_file = fopen(nonacoustic_filename_buff, "wt")) == NULL) {
	  glog << "Unable to open a file: " <<  nonacoustic_filename_buff << endl;
	  glog << "Does the directory exist?" << endl;
	  glog << "Does base_dir_name begin and end with a forward slash?" << endl;
	  glog << "Do you have permission to write to that directory?" << endl;
	  exit(EXIT_FAILURE);
	} else {
		fprintf(nas_data_file, "Verbose uSimPassiveSonar non-acoustic data file\n");
		fprintf(nas_data_file, "\n--Data--\n");
		fprintf(nas_data_file, "%d \t\t! file number\n", frame_number); 
		fprintf(nas_data_file, "%8.4f ! current time\n", timestamp); 
		fprintf(nas_data_file, "%8.4f \t! array uptime\n", (timestamp - start_time));
		fprintf(nas_data_file, "%d \t\t! samples per file\n", num_samples);
		fprintf(nas_data_file, "%8.4f \t! sampling rate\n", sampling_frequency);		
		fprintf(nas_data_file, "%8.4f \t! file time interval (sec)\n", file_interval_time);
		fprintf(nas_data_file, "%8.8f %8.8f \t! Lat, Lon position of sim. origin\n", m_Geodesy.GetOriginLatitude(), m_Geodesy.GetOriginLongitude()); 
		
		fprintf(nas_data_file, "\n--Array--\n");
		if (array_type == "DURIP") {
			fprintf(nas_data_file, "2 \t! number of sensors\n");
		} else {
			fprintf(nas_data_file, "%d \t! number of array elements\n", num_array_elements);			
		}

		// VSA Heading and Pitch
		if (array_type == "VSA") {
			vector<double> hdz(num_array_elements, 0);
			vector<double> hdy(num_array_elements, 0);
			vector<double> hdx(num_array_elements, 0);
			vector<double> prz(num_array_elements, 0);
			vector<double> pry(num_array_elements, 0);
			vector<double> prx(num_array_elements, 0);

			for (int jj = 0; jj < num_array_elements; jj++) {
				hdz.at(jj) = cos(array_pitch.at(jj)) * cos(array_heading.at(jj) * deg2rad) - sin(array_pitch.at(jj));
				hdy.at(jj) = sin(array_heading.at(jj) * deg2rad);
				hdx.at(jj) = sin(array_pitch.at(jj)) * cos(array_heading.at(jj) * deg2rad) + cos(array_pitch.at(jj));
				prz.at(jj) = -sin(array_pitch.at(jj));
				pry.at(jj) = 0;
				prx.at(jj) = -cos(array_pitch.at(jj));

				fprintf(nas_data_file, "%8.4f %8.4f %8.4f %8.4f %8.4f %8.4f 0.0 0.0 %8.4f ! VSA element %03d: heading (x,y,z), pitch (x,y,z), depth\n", hdx.at(jj), hdy.at(jj), hdz.at(jj), prx.at(jj), pry.at(jj), prz.at(jj), fabs(array_z.at(jj)), jj);
			}
		}

		if (array_type == "DURIP") {
			// For DURIP, print out first and last sensors' info only.
			fprintf(nas_data_file, "%8.4f %8.4f 0.0 %8.4f \t! X,Y,Z position (m) of element %d\n", 
				array_heading.at(0), array_pitch.at(0), fabs(array_z.at(0)),num_array_elements-1);
			fprintf(nas_data_file, "%8.4f %8.4f 0.0 %8.4f \t! X,Y,Z position (m) of element %d\n", 
				array_heading.at(num_array_elements - 1), array_pitch.at(num_array_elements - 1), fabs(array_z.at(num_array_elements - 1)),num_array_elements-1);
		} else {
			for (int jj = 0; jj < num_array_elements; jj++) {
				double element_pos_lat, element_pos_lon;
				m_Geodesy.UTM2LatLong(array_x.at(jj), array_y.at(jj), element_pos_lat, element_pos_lon);
				fprintf(nas_data_file, "%8.8f %8.8f \t! Lat, Lon position of element %d\n", element_pos_lat, element_pos_lon, jj);				
				fprintf(nas_data_file, "%8.4f %8.4f %8.4f \t! X,Y,Z position (m) of element %d\n", 
					array_x.at(jj), array_y.at(jj), fabs(array_z.at(jj)), jj);
			}
		}
		
		// Targets
		fprintf(nas_data_file,"\n--Targets--\n");
		fprintf(nas_data_file,"%d \t!  Number of targets\n",number_of_targets);
		for (int i = 0; i < number_of_targets; i++) {
			if(getTargetInfo(i)) { // variables used here are set in that function call
				double source_pos_lat, source_pos_lon;
				m_Geodesy.UTM2LatLong(source_pos_x, source_pos_y, source_pos_lat, source_pos_lon);
				
				fprintf(nas_data_file,"%d \t!  on(1)/off(0) status of target %d\n", source_status, i);
				fprintf(nas_data_file,"%8.8f %8.8f \t! Lat, Long of target %d\n",source_pos_lat, source_pos_lon, i);					
				fprintf(nas_data_file,"%8.4f %8.4f %8.4f \t! X,Y,Z position (m) of target %d\n",source_pos_x, source_pos_y, source_dep,i);
				fprintf(nas_data_file,"%8.8f  \t! Broadband source amp. (Pa) of target %d\n",source_bb_amp,i);
				fprintf(nas_data_file,"%8.4f %8.4f \t! Broadband center freq and bandwidth of target %d\n",source_bb_freq, source_bb_bw,i);
				
				int num_tones = Targets[i].tone_freq.size();
				fprintf(nas_data_file,"%d \t! Number of tones of target %d\n",num_tones,i);
				for (int j = 0; j < num_tones; j++) {
					fprintf(nas_data_file,"%8.3f %8.3f %8.8f %8.8f \t! Tone %d of target %d: Freq, dB Amp., Wavenumber, Amp. (Pa)\n",
						tone_freqs[j],tone_amps[j],tone_ks[j],tone_amps_Pa[j],j,i);
				}	
			}
		}
		
		fclose(nas_data_file);
		glog.is(DEBUG1) && glog << "Wrote file: " << nonacoustic_filename_buff
					<< endl;
		
		if (sync) {


		  std::string nonacoustic_filename(nonacoustic_filename_buff);
		  std::string sync_cmd = "bash -c 'RSYNC_PASSWORD=" + sync_password 
		    + " rsync "
		    + nonacoustic_filename + " " + sync_path + "/'";
		  glog.is(DEBUG1) && glog << "Synching with: " << sync_cmd.c_str() << endl;
		  system(sync_cmd.c_str());
		}
	}
}


int SimPassiveSonar::str2int_vec(string msg, vector<int>& values)
{
  string bite;
  
  // MOOSTrace("string = %s\n",msg.c_str());
  values.clear();
  int nz = 0;
  while (msg.length() > 0)
    {
      bite = MOOSChomp(msg,",");
      values.push_back(atoi(bite.c_str()));
      nz++;
    }
  // MOOSTrace("number of values = %d\n",nz);
  
  return(nz);
}

bool SimPassiveSonar::getPingerInfo()
{
  if (pinger_enabled)
    {
      source_pos_x = pinger_x;
      source_pos_y = pinger_y;
      source_dep = pinger_depth;
      source_bb_amp = pow(10.0,pinger_db/20.0);
      got_source_bb_info = true;
      got_source_loc = true;
      source_status = true;
      return(true);
    }
  else
    {
      got_source_bb_info = false;
      got_source_loc = false;
      source_status = false;
      return(false);
    }
}

bool SimPassiveSonar::getTargetInfo(int i)
{
  //  MOOSTrace("entering getTargetInfo. i=%d\n",i);
  if (i>=0 && i< Targets.size() && ok_target_info[i])
    {
      source_status = Targets[i].target_on;
      source_pos_x = Targets[i].target_x;
      source_pos_y = Targets[i].target_y;
      source_dep = Targets[i].target_dep;
      source_bb_spl = Targets[i].target_spl;
      // 1e-6 factor removed for compatibility with active simulator
      //      source_bb_amp = 1e-6*pow(10.0,source_bb_spl/20.0);
      source_bb_amp = pow(10.0,source_bb_spl/20.0);
      glog.is(VERBOSE) && glog << "target: " << Targets[i].target_number
			       << ", spl: " << source_bb_spl
			       << ", amp: " << source_bb_amp << endl;
      source_bb_freq = Targets[i].target_freq;
      source_bb_bw = Targets[i].target_bw;
      // get tones
      tone_freqs.clear();
      tone_amps.clear();
      tone_ks.clear();
      tone_amps_Pa.clear();
      for (int j=0; j<Targets[i].tone_freq.size(); j++)
	{
	  tone_freqs.push_back(Targets[i].tone_freq[j]);
	  tone_amps.push_back(Targets[i].tone_dbl[j]);
	  tone_ks.push_back(2*M_PI*Targets[i].tone_freq[j]/water_c);
	  // 1e-6 factor removed for compatibility with active simulator
	  //          tone_amps_Pa.push_back(pow(10.0, Targets[i].tone_dbl[j]/20.0)*pow(10.0, -6.0));
          tone_amps_Pa.push_back(pow(10.0, Targets[i].tone_dbl[j]/20.0));
	}
      got_source_bb_info = true;
      got_source_loc = true;
      //     MOOSTrace("exiting getTargetInfo = true\n");
    }
  else
    {
      got_source_bb_info = false;
      got_source_loc = false;
      //      MOOSTrace("exiting getTargetInfo = false\n");
    }
      return(got_source_bb_info && got_source_loc);
}

bool SimPassiveSonar::putAcousticContact(std::string cn, 
					 double cn_x,
					 double cn_y,
					 double cn_depth,
					 CEigenRayTable eig_table)
{
  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      if (MOOSStrCmp(acoustic_contacts[i].contact,cn))
	{
	  acoustic_contacts[i].contact_x = cn_x;
	  acoustic_contacts[i].contact_y = cn_y;
	  acoustic_contacts[i].contact_depth = cn_depth;
	  acoustic_contacts[i].ray_table = eig_table;
	  acoustic_contacts[i].ready = true;
	  return(true); 
	}
    }
  // make new contact
  CAcousticContact new_contact;
  new_contact.contact = cn;
  new_contact.contact_x = cn_x;
  new_contact.contact_y = cn_y;
  new_contact.contact_depth = cn_depth;
  new_contact.ready = false;
  new_contact.ray_table = eig_table;
  new_contact.svp_ready = false;
  new_contact.last_svp_request = -1;
  new_contact.last_request = -1;
  new_contact.last_range = 0;
  new_contact.last_rate = 0;
  new_contact.last_rate_update = -1;
  acoustic_contacts.push_back(new_contact);
  glog.is(DEBUG1) && glog << "Contact " << cn << " eigenrays updated" << endl;
  glog.is(DEBUG1) && glog << "Number of ranges: " << eig_table.num_ranges << endl;
  glog.is(DEBUG1) && glog << "Number of depths: " << eig_table.num_depths << endl;
  return(true);
}

int SimPassiveSonar::findAcousticContact(std::string cn)
{
  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      if (MOOSStrCmp(acoustic_contacts[i].contact,cn))
	{
	  return(i); 
	}
    }
  // not found, something wrong, return -1
  return(-1);
}

bool SimPassiveSonar::putAcousticContactSVP(SVP::Response svp_response)
{
  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      if (svp_response.request_id() == acoustic_contacts[i].svp_request_id)
	{
	  acoustic_contacts[i].svp_response = svp_response;
	  acoustic_contacts[i].svp_ready = true;
	  return(true); 
	}
    }
  return(false);
}

bool SimPassiveSonar::getAcousticContact(std::string cn, CEigenRayTable& eig_table )
{
  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      if (MOOSStrCmp(acoustic_contacts[i].contact,cn))
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

bool SimPassiveSonar::getAcousticContactSVP(std::string cn, 
					     SVP::Response& svp_response )
{
  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      if (MOOSStrCmp(acoustic_contacts[i].contact,cn))
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

bool SimPassiveSonar::readyAcousticContact(std::string cn)
{
  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      if (MOOSStrCmp(acoustic_contacts[i].contact,cn))
	{
	  return(acoustic_contacts[i].ready);
	}
    }
  return(false);
}

bool SimPassiveSonar::readyAcousticContactSVP(std::string cn)
{
  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      if (MOOSStrCmp(acoustic_contacts[i].contact,cn))
	{
	  return(acoustic_contacts[i].svp_ready);
	}
    }
  return(false);
}

bool SimPassiveSonar::requestRayTrace(std::string cn, 
				       double r_min, double r_max, int num_r, 
				       double dep_min, double dep_max, int num_dep,
				       double freq, int target_number)
{

  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      if (MOOSStrCmp(acoustic_contacts[i].contact,cn))
	{
	  // if we waited more than 30 secs, iBellhop must have missed request
	  if (readyAcousticContact(cn) || (MOOSTime() - acoustic_contacts[i].last_request) > 30.0 )
	    // if (readyAcousticContact(cn) || acoustic_contacts[i].last_request <= 0 )
	    {
	      // Request raytracing
	      string acoustic_contact = cn + "_SIM";

	      //	      double direct_theta = atan2(depth,range)*180.0/M_PI;
	      double theta_max = 60.0;
	      //	      if (direct_theta > 0.5*theta_max)
	      //		theta_max = 0.5*(direct_theta+90.0);

	      iBellhopRequest request;

	      bellhop::protobuf::Environment::AdaptiveInfo* adaptive_info = request.mutable_env()->mutable_adaptive_info();
	      adaptive_info->set_contact(acoustic_contact);
	      //  adaptive_info->set_ownship(receiver);
	      //adaptive_info->set_ownship(ownship);
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
	      env->set_freq(freq);
	      request.set_request_number(target_number);
	      // mseas svp updates
	      if (mseas_svp_updates && acoustic_contacts[i].svp_ready)
		{
		  request.mutable_env()->mutable_water_column()->
		    CopyFrom(acoustic_contacts[i].svp_response.water_column());
		  request.set_water_column_action(iBellhopRequest::OVERWRITE);
		}
	      std::string noutput;
	      serialize_for_moos(&noutput,request);
	      m_Comms.Notify("BELLHOP_REQUEST",noutput);
	      // Wait for Bellhop before making next request
	      
	      acoustic_contacts[i].ready = false;
	      acoustic_contacts[i].last_request = MOOSTime();
	    } 
	  return(true);
	}
    }
  return(false);
}

bool SimPassiveSonar::requestSVPUpdate(std::string cn, 
					int contact_number)
{

  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      if (MOOSStrCmp(acoustic_contacts[i].contact,cn))
	{
	  if (readyAcousticContactSVP(cn) || acoustic_contacts[i].last_svp_request <= 0 )
	    {

	      SVP::Request svp_request;
	      std::string serialized_svp_request;
	      double ael_lat, ael_lon, source_pos_lat, source_pos_lon;
	      acoustic_contacts[i].svp_request_id = contact_number;
	      svp_request.set_request_id(contact_number);
	      svp_request.set_utc_time(MOOSTime());
	      m_Geodesy.UTM2LatLong(ael_x,ael_y,ael_lat,ael_lon); 
	      m_Geodesy.UTM2LatLong(acoustic_contacts[i].contact_x,
				    acoustic_contacts[i].contact_y,
				    source_pos_lat,source_pos_lon); 
	      
	      svp_request.set_receiver_lat(ael_lat);
	      svp_request.set_receiver_lon(ael_lon);
	      svp_request.set_source_lat(source_pos_lat);
	      svp_request.set_source_lon(source_pos_lon);
	      svp_request.set_delta_range(svp_range_resolution);
	      svp_request.set_delta_depth(svp_depth_resolution);
	      
	      
	      serialize_for_moos(&serialized_svp_request, svp_request);
	      //m_Comms.Notify("SVP_REQUEST", serialized_svp_request);
	      // Queue request for slow loop
	      svp_requests.push_back(serialized_svp_request);
	      // if not ready use old one
	      // acoustic_contacts[i].svp_ready = false;
	      acoustic_contacts[i].last_svp_request = MOOSTime();	    
	    }
	  return(true);
	}
    }
  return(false);
}

bool SimPassiveSonar::handleSVPResponse(const CMOOSMsg& msg)
{
  std::string serialized_svp_response = msg.GetString();
  SVP::Response svp_response;
  parse_for_moos(serialized_svp_response, &svp_response);
  if (putAcousticContactSVP(svp_response))
    {
      glog << "Got SVP update for source id " << svp_response.request_id() 
	   << endl; 
      return(true);
    }
  else
    return(false);
  
}

void SimPassiveSonar::handle_pinger_command(const CMOOSMsg& msg)
{
  std::string serialized_pinger_command = msg.GetString();
  parse_for_moos(serialized_pinger_command,&pinger_command);

  pinger_active = pinger_command.pinger_active();
  if (pinger_active)
    {
      pinger_id = "PINGER";
      pinger_x = pinger_command.pinger_x();
      pinger_y = pinger_command.pinger_y();
      pinger_depth = pinger_command.pinger_depth();
      pinger_db = pinger_command.pinger_db();

      ping_center_frequency = pinger_command.center_frequency();
      ping_bandwidth = pinger_command.bandwidth();
      ping_pulsewidth = pinger_command.pulsewidth();
      CEigenRayTable buffer;
      buffer.clear();
      putAcousticContact(pinger_id, pinger_x,
			 pinger_y,
			 pinger_depth, 
			 buffer);
      int cn_num = findAcousticContact(pinger_id);
      if (MOOSTime() - acoustic_contacts[cn_num].last_request > 10)
	{
	  // force fresh raytracing
	  acoustic_contacts[cn_num].ready = false;
	}
      glog.is(DEBUG1) && glog << "Pinger " 
			      << " assigned acoustic contact number "
			      << findAcousticContact(pinger_id) << std::endl;
      if (!pinger_enabled && ray_acoustics)
	{
	  string acoustic_contact = pinger_id 
	    +"_SIM";
	  
	  std::string var = acoustic_contact + "_ARR_FILE";
	  subscribe(var, &SimPassiveSonar::handle_pinger_arrival_file, this);    
	  
	  glog.is(DEBUG1) && glog << "Variable " << var << " registered" << endl;
	  pinger_enabled = true;
	}
    }

  if (pinger_command.pingtime() < 1)
    pingtime = pinger_command._time();
  else
    pingtime = pinger_command.pingtime();
  // Generate no-doppler replica in case we want to save for MF
  generate_doppler_waveform(waveform,1.0);
}

double SimPassiveSonar::getAmbientNoise(double depth, 
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
    nl = noise_level;

  // generate isotropic noise covariance matrix
  // first time, following ones just scaled, avoiding cholesky factorization
  //
  //  if (noise_direct)
  //    {

      int ndim;
      if (vsa)
	ndim = 4*num_array_elements;
      else
	ndim = num_array_elements;
	glog.is(DEBUG1) && glog << "nl=" << nl
				<< ", Fs=" << sampling_frequency
				<< ", nfft=" << nfft << endl;
      double p_intens = 1e-12 * pow(10.0,nl/10) 
	* sampling_frequency * 0.5 * nfft;
      double rho = 1000;
      // Compute correlated noise only for spectrum of 
      // interest
      double delta_f = sampling_frequency/nfft;
      int i_first = 0;
      if (minimum_frequency >= 0)
	i_first = int(minimum_frequency/delta_f);
      int i_last = num_freqs;
      if (maximum_frequency < 0.5*sampling_frequency)
	i_last = int(maximum_frequency/delta_f);

      // Initialize covariances with white noise    

      if (!noise_cov_built)
	{
	  noise_covar.clear();
	  vector<complex<double> > wncov;
	  CovMat dummycov;
	  // first fill in white noise at all frequencies
	  wncov.clear();
	  wncov = vecUtils.zzeros(ndim,ndim);
	  
	  for (int i_frq = 0; i_frq< num_freqs; i_frq++)
	    {
	      double freq = i_frq*sampling_frequency/nfft; 
	      double k = 2*M_PI*freq/water_c;
	      double a_intens = p_intens*pow(k,2.0)
		/(3*pow(rho,2.0));
	      if (i_frq==i_first || (i_frq== (i_first+i_last)/2) || i_frq==i_last)
		glog.is(DEBUG1) && glog << "p_intens= " << p_intens
					<< ", a_intens= " << a_intens
					<< ", freq= " << freq <<  "Hz" << endl;
	      for (int i=0; i< num_array_elements; i++)
		{
		  if (vsa)
		    {
		      wncov[4*i*(1+ndim)] = p_intens;
		      wncov[(4*i+1)*(1+ndim)] = a_intens;
		      wncov[(4*i+2)*(1+ndim)] = a_intens;
		      wncov[(4*i+3)*(1+ndim)] = a_intens;
		    }
		  else
		    wncov[i*(1+ndim)] = p_intens;
		}
	      
	      dummycov.putCovMat(ndim,freq,wncov);
	      // create diagonal cholesky
	      dummycov.wnChol();
	      noise_covar.push_back(dummycov);
	    }
	  theta.clear();
	  S.clear();
	  
	  for (int i=0; i < theta_deg.size(); i++)
	    {
	      theta.push_back(theta_deg[i]*M_PI/180.0);
	      S.push_back(pow(10.0,S_dB[i]/10));
	    }
	  noise_cov_built = true;
	}
      else
	{
	  double amp_fac = pow(10.0,(nl-noise_level)/20.0);
	  for (int k = 0; k< num_freqs; k++)
	    {
	      noise_covar[k].scaleCovMat(amp_fac);
	    }
	  glog.is(DEBUG1) && glog << "Scaled covariance matrices, factor = "
				  << amp_fac << endl;
	}
      noise_level = nl;

      // if pitch has changed more than 5 deg recompute 
      // covariances for frequency interval of interest      

      if ((noise_direct || ISOTROPIC) && fabs(array_avg_pitch-array_prev_pitch) 
	  >= 5.0*M_PI/180.0 )
	{    
	  string req = "nfft=" + intToString(nfft) + ",sampling_frequency=" + doubleToString(sampling_frequency);   
	  m_Comms.Notify("COVARIANCE_REQUEST",req);

	  glog.is(DEBUG1) && glog << "Requesting correlated noise covariances, i_first="
				  << i_first << ", i_last=" << i_last << endl;
	  array_prev_pitch = array_avg_pitch;
	}
      //    }
  return(nl);
}

bool SimPassiveSonar::parseNoiseProfile( string noise_in, 
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
		d += noise_level;
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

bool SimPassiveSonar::parseNoiseDirectionality( string noise_in, 
					  vector<double>& dom_nl,
					  vector<double>& noise_db)
{
  // read the ambient noise profile
  dom_nl.clear();
  noise_db.clear();

  std::string depths_nl, nls;    
  
  bool elevation;

  if (val_from_string(depths_nl, noise_in, "theta"))
    {
      elevation = false;
    }
  else if (val_from_string(depths_nl, noise_in, "elevation"))
    {
      elevation = true;
    }
  else
    {
      glog << "Error parsing Noise Directionality" << endl;
      return(false);
    }

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
	    {
	      double d = boost::lexical_cast<double>(s);
	      if (elevation)
		d = 90.0 - d;
	      dom_nl.push_back(d);
	    }
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
  return(true);
}    

bool SimPassiveSonar::read_covmat_file(string covmat_file, double nl_now, double& t_req)
{
  char covmat_filename_buff[1000]; 
  // Sure hope nobody wants a filename longer than 1000 chars
  sprintf(covmat_filename_buff,"%s",covmat_file.c_str());
  
  FILE * cov_data_file;
  if ( (cov_data_file = fopen(covmat_filename_buff, "rb")) == NULL)
    {
      glog << "Unable to open a file: " <<  covmat_filename_buff << endl;
      glog << "Does the directory exist?" << endl;
      glog << "Does base_dir_name begin and end with a forward slash?" << endl;
      glog << "Do you have permission to write to that directory?" << endl;
      exit(EXIT_FAILURE);
    }
  else
    {
      glog.is(DEBUG2) && glog << "Start reading file" << endl;
      fread(&t_req, sizeof(double), 1 , cov_data_file);

      double nl_req;
      fread(&nl_req, sizeof(double), 1 , cov_data_file);
      double facamp = pow(10.0,(nl_now-nl_req)/20.0);
      double faccov = facamp*facamp;
      double f_min, f_max, df;
      fread(&f_min, sizeof(double), 1 , cov_data_file);
      fread(&f_max, sizeof(double), 1 , cov_data_file);
      fread(&df, sizeof(double), 1 , cov_data_file);
      int i_first, i_last;
      fread(&i_first, sizeof(int), 1 , cov_data_file);
      fread(&i_last, sizeof(int), 1 , cov_data_file);

      int num_sensors;

      fread(&num_sensors, sizeof(int), 1 , cov_data_file);
      glog.is(DEBUG1) && glog << "Done reading header, i_first=" << i_first
			      << ", i_last=" << i_last
			      << ", num-sensors=" << num_sensors<< endl;
      for (int f_iter=i_first; f_iter <= i_last; f_iter++)
	{
	  
	  vector<complex<double> > covmat;
	  vector<complex<double> > chollow;
	  double f;

	  fread(&f, sizeof(double), 1 , cov_data_file);

	  int num = num_sensors*(num_sensors+1)/2;
	  for (int i=0; i<num; i++)
	    {
	      float rr,ri;
	      fread(&rr, sizeof(float), 1 , cov_data_file);
	      fread(&ri, sizeof(float), 1 , cov_data_file);
	      covmat.push_back(faccov*complex<double>(double(rr),double(ri)));
	    }
	  noise_covar[f_iter].putCovLow(num_sensors, f, covmat);
	  for (int i=0; i<num; i++)
	    {
	      float rr,ri;
	      fread(&rr, sizeof(float), 1 , cov_data_file);
	      fread(&ri, sizeof(float), 1 , cov_data_file);
	      chollow.push_back(facamp*complex<double>(double(rr),double(ri)));
	    }
	  noise_covar[f_iter].putCholLow(num_sensors, f, chollow);
	}

      fclose (cov_data_file);
      glog.is(VERBOSE) && glog << "Read Covariance file: " << covmat_filename_buff 
			       << endl;
    }
  return(true);  
}

// Sensor class

bool CSensor::initSpectrum(int nfft_in)
{
  nfft = nfft_in;
  spectrum.clear();
  for (int i=0; i< nfft; i++)
    spectrum.push_back(0.0);
  return(true);
}

bool CSensor::clearSpectrum()
{
  for (int i=0; i< nfft; i++)
    spectrum[i]=0.0;
  return(true);
}

bool CSensor::putSpectrum(vector<double> spec_in)
{
  if (nfft == spec_in.size())
    spectrum = spec_in;
  else
    return(false);
  return(true);
}

bool CSensor::addToSpectrum(vector<double> spec_in)
{
  if (nfft == spec_in.size())
    {
      for (int i=0; i< nfft; i++)
	spectrum[i] += spec_in[i];
    }
  else
    return(false);
  return(true);
}

vector<double> CSensor::getSpectrum()
{
  return(spectrum);
}

vector<double> CSensor::timeseries()
{
  double gsl_data[nfft];
  for (int i=0; i< nfft; i++)
    gsl_data[i] = spectrum[i];
  gsl_fft_halfcomplex_radix2_inverse (gsl_data, 1, nfft);
  vector<double> ts;
  for (int i=0; i< nfft; i++)
    ts.push_back(gsl_data[i]);
  return(ts);
}

