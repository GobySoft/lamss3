// copyright 2010 H. Schmidt henrik@mit.edu
// 
// this file is part of moos-ivp-local/src-shared, a collection of MOOS
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

CSimPassiveSonar::CSimPassiveSonar()
{
  MOOSTrace("====================================\n");
  MOOSTrace("\n");
  MOOSTrace("          pSimPassiveSonar\n");
  MOOSTrace("\n");
  MOOSTrace("          V 2.0 9/29/2010 \n");
  MOOSTrace("\n");
  MOOSTrace("         Henrik Schmidt, MIT\n");
  MOOSTrace("Sep. 2009, H. Schmidt. Multiple targets \n");
  MOOSTrace("Jul. 2010, HS.:Adapted to DSOP convention \n");   
  MOOSTrace("Sep. 2010, HS.:Interfaced to Bellhop for HiFi simulation \n");   
  MOOSTrace("====================================\n");

  // initialize a bunch of variables  

  t_control = false;
  // The following is for multiple targets
  // single target mode disabled. Should always be used with pMultiTargetSim
  multi_target = false;
  number_of_targets = 0;
  max_number_bounces = 1;
  // range change forcing new raytrace
  raytracing_interval = 30;
  vsa = false;
  // linear svp, default isovelocity
  c_0 = 1500;
  c_grad = 0;
  z_max = 5000;
  water_c = c_0;
  ray_acoustics =  false;
  rap_acoustics =  false;
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

  hydrophone_gain = 1;
  accel_gain = 1;
  
  ok_sourcex = false;
  ok_sourcey = false;
  
  ok_source_bb_spl = false;
  ok_source_bb_freq = false;
  ok_source_bb_bw = false;
  got_source_bb_info = false;

  sync = false;
  sync_path = "mituser@gambero:/opt/missions-dsop/logs/simulation/DURIP";

  // Clear ray buffer
  eig.clear();
  
  source_status = false; // default to the source being off.

  time_last = 0;
  frame_number = 0;

  debug = false;
  debug2 = false;

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
  noise_level = -1000;
  array_prev_pitch = -1000;

  minimum_frequency = 0;
  maximum_frequency = 1e20;
  noise_cov_built = false;

  mseas_svp_updates =false;
  mseas_update_interval = 30;
  svp_range_resolution = 100;
  svp_depth_resolution = 20;
}

CSimPassiveSonar::~CSimPassiveSonar()
{
}

bool CSimPassiveSonar::OnStartUp()
{
  string sVal;
  
  //Initialize m_Geodesy from lat lon origin in .moos file
  double dfLatOrigin;
  double dfLongOrigin;
  
  if(m_MissionReader.GetValue("LatOrigin",sVal))
    {
      dfLatOrigin = atof(sVal.c_str());
      //IF_ASC_MOOSTRACE MOOSTrace("  LatOrigin  = %10.5f deg.\n",dfLatOrigin);
    }
  else
    {
      MOOSTrace("LatOrigin not specified in mission file - FAIL\n");
      return false;
    }
  
  if(m_MissionReader.GetValue("LongOrigin",sVal))
    {
      dfLongOrigin = atof(sVal.c_str());
      //IF_ASC_MOOSTRACE MOOSTrace("  LongOrigin = %10.5f deg.\n",dfLongOrigin);
    }
  else
    {
      MOOSTrace("LongOrigin not specified in mission file - FAIL\n");
      return false;
    }
  
  if(!m_Geodesy.Initialise(dfLatOrigin,dfLongOrigin))
    {
      MOOSTrace("Geodesy Init inside pActiveSonarControl failed - FAIL\n");
      return false;
    }
  //Done initializaing m_Geodesy
  

  if(m_MissionReader.GetConfigurationParam("mseas_svp_updates",sVal))
   {
     mseas_svp_updates = MOOSStrCmp(tolower(sVal),"true");
   }

  if(m_MissionReader.GetConfigurationParam("mseas_update_interval",sVal))
   {
     mseas_update_interval = atof(sVal.c_str());
   }

  if(m_MissionReader.GetConfigurationParam("svp_range_resolution",sVal))
   {
     svp_range_resolution = atof(sVal.c_str());
   }

  if(m_MissionReader.GetConfigurationParam("svp_depth_resolution",sVal))
   {
     svp_depth_resolution = atof(sVal.c_str());
   }

  if(m_MissionReader.GetConfigurationParam("minimum_frequency",sVal))
  {
    minimum_frequency = atof(sVal.c_str());
    MOOSTrace("Minimum processing frequency: %f\n",minimum_frequency);
  }

  if(m_MissionReader.GetConfigurationParam("maximum_frequency",sVal))
  {
    maximum_frequency = atof(sVal.c_str());
    MOOSTrace("Maximum processing frequency: %f\n",maximum_frequency);
  }

  if (m_MissionReader.GetConfigurationParam("isotropic_noise",sVal))
  {
    ISOTROPIC = MOOSStrCmp(tolower(sVal),"true");
    if (ISOTROPIC)
      MOOSTrace("Using isotropic noice covariances\n");
  }

  if (m_MissionReader.GetConfigurationParam("ray_acoustics",sVal))
  {
    ray_acoustics = MOOSStrCmp(tolower(sVal),"true");
    if (ray_acoustics)
      MOOSTrace("Using Bellhop ray tracing\n");
  }

  if (m_MissionReader.GetConfigurationParam("rap_acoustics",sVal))
  {
    rap_acoustics = MOOSStrCmp(tolower(sVal),"true");
    if (rap_acoustics)
      MOOSTrace("Using RAP path ray tracing\n");
  }

  if(m_MissionReader.GetConfigurationParam("max_number_bounces",sVal))
    {
      max_number_bounces = atoi(sVal.c_str());
      MOOSTrace("max_number_bounces = %d \n",max_number_bounces);
    }
  else
    {
      MOOSTrace("max_number_bounces not specified, using default = %d \n",max_number_bounces);
    }

  if(m_MissionReader.GetConfigurationParam("num_array_elements",sVal))
  {
    num_array_elements = atoi(sVal.c_str());
    MOOSTrace("num_array_elements = %d \n",num_array_elements);
    // Initialize some vectors now that the we know the number of elements
    array_x.assign(num_array_elements,0);
    array_y.assign(num_array_elements,0);
    array_z.assign(num_array_elements,0);
    array_heading.assign(num_array_elements,0);
    array_pitch.assign(num_array_elements,0);
  }
  else
  {
    MOOSTrace("pSimPassiveSonar:: num_array_elements not specified - FAIL\n");
    exit(EXIT_FAILURE);
  }

  if(m_MissionReader.GetConfigurationParam("base_dir_name",sVal))
  {
    base_dir_name = sVal.c_str();
  }
  else
  {
    MOOSTrace("pSimPassiveSonar:: base_dir_name not specified - FAIL\n");
    exit(EXIT_FAILURE);
  }

  if(m_MissionReader.GetConfigurationParam("array_type",sVal))
  {
    array_type = sVal.c_str();
    vsa = (array_type == "VSA");
  }
  else
  {
    MOOSTrace("pSimPassiveSonar:: array_type not specified - FAIL\n");
    exit(EXIT_FAILURE);
  }

  if(m_MissionReader.GetConfigurationParam("acoustic_filename_prefix",sVal))
  {
    acoustic_filename_prefix = sVal.c_str();
  }
  else
  {
    MOOSTrace("pSimPassiveSonar:: acoustic_filename_prefix not specified - FAIL\n");
    exit(EXIT_FAILURE);
  }

  if(m_MissionReader.GetConfigurationParam("acoustic_filename_suffix",sVal))
  {
    acoustic_filename_suffix = sVal.c_str();
  }
  else
  {
    MOOSTrace("pSimPassiveSonar:: acoustic_filename_suffix not specified - FAIL\n");
    exit(EXIT_FAILURE);
  }

  if(m_MissionReader.GetConfigurationParam("nonacoustic_filename_prefix",sVal))
  {
    nonacoustic_filename_prefix = sVal.c_str();
  }
  else
  {
    MOOSTrace("pSimPassiveSonar:: nonacoustic_filename_prefix not specified - FAIL\n");
    exit(EXIT_FAILURE);
  }

  if(m_MissionReader.GetConfigurationParam("nonacoustic_filename_suffix",sVal))
  {
    nonacoustic_filename_suffix = sVal.c_str();
  }
  else
  {
    MOOSTrace("pSimPassiveSonar:: nonacoustic_filename_suffix not specified - FAIL\n");
    exit(EXIT_FAILURE);
  }

  if(m_MissionReader.GetConfigurationParam("sync_path",sVal))
    {
      sync_path = sVal.c_str();
      sync = true;
    }

  if(m_MissionReader.GetConfigurationParam("water_depth",sVal))
  {
    water_depth = atof(sVal.c_str());
  }
  else
  {
    MOOSTrace("pSimPassiveSonar:: No water_depth - FAIL\n");
    exit(EXIT_FAILURE);
  }

    if (m_MissionReader.GetValue("svp_surface",c_0) ||
	m_MissionReader.GetValue("water_c",c_0)       )
      {
	water_c = c_0;
	MOOSTrace("Surface Sound Speed = %f m/s\n",c_0);
      }
    else
      MOOSTrace("No svp_surface or water_c specified, using default %f m/s \n",c_0);
    
    if(m_MissionReader.GetValue("svp_gradient",c_grad))
      {
	MOOSTrace("Sound Speed Gradient = %f (m/s)/m\n",c_grad); 
      }
    else
      MOOSTrace("No svp_gradient specied, assuming isovelocity \n");

    if(m_MissionReader.GetValue("max_rap_depth",z_max))
      {
	MOOSTrace("Max RAP Depth = %f m/s\n",z_max);
      }
    else
      MOOSTrace("No max_rap_depth specified, using default %f m \n",z_max);
    
  if(m_MissionReader.GetConfigurationParam("bottom_loss",sVal))
  {
    bottom_loss = atof(sVal.c_str());
  }
  else
  {
    MOOSTrace("pSimPassiveSonar:: No bottom_loss - FAIL\n");
    exit(EXIT_FAILURE);
  }


  if(m_MissionReader.GetConfigurationParam("hydrophone_gain",sVal))
  {
    // hydrophone_gain = atof(sVal.c_str());

    hydrophone_gain = pow(10.0, atof(sVal.c_str())/20.0);
  }
  else
  {
    MOOSTrace("pSimPassiveSonar:: No hydrophone_gain - using default unity \n");
  }

  if(m_MissionReader.GetConfigurationParam("accelerometer_gain",sVal))
  {
    accel_gain = pow(10.0, atof(sVal.c_str())/20.0);
  }
  else
  {
    MOOSTrace("pSimPassiveSonar:: No accelerometer_gain - Using default unity \n");
  }

  if(m_MissionReader.GetConfigurationParam("sampling_frequency",sVal))
  {
    sampling_frequency  = atof(sVal.c_str());
  }
  else
  {
    MOOSTrace("pSimPassiveSonar:: no sampling_frequency - FAIL\n");
    exit(EXIT_FAILURE);
  }

  if(m_MissionReader.GetConfigurationParam("noise_level",sVal))
  {
    noise_level  = atof(sVal.c_str());
    // calculate the standard deviation of the Gaussian 
    // distribution reqired to make the noise have a decibel
    // level commensurate with the one specified in noise_level.
    //noise_std = pow(10, noise_level/20)*pow(10, -6);
    noise_std = pow(10.0, noise_level/20.0)*pow(10.0, -6.0)*sqrt(sampling_frequency*0.5);//Correction AJP 3 NOV 2010 (Generate background noise with the correct spectral level.)
  }
  else
  {
    MOOSTrace("pSimPassiveSonar:: no noise_level- FAIL\n");
    exit(EXIT_FAILURE);
  }

  if(m_MissionReader.GetConfigurationParam("num_samples",sVal))
  {
    num_samples = atoi(sVal.c_str());
    file_interval_time = double(num_samples)/sampling_frequency;
    // Find an fft length (round to next highest power of 2)
    unsigned int const v = num_samples; 
    // Put the result here. (So v=3 -> r=4; v=8 -> r=8)
    float f = (float)(v - 1);  
    nfft = 1 << ((*(unsigned int*)(&f) >> 23) - 126);
    MOOSTrace("nfft = %u\n",nfft);
    // Build the dft of the signal in a format similar to 
    // matlab (same as gsl), described in
    // http://www.gnu.org/software/gsl/manual/html_node/Overview-of-complex-data-FFTs.html
    num_freqs = nfft/2 + 1; 
    MOOSTrace("Each file represents about %0.3f seconds of data\n", file_interval_time);
  }
  else
  {
    MOOSTrace("pSimPassiveSonar:: No num_samples - FAIL\n");
    exit(EXIT_FAILURE);
  }


  // Now that we have some parameters, allocate some matricies
  sum_of_timeseries = gsl_matrix_calloc (num_array_elements,num_samples);
    if (vsa)
    {
      //Vector components
      sum_of_timeseries_x = gsl_matrix_calloc (num_array_elements,num_samples);
      sum_of_timeseries_y = gsl_matrix_calloc (num_array_elements,num_samples);
      sum_of_timeseries_z = gsl_matrix_calloc (num_array_elements,num_samples);
    }
  return(true);
}

bool CSimPassiveSonar::OnConnectToServer()
{
  m_Comms.Register( "ARRAY_X",0);
  m_Comms.Register( "ARRAY_Y",0);
  m_Comms.Register( "ARRAY_Z",0);
  m_Comms.Register( "ARRAY_DIR_X",0);
  m_Comms.Register( "ARRAY_DIR_Y",0);
  m_Comms.Register( "ARRAY_DIR_Z",0);
  m_Comms.Register( "ARRAY_HEADING",0);
  m_Comms.Register( "ARRAY_PITCH",0);

  m_Comms.Register( "TARGET_SIM_LIST",0);
  m_Comms.Register( "AMBIENT_NL_DEPTH",0);
  m_Comms.Register( "NOISE_VERT_DIR",0);
  m_Comms.Register( "COVARIANCE_FILE",0);

  m_Comms.Register( "SVP_RESPONSE",0);



  return(true);
}

bool CSimPassiveSonar::Iterate()
{

  //  MOOSTrace("Iterate \n ");
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
			+pow(array_dir_x[i],2) > 1e-6);
    }  
  else
    {
      array_dir.resize(num_array_elements);
      for (unsigned int i = 0; i < num_array_elements; i++)
	array_dir[i] = false;
    }

  // MOOSTrace("Iterate: got target info\n");

  double t_now = MOOSTime();
  // Note that this timing scheme isn't perfect, but the processing chain
  // should be robust enough to handle it.
  if ( (t_now - time_last) > file_interval_time)
    {
      time_last = t_now;
      
      if (multi_target && number_of_targets >0)
	getTargetInfo(0);
      else
	{
	  got_source_bb_info = (ok_source_bb_spl && ok_source_bb_freq && 
				ok_source_bb_bw);
	  got_source_loc = (ok_sourcex && ok_sourcey);
	}
      
      // Output error messages 
      if(!got_array_info)
	{
	  MOOSTrace("ARRAY_[X, Y or Z] *not* in the MOOSDB.\n");
	  MOOSTrace("Waiting until all required variables are in the MOOSDB...\n\n");
	  return(true);
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
	  MOOSTrace("Local ambient noise in dB: %f \n",nl);
	  //noise_std = pow(10.0, nl/20.0)*pow(10.0, -6.0);
	  //Correction AJP 3 NOV 2010 (Generate background noise with the correct spectral level.)
	  noise_std = pow(10.0, nl/20.0)*pow(10.0, -6.0)*sqrt(sampling_frequency*0.5); 	  
	  
	  // initialize spectra
	  initialize_spectrum();
	  
	  // generate corrrelated  noise spectrum
	  generate_corr_noise_spectrum();

	  for (int i=0; i<number_of_targets; i++)
	    {
	      MOOSTrace("Handling target %d\n",i);

	      bool ok_tgt_info = getTargetInfo(i);
	      if (!ok_tgt_info)
		{
		  MOOSTrace("Waiting for TGT_%d info\n",m_target_number[i]);
		  return(true);
		}
	      
	      MOOSTrace("Got TGT_%d info \n",m_target_number[i]);
	      
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
		  putAcousticContact(Targets[i].target_id(),eigenray_table);
		}
	      // first get the ray table for this contact
	      if (getAcousticContact(Targets[i].target_id(),eigenray_table))
		{
		  CEigenRay eig;
		  // Get the closest eigenray bundle
		  eigenray_table.getEigenRays(range_req, depth_req, range_ref, depth_ref, eig);
		  if (ray_acoustics)
		    {
		      MOOSTrace("Target range= %f, Raytracing range = %f\n", range_req,eig.range);
		      MOOSTrace("Receiver depth= %f, Raytracing depth = %f\n", depth_req,eig.receiver_depth);
		      MOOSTrace("Number of eigenrays: %d\n",eig.num_rays);
		      tl_est = 1e-30;
		      for (int iray = 0; iray < eig.num_rays; iray++)
			{
			  tl_est += pow(eig.bundle[iray].amp,2);
			}
		      tl_est = - 10.0 * log10(tl_est/eig.num_rays);
		      MOOSTrace("Incoherent TL = %f\n", tl_est);
		      
		      //		      if (fabs(range_req - eig.range) > 0.5*raytracing_interval)
		      if (fabs(range_req - acoustic_contacts[i].last_range) > raytracing_interval)
			{
			  double range_min;
			  double range_max;
			  double depth_min;
			  double depth_max;
			  int num_ranges;
			  int num_depths;
			  defineRayTracingBox(acoustic_contacts[i],
					      range_min,range_max,
					      depth_min,depth_max,
					      num_ranges,num_depths);
			  requestRayTrace(Targets[i].target_id(), 
					  range_min, range_max, num_ranges, 
					  depth_min, depth_max, num_depths,
					  source_bb_freq, m_target_number[i]);
			  
			}
		    // if mseas_svp_updates, request SVP
		      if ( mseas_svp_updates && 
			   (MOOSTime() - acoustic_contacts[i].last_svp_request >= 
			    mseas_update_interval ) )
			{
			  requestSVPUpdate(Targets[i].target_id(),
				       m_target_number[i]);
			}
		    }

		  generate_signal_spectrum(i);

		  MOOSTrace("BB signal done -  target %d\n",i);

		  icnt_rot = int( (t_now-start_time)/7 )%number_of_targets;
		  if(got_source_loc && i == icnt_rot)
		    {
		      // Publish simulated target position rotating through targets
		      m_Comms.Notify("TARGET_XPOS",source_pos_x);
		      m_Comms.Notify("TARGET_YPOS",source_pos_y);
		    }
		}
	      else if (ray_acoustics && (MOOSTime() - acoustic_contacts[i].last_request > 30.0))
		{
		  double range_min;
		  double range_max;
		  double depth_min;
		  double depth_max;
		  int num_ranges;
		  int num_depths;
		  defineRayTracingBox(acoustic_contacts[i],
				      range_min,range_max,
				      depth_min,depth_max,
				      num_ranges,num_depths);
		  requestRayTrace(Targets[i].target_id(), 
				  range_min, range_max, num_ranges, 
				  depth_min, depth_max, num_depths,
				  source_bb_freq, m_target_number[i]);

		  // if mseas_svp_updates, request SVP
		  if (mseas_svp_updates &&
		      (MOOSTime() - acoustic_contacts[i].last_svp_request >= 
		       mseas_update_interval) )
		    {
		      requestSVPUpdate(Targets[i].target_id(),
				       m_target_number[i]);
		    }

		}
	    }
	  
	  generate_spectral_timeseries();
	  
	  MOOSTrace("writing data files\n");
	  write_nas_file(t_now);
	  write_nas_file_verbose(t_now);
	  write_hyd_file();
	  m_Comms.Notify("SONAR_DIR", base_dir_name);
	  m_Comms.Notify("SONAR_FILE_NUM", double(frame_number));
	  
	  MOOSTrace("\n");
	  frame_number++;
	}
    }
  return(true);
}

//-------------------------------------------------------------
// Procedure: OnNewMail

bool CSimPassiveSonar::OnNewMail(MOOSMSG_LIST &NewMail)
{
  MOOSMSG_LIST::iterator p;
  for(p = NewMail.begin(); p != NewMail.end(); p++) {
    CMOOSMsg &Msg = *p;

    if(Msg.m_sKey == "ARRAY_X")
    {
      string s_hydrophone_x = Msg.m_sVal;
      int nx=str2dbl_vec(s_hydrophone_x, array_x_rel);
      if (nx != num_array_elements)
      {
        MOOSTrace("Array inconsistency - FAIL\n");
        exit(EXIT_FAILURE);
      }	
      ael_x = vecUtils.mean(array_x_rel);
      ok_hydx = true;
    }

    else if(Msg.m_sKey == "ARRAY_Y")
    {
      string s_hydrophone_y = Msg.m_sVal;
      int ny=str2dbl_vec(s_hydrophone_y, array_y_rel);
      if (ny != num_array_elements)
      {
        MOOSTrace("Array inconsistency - FAIL\n");
        exit(EXIT_FAILURE);
      }
      ael_y = vecUtils.mean(array_y_rel);
      ok_hydy = true;
    }

    else if(Msg.m_sKey == "ARRAY_Z")
    {
      string s_hydrophone_z = Msg.m_sVal;
      int nz=str2dbl_vec(s_hydrophone_z, array_z_rel);
      if (nz != num_array_elements)
      {
        MOOSTrace("Array inconsistency - FAIL\n");
        exit(EXIT_FAILURE);
      }
      ael_z = vecUtils.mean(array_z_rel);
      ok_hydz = true;
    }

       else if(Msg.m_sKey == "ARRAY_DIR_X") 
	 {
	   string s_dir_x = Msg.m_sVal;
	   int nx=str2dbl_vec(s_dir_x, array_dir_x);
	   if (nx != num_array_elements) {
	     MOOSTrace("Array inconsistency - FAIL\n");
	     exit(EXIT_FAILURE); }	
	   ok_dir_x = true;
	 }
       else if(Msg.m_sKey == "ARRAY_DIR_Y") 
	 {
	   string s_dir_y = Msg.m_sVal;
	   int ny=str2dbl_vec(s_dir_y, array_dir_y);
	   if (ny != num_array_elements) {
	     MOOSTrace("Array inconsistency - FAIL\n");
	     exit(EXIT_FAILURE); }	
	   ok_dir_y = true;
	 }
       else if(Msg.m_sKey == "ARRAY_DIR_Z") 
	 {
	   string s_dir_z = Msg.m_sVal;
	   int nz=str2dbl_vec(s_dir_z, array_dir_z);
	   if (nz != num_array_elements) {
	     MOOSTrace("Array inconsistency - FAIL\n");
	     exit(EXIT_FAILURE); }	
	   ok_dir_z = true;
	 }
    else if(Msg.m_sKey == "ARRAY_HEADING")
    {
      string s_array_heading = Msg.m_sVal;
      int nz=str2dbl_vec(s_array_heading, array_heading);
      if (nz != num_array_elements)
      {
        MOOSTrace("Array inconsistency - FAIL\n");
        exit(EXIT_FAILURE);
      }
      ok_array_heading = true;
    }

    else if(Msg.m_sKey == "ARRAY_PITCH")
    {
      string s_array_pitch = Msg.m_sVal;
      int nz=str2dbl_vec(s_array_pitch, array_pitch);
      if (nz != num_array_elements)
      {
        MOOSTrace("Array inconsistency - FAIL\n");
        exit(EXIT_FAILURE);
      }
      double sum = 0;
      for (int i = 0; i< nz; i++)
	sum += array_pitch[i];
      array_avg_pitch = sum/nz;

      ok_array_pitch = true;
    }

    else if(Msg.m_sKey == "AMBIENT_NL_DEPTH")
      {
	// noise profile
	noise_profile = parseNoiseProfile(Msg.m_sVal, 
					  depth_nl,
					  noise_db);
      }
    else if(Msg.m_sKey == "NOISE_VERT_DIR")
      {
	// noise profile. Disabled until debugged

	MOOSTrace("Received NOISE_VERT_DIR...");

	noise_direct = parseNoiseDirectionality(Msg.m_sVal, theta_deg, S_dB);

  	if (noise_direct)
    	    MOOSTrace("noise_direct=true\n");
  	else
    	    MOOSTrace("noise_direct=false\n");

      }
    else if(Msg.m_sKey == "COVARIANCE_FILE")
      {
	//	if (noise_profile && noise_direct && noise_cov_built)
	if (noise_direct && noise_cov_built)
	  {
	    string covmat_file = Msg.m_sVal;
	    double t_req;
	    MOOSTrace("Reading Covariance\n");
	    read_covmat_file(covmat_file, noise_level, t_req);
	    MOOSTrace("Got Covariance, delay = %f secs\n",MOOSTime()-t_req);
	  }
	else
	  MOOSTrace("Noise covariance received, but not used\n"); 
      }
    else if(Msg.m_sKey == "SVP_RESPONSE" )
      {
	handleSVPResponse(Msg);
      }
    else if(Msg.m_sKey == "TARGET_SIM_LIST" )
      {
	multi_target = true;
        vector<int> old_targets;
	old_targets.clear();
	
	for (int i=0; i< m_target_number.size(); i++)
	  old_targets.push_back(m_target_number[i]);
	int old_number_of_targets = old_targets.size();

	number_of_targets = str2int_vec(Msg.m_sVal,m_target_number);
	MOOSTrace("Number of targets = %d\n",number_of_targets);

	CTarget Empty;
	if (number_of_targets != old_number_of_targets )
	  {
	    Targets.clear();
	    ok_target_info.clear();

	    for (int i = 0; i< number_of_targets; i++)
	      {
		MOOSTrace("Target TGT_%d\n",m_target_number[i]);
		Targets.push_back(Empty);
		ok_target_info.push_back(false);
		Targets[i].target_number = m_target_number[i];
	      }
	    MOOSTrace("Done resetting \n");
	  }
	else
	  MOOSTrace("No resetting \n");
	
	for (int i = 0; i< number_of_targets; i++)
	  {
	    bool register_done = false;
	    for (int j=0; j< old_targets.size(); j++)
	      register_done =  (old_targets[j] == m_target_number[i]);
	    
	    if ( !register_done )
	      {
		string target_id = Targets[i].target_id();
		
		string var =target_id+"_SIM_STATE";
		m_Comms.Register(var,0);
		MOOSTrace("Variable %s registered\n",var.c_str());
		MOOSTrace("Target_id %s registered\n",target_id.c_str());

		// Initialize random number generator with seed=target_number
		gsl_rng_tgt.push_back(gsl_rng_alloc (gsl_rng_type_1));
		gsl_rng_set(gsl_rng_tgt[gsl_rng_tgt.size()-1], (long unsigned int) m_target_number[i]);

		if (ray_acoustics)
		  {

		    string acoustic_contact = Targets[i].target_id() 
		      +"_SIM";

		    var = acoustic_contact + "_ARR_FILE";
		    m_Comms.Register(var,0);
		    MOOSTrace("Variable %s registered\n",var.c_str());
		    
		    CEigenRayTable buffer;
		    buffer.clear();
		    putAcousticContact(Targets[i].target_id(), buffer);
		  }
	      }
	  }
      }
    else
      {
	for (int i = 0; i<number_of_targets; i++)
	  {
	    string target_id = Targets[i].target_id();;
	    //MOOSTrace("Looking for info on %s \n",target_id.c_str());
	    
	    string var =target_id+"_SIM_STATE";
	    string arr_file =target_id+"_SIM_ARR_FILE";
	    if(Msg.m_sKey == var)
	      {
		vector<double> values;
		int num_val = str2dbl_vec(Msg.m_sVal,values);
		int tgt_num = int(values[0]);
		if (tgt_num != Targets[i].target_number)
		  {
		    MOOSTrace("Target number inconsistency - Aborting \n");
		    return(false);
		  }
		bool tgt_on = (int(values[1]) == 1);
		for (int j = 0; j < num_val-2;j++)
		  values[j]=values[j+2];
		values.pop_back();
		values.pop_back();
		Targets[i].setTargetValues(tgt_num, tgt_on, values);
		// Targets[i].printTargetValues();
		//MOOSTrace("got %s\n", Msg.m_sKey.c_str());
		// if first target info request svp from mseas
		if ( mseas_svp_updates && 
		     (MOOSTime() - acoustic_contacts[i].last_svp_request >= 
		      mseas_update_interval ))
		  {
		    requestSVPUpdate(target_id,
				     m_target_number[i]);
		  }
		ok_target_info[i] = true;
	      }
	    else if (Msg.m_sKey == arr_file)
	      {
		eigenray_table.readEigenRayTable(Msg.m_sVal, max_number_bounces);
		putAcousticContact(target_id, eigenray_table);
	      }	
	  } 
      }
  }
  return(true);
}

bool CSimPassiveSonar::defineRayTracingBox(CAcousticContact& target,
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
	      spacing = std::min(sqrt(pow(array_x.at(j)-array_x.at(i),2)
				      + pow(array_y.at(j)-array_y.at(i),2)
				      + pow(array_z.at(j)-array_z.at(i),2)),
				 spacing);
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
      num_dep = 2*int((dep_max-dep_min)/(2*spacing)) + 1;

      double delta_t = MOOSTime() - target.last_rate_update; 
      if (target.last_rate_update > 0)
	{
	  target.last_rate = (range-target.last_range)/delta_t;	    
	  cout << endl << ">>> rate_update= " << target.last_rate << endl; 
	  cout << ">>> delta_t = " << delta_t << endl << endl ;
	}
      else
	target.last_rate = 0;
      target.last_range = range;
      target.last_rate_update = MOOSTime();
      return(true);
    }
  return(false);
}

bool CSimPassiveSonar::generate_signal_spectrum(int tgt_count)
{
  
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
	  
	  MOOSTrace("Receiver %d \n", phone_iter);
	  MOOSTrace("Target range= %f, Raytracing range = %f\n", range,eig.range);
	  MOOSTrace("Receiver depth= %f, Raytracing depth = %f\n", depth_req,eig.receiver_depth);
	  MOOSTrace("Number of eigenrays: %d\n",eig.num_rays);
	  
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
			      MOOSTrace("adding tone f=%f,mag=%f\n",tone_freqs[tone_iter],p_mag);
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

bool CSimPassiveSonar::generate_corr_noise_spectrum()
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
  
  MOOSTrace("Realization generation started, num covars=%d \n",noise_covar.size());
  for (int f_iter=1; f_iter < num_freqs; f_iter++)
    {
             
      vector<complex<double> > realization;
      noise_covar[f_iter].realization(realization);
      
      for (int sensor_iter=0; sensor_iter< num_sensors; sensor_iter++)
	{
	    noise_realization.push_back(realization[sensor_iter]);
	}
    }
  MOOSTrace("Realizations done\n");
 
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

bool CSimPassiveSonar::generate_spectral_timeseries()
{
  int sensors_per_element;
  int num_sensors;
  if (vsa) 
    sensors_per_element = 4;
  else
    sensors_per_element = 1;
  num_sensors = sensors_per_element*num_array_elements;
  
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

bool CSimPassiveSonar::initialize_spectrum()
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

bool CSimPassiveSonar::print_vec(vector<double> temp)
{
  for (int i=0; i < temp.size(); i++)
    MOOSTrace("%02d: %2.2f\n",i, temp.at(i));

  MOOSTrace("--done--\n");
  return(true);
}

bool CSimPassiveSonar::print_array(double a[], int l)
{
  for (int i=0; i < l; i++)
    MOOSTrace("%02d: %f\n",i, a[i]);

  MOOSTrace("--done--\n");
  return(true);
}


bool CSimPassiveSonar::write_hyd_file()
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
  m_Comms.Notify("SONAR_FILENAME",acoustic_filename_buff);

  // Got this filename stuff from CpBearingTrack
  
  FILE * data_file;
  if ( (data_file = fopen(acoustic_filename_buff, "wb")) == NULL)  
    {
      MOOSTrace("\nUnable to open a file:  %s \nFAIL\n", acoustic_filename_buff);
      MOOSTrace("Does the directory exist?\nDoes base_dir_name begin and end with a forward slash?\nDo you have permission to write to that directory?\n\n");
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
  
  MOOSTrace("Wrote file: %s \n", acoustic_filename_buff);
 
  // The following was added by HS 111102 to eliminate the need for
  // the frame_count.inp file and iDAS in simulation, 
  // and preparing for compatibility with consolidated acquisition 
  // software by tes for DURIP array etc.   
  //

  // Now transfer file to client if on separate host
  if (sync)
    {
      std::string acoustic_filename(acoustic_filename_buff);
      std::string cmd = "scp " + acoustic_filename + " " + sync_path;
      MOOSTrace("Synching with: %s \n", cmd.c_str());
      system(cmd.c_str());
      MOOSTrace("Synched with: %s \n", cmd.c_str());
    }
  
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

bool CSimPassiveSonar::write_nas_file(double t_now)
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
      MOOSTrace("\nUnable to open a file:  %s \nFAIL\n", nonacoustic_filename_buff);
      MOOSTrace("Does the directory exist?\nDoes base_dir_name begin and end with a forward slash?\nDo you have permission to write to that directory?\n\n");
      exit(EXIT_FAILURE);
    }
  else
    {
      // The following was copied from SealabDataSim2.m
      fprintf (nas_data_file, "%f\n", t_now);              // Current time
      
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
	  fprintf (nas_data_file, "%f\n", (t_now-start_time));
	  for (int jj=0; jj < num_array_elements; jj++)
	    {
	      fprintf(nas_data_file, "%8.4f %8.4f %8.4f\n",
		      array_x.at(jj), array_y.at(jj), fabs(array_z.at(jj)));
	    }
	}
      
      fclose (nas_data_file);
      MOOSTrace("Wrote file: %s \n", nonacoustic_filename_buff);
      if (sync)
	{
	  std::string nonacoustic_filename(nonacoustic_filename_buff);
	  std::string cmd = "scp -B " + nonacoustic_filename + " " + sync_path;
	  MOOSTrace("Synching with: %s \n", cmd.c_str());
	  system(cmd.c_str());
	  MOOSTrace("Synched with: %s \n", cmd.c_str());
	}
    }
  
  
}


bool CSimPassiveSonar::write_nas_file_verbose(double t_now) {
	double deg2rad = M_PI / 180;
	char nonacoustic_filename_buff[1000];
	// Sure hope nobody wants a filename longer than 1000 chars
	sprintf(nonacoustic_filename_buff, "%s/%s_%s_%09d%s", base_dir_name.c_str(), nonacoustic_filename_prefix.c_str(),"verbose", frame_number, nonacoustic_filename_suffix.c_str());

	FILE *nas_data_file;
	if ((nas_data_file = fopen(nonacoustic_filename_buff, "wt")) == NULL) {
		MOOSTrace("\nUnable to open a file:  %s \nFAIL\n", nonacoustic_filename_buff);
		MOOSTrace("Does the directory exist?\nDoes base_dir_name begin and end with a forward slash?\nDo you have permission to write to that directory?\n\n");
		exit(EXIT_FAILURE);
	} else {
		fprintf(nas_data_file, "Verbose uSimPassiveSonar non-acoustic data file\n");
		fprintf(nas_data_file, "\n--Data--\n");
		fprintf(nas_data_file, "%d \t\t! file number\n", frame_number); 
		fprintf(nas_data_file, "%8.4f ! current time\n", t_now); 
		fprintf(nas_data_file, "%8.4f \t! array uptime\n", (t_now - start_time));
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
		MOOSTrace("Wrote file: %s \n", nonacoustic_filename_buff);
		
		if (sync) {
			std::string nonacoustic_filename(nonacoustic_filename_buff);
			std::string cmd = "scp -B " + nonacoustic_filename + " " + sync_path;
			MOOSTrace("Synching with: %s \n", cmd.c_str());
			system(cmd.c_str());
			MOOSTrace("Synched with: %s \n", cmd.c_str());
		}
	}
}


int CSimPassiveSonar::str2int_vec(string msg, vector<int>& values)
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

bool CSimPassiveSonar::getTargetInfo(int i)
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
      MOOSTrace("target %d spl = %f, amp = %e \n", Targets[i].target_number, source_bb_spl,source_bb_amp);
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

bool CSimPassiveSonar::putAcousticContact(std::string cn, CEigenRayTable eig_table)
{
  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      if (MOOSStrCmp(acoustic_contacts[i].contact,cn))
	{
	  acoustic_contacts[i].ray_table = eig_table;
	  acoustic_contacts[i].ready = true;
	  return(true); 
	}
    }
  // make new contact
  CAcousticContact new_contact;
  new_contact.contact = cn;
  new_contact.ready = false;
  new_contact.ray_table = eig_table;
  new_contact.svp_ready = false;
  new_contact.last_svp_request = -1;
  new_contact.last_request = -1;
  new_contact.last_range = 0;
  new_contact.last_rate = 0;
  new_contact.last_rate_update = -1;
  acoustic_contacts.push_back(new_contact);
  MOOSTrace("Contact %s eigenrays updated\n",cn.c_str());
  MOOSTrace("Number of ranges: %d\n",eig_table.num_ranges);
  MOOSTrace("Number of depths: %d\n",eig_table.num_depths);
  return(true);
}

bool CSimPassiveSonar::putAcousticContactSVP(SVP::Response svp_response)
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

bool CSimPassiveSonar::getAcousticContact(std::string cn, CEigenRayTable& eig_table )
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

bool CSimPassiveSonar::getAcousticContactSVP(std::string cn, 
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

bool CSimPassiveSonar::readyAcousticContact(std::string cn)
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

bool CSimPassiveSonar::readyAcousticContactSVP(std::string cn)
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

bool CSimPassiveSonar::requestRayTrace(std::string cn, 
				       double r_min, double r_max, int num_r, 
				       double dep_min, double dep_max, int num_dep,
				       double freq, int target_number)
{

  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      if (MOOSStrCmp(acoustic_contacts[i].contact,cn))
	{
	  // if we waited more than 30 secs, iBellhop must have missed request
	  //	  if (readyAcousticContact(cn) || (MOOSTime() - acoustic_contacts[i].last_request) > 30.0 )
	  if (readyAcousticContact(cn) || acoustic_contacts[i].last_request <= 0 )
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
	      adaptive_info->set_ownship(acoustic_contact);
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

bool CSimPassiveSonar::requestSVPUpdate(std::string cn, 
					int target_number)
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
	      acoustic_contacts[i].svp_request_id = target_number;
	      svp_request.set_request_id(target_number);
	      svp_request.set_utc_time(MOOSTime());
	      m_Geodesy.UTM2LatLong(ael_x,ael_y,ael_lat,ael_lon); 
	      m_Geodesy.UTM2LatLong(Targets[i].target_x,Targets[i].target_y,
				    source_pos_lat,source_pos_lon); 
	      
	      svp_request.set_receiver_lat(ael_lat);
	      svp_request.set_receiver_lon(ael_lon);
	      svp_request.set_source_lat(source_pos_lat);
	      svp_request.set_source_lon(source_pos_lon);
	      svp_request.set_delta_range(svp_range_resolution);
	      svp_request.set_delta_depth(svp_depth_resolution);
	      
	      
	      serialize_for_moos(&serialized_svp_request, svp_request);
	      m_Comms.Notify("SVP_REQUEST", serialized_svp_request);
	      // if not ready use old one
	      acoustic_contacts[i].svp_ready = false;
	      acoustic_contacts[i].last_svp_request = MOOSTime();	    
	    }
	  return(true);
	}
    }
  return(false);
}

bool CSimPassiveSonar::handleSVPResponse(const CMOOSMsg& msg)
{
  std::string serialized_svp_response = msg.GetString();
  SVP::Response svp_response;
  parse_for_moos(serialized_svp_response, &svp_response);
  if (putAcousticContactSVP(svp_response))
    {
      cout << "Got SVP update for source id " << svp_response.request_id() 
	   << endl; 
      return(true);
    }
  else
    return(false);
  
}

double CSimPassiveSonar::getAmbientNoise(double depth, 
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
      if (debug2)
          MOOSTrace("nl=%lf, Fs=%lf, nfft=%d\n",nl,sampling_frequency,nfft);
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
	      if (debug2 && (i_frq==500 || i_frq==2000))
		  MOOSTrace("p_intens=%lf, a_intens=%e, freq=%lf Hz\n", p_intens, a_intens, freq);
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
	  MOOSTrace("Scaled covariance matrices, factor = %f\n",amp_fac);
	}
      noise_level = nl;

      // if pitch has changed more than 5 deg recompute 
      // covariances for frequency interval of interest      

      if ((noise_direct || ISOTROPIC) && fabs(array_avg_pitch-array_prev_pitch) 
	  >= 5.0*M_PI/180.0 )
	{    
	  string req = "nfft=" + intToString(nfft) + ",sampling_frequency=" + doubleToString(sampling_frequency);   
	  m_Comms.Notify("COVARIANCE_REQUEST",req);

	  MOOSTrace("Requesting correlated noise covariances, i_first=%d, i_last=%d\n", i_first,i_last);
	  array_prev_pitch = array_avg_pitch;
	}
      //    }
  return(nl);
}

bool CSimPassiveSonar::parseNoiseProfile( string noise_in, 
					  vector<double>& dom_nl,
					  vector<double>& noise_db)
{
  // read the ambient noise profile
  dom_nl.clear();
  noise_db.clear();

  std::string depths_nl, nls;    
  val_from_string(depths_nl, noise_in, "depths");
  val_from_string(nls, noise_in, "NL");
  
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
  return(true);
}    

bool CSimPassiveSonar::parseNoiseDirectionality( string noise_in, 
					  vector<double>& dom_nl,
					  vector<double>& noise_db)
{
  // read the ambient noise profile
  dom_nl.clear();
  noise_db.clear();

  std::string depths_nl, nls;    
  val_from_string(depths_nl, noise_in, "theta");
  val_from_string(nls, noise_in, "S");
  
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
  return(true);
}    

bool CSimPassiveSonar::read_covmat_file(string covmat_file, double nl_now, double& t_req)
{
  char covmat_filename_buff[1000]; 
  // Sure hope nobody wants a filename longer than 1000 chars
  sprintf(covmat_filename_buff,"%s",covmat_file.c_str());
  
  FILE * cov_data_file;
  if ( (cov_data_file = fopen(covmat_filename_buff, "rb")) == NULL)
    {
      MOOSTrace("\nUnable to open a file:  %s \nFAIL\n", covmat_filename_buff);
      MOOSTrace("Does the directory exist?\nDoes base_dir_name begin and end with a forward slash?\nDo you have permission to write to that directory?\n\n");
      exit(EXIT_FAILURE);
    }
  else
    {
      MOOSTrace("Start reading file\n");
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
      MOOSTrace("Done reading header, i_first=%d, i_last=%d, num-sensors=%d\n",i_first,i_last,num_sensors);
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
      MOOSTrace("Read Covariance file: %s \n", covmat_filename_buff);
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

