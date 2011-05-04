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

#include "goby/util/string.h"
#include "goby/util/time.h"
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
  raytracing_interval = 20;
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
  hydrophone_gain = 1;
  accel_gain = 1;
  
  ok_sourcex = false;
  ok_sourcey = false;
  
  ok_source_bb_spl = false;
  ok_source_bb_freq = false;
  ok_source_bb_bw = false;
  got_source_bb_info = false;

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
  gsl_rng_set(gsl_rng_1, (long unsigned int) MOOSTime());
  start_time = MOOSTime();
  frame_number = 0;
  noise_profile = false;
  noise_direct = false;
  //Flag indicating whether to use 3D Isotropic Ambient Noise Model. Default false
  ISOTROPIC=false;

  // random number seed
  iseed = -123456;
  nl_prev = -1000;
  array_prev_pitch = -1000;

  minimum_frequency = 0;
  maximum_frequency = 1e20;
  noise_cov_built = false;

  // prepare Bessel functions for noise covariances

   compCov.besgen.initBessel(2,40*M_PI);

   compCov.spherical_besgen.initSphBes(2,40*M_PI);

  // testing
   /*
    MOOSTrace("J_%d(%f) = %f \n",0,1.0,compCov.besgen.bessel_j(0,1.0));
    MOOSTrace("J_%d(%f) = %f \n",0,5.0,compCov.besgen.bessel_j(0,5.0));
    MOOSTrace("J_%d(%f) = %f \n",1,1.0,compCov.besgen.bessel_j(1,1.0));
    MOOSTrace("J_%d(%f) = %f \n",1,5.0,compCov.besgen.bessel_j(1,5.0));
    MOOSTrace("J_%d(%f) = %f \n",2,1.0,compCov.besgen.bessel_j(2,1.0));
    MOOSTrace("J_%d(%f) = %f \n",2,5.0,compCov.besgen.bessel_j(2,5.0));
   */
}

CSimPassiveSonar::~CSimPassiveSonar()
{
}

bool CSimPassiveSonar::OnStartUp()
{
  string sVal;

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
    noise_std = pow(10, noise_level/20)*pow(10, -6)*sqrt(sampling_frequency*0.5);//Correction AJP 3 NOV 2010 (Generate background noise with the correct spectral level.)
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
  noise_timeseries = gsl_matrix_calloc (num_array_elements,num_samples);
  tones_timeseries = gsl_matrix_calloc (num_array_elements,num_samples);
  bb_timeseries = gsl_matrix_calloc (num_array_elements,num_samples);
  if (vsa)
    {
      //Vector components
      sum_of_timeseries_x = gsl_matrix_calloc (num_array_elements,num_samples);
      noise_timeseries_x = gsl_matrix_calloc (num_array_elements,num_samples);
      tones_timeseries_x = gsl_matrix_calloc (num_array_elements,num_samples);
      bb_timeseries_x = gsl_matrix_calloc (num_array_elements,num_samples);
      sum_of_timeseries_y = gsl_matrix_calloc (num_array_elements,num_samples);
      noise_timeseries_y = gsl_matrix_calloc (num_array_elements,num_samples);
      tones_timeseries_y = gsl_matrix_calloc (num_array_elements,num_samples);
      bb_timeseries_y = gsl_matrix_calloc (num_array_elements,num_samples);
      sum_of_timeseries_z = gsl_matrix_calloc (num_array_elements,num_samples);
      noise_timeseries_z = gsl_matrix_calloc (num_array_elements,num_samples);
      tones_timeseries_z = gsl_matrix_calloc (num_array_elements,num_samples);
      bb_timeseries_z = gsl_matrix_calloc (num_array_elements,num_samples);
    }
  return(true);
}

bool CSimPassiveSonar::OnConnectToServer()
{
  m_Comms.Register( "ARRAY_X",0);
  m_Comms.Register( "ARRAY_Y",0);
  m_Comms.Register( "ARRAY_Z",0);
  m_Comms.Register( "ARRAY_HEADING",0);
  m_Comms.Register( "ARRAY_PITCH",0);

  m_Comms.Register( "TARGET_SIM_LIST",0);
  m_Comms.Register( "AMBIENT_NL_DEPTH",0);
  m_Comms.Register( "NOISE_VERT_DIR",0);



  return(true);
}

bool CSimPassiveSonar::Iterate()
{

  //  MOOSTrace("Iterate \n ");
  got_array_info = (ok_hydx && ok_hydy 
      && ok_hydz);

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
    }

    // If we have the array's information, generate timeseries and write files 
    if(got_array_info)
    {
      //Set all timeseries matricies to zero.
      gsl_matrix_set_zero(sum_of_timeseries); 
      gsl_matrix_set_zero(noise_timeseries); 
      gsl_matrix_set_zero(tones_timeseries); 
      gsl_matrix_set_zero(bb_timeseries); 

      if (vsa)
	{
	  gsl_matrix_set_zero(sum_of_timeseries_x);
	  gsl_matrix_set_zero(noise_timeseries_x); 
	  gsl_matrix_set_zero(tones_timeseries_x); 
	  gsl_matrix_set_zero(bb_timeseries_x); 
	  gsl_matrix_set_zero(sum_of_timeseries_y); 
	  gsl_matrix_set_zero(noise_timeseries_y); 
	  gsl_matrix_set_zero(tones_timeseries_y); 
	  gsl_matrix_set_zero(bb_timeseries_y); 
	  gsl_matrix_set_zero(sum_of_timeseries_z); 
	  gsl_matrix_set_zero(noise_timeseries_z); 
	  gsl_matrix_set_zero(tones_timeseries_z); 
	  gsl_matrix_set_zero(bb_timeseries_z); 
	}
      // calculate the hydrophone positions every iteration in case tow position
      // and relative hydrophone locations are published asychronously.
      for (int i=0; i < num_array_elements; i++)
      {
        array_x.at(i) = array_x_rel.at(i);
        array_y.at(i) = array_y_rel.at(i);
        array_z.at(i) = array_z_rel.at(i);
      }

  // get the noise if profile is available
  if (noise_profile)
    {
      double nl = getAmbientNoise(-ael_z, 
				  depth_nl,noise_db); 
      MOOSTrace("Local ambient noise in dB: %f \n",nl);
      //noise_std = pow(10, nl/20)*pow(10, -6);
      noise_std = pow(10, nl/20)*pow(10, -6)*sqrt(sampling_frequency*0.5);  //Correction AJP 3 NOV 2010 (Generate background noise with the correct spectral level.)
    }

  // if non-white noise generate via covariances
  if (noise_direct || ISOTROPIC)
    {
      if (ISOTROPIC)
	  MOOSTrace("Generating 3D Isotropic Noise\n");
      generate_corr_noise_timeseries();
    }
  else
    {
      generate_noise_timeseries();
    }
      // Sum up the pressure from the noise, tones, and bb source.

      gsl_matrix_add (sum_of_timeseries, noise_timeseries);
      if (vsa)
	{
	  gsl_matrix_add (sum_of_timeseries_x, noise_timeseries_x);
	  gsl_matrix_add (sum_of_timeseries_y, noise_timeseries_y);
	  gsl_matrix_add (sum_of_timeseries_z, noise_timeseries_z);
	}

      for (int i=0; i<number_of_targets; i++)
	{
	  
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
	      // Get the closest eigenray bundle
	      eigenray_table.getEigenRays(range_req, depth_req, range_ref, depth_ref, eig);
	      if (ray_acoustics)
		{
		  MOOSTrace("Target range= %f, Raytracing range = %f\n", range_req,eig.range);
		  MOOSTrace("Receiver depth= %f, Raytracing depth = %f\n", depth_req,eig.receiver_depth);
		  MOOSTrace("Number of eigenrays: %d\n",eig.num_rays);
		  tl_est = 1e-30;
		  for (int iray = 0; iray < eig.num_rays; iray++)
		      tl_est += pow(eig.bundle[iray].amp,2);
		  tl_est = - 10.0 * log10(tl_est/eig.num_rays);
		  MOOSTrace("Incoherent TL = %f\n", tl_est);

		  if (fabs(range_req - eig.range) > raytracing_interval)
		    requestRayTrace(Targets[i].target_id(), range_req, depth_req, source_bb_freq);
		}
	      
	      generate_tones_timeseries();
	      generate_bb_timeseries();
	      // Hydrophones
	      gsl_matrix_add (sum_of_timeseries, tones_timeseries);
	      gsl_matrix_add (sum_of_timeseries, bb_timeseries);
	      if (vsa)
		{
		  // Vector components
		  gsl_matrix_add (sum_of_timeseries_x, tones_timeseries_x);
		  gsl_matrix_add (sum_of_timeseries_x, bb_timeseries_x);
		  gsl_matrix_add (sum_of_timeseries_y, tones_timeseries_y);
		  gsl_matrix_add (sum_of_timeseries_y, bb_timeseries_y);
		  gsl_matrix_add (sum_of_timeseries_z, tones_timeseries_z);
		  gsl_matrix_add (sum_of_timeseries_z, bb_timeseries_z);
		}
	      icnt_rot = int( (t_now-start_time)/7 )%number_of_targets;
	      if(got_source_loc && i == icnt_rot)
		{
		  // Publish simulated target position rotating through targets
		  m_Comms.Notify("TARGET_XPOS",source_pos_x);
		  m_Comms.Notify("TARGET_YPOS",source_pos_y);
		}
	    }
	  else if (ray_acoustics)
	    requestRayTrace(Targets[i].target_id(), range_req, depth_req, source_bb_freq);
	}
      
      MOOSTrace("writing data files\n");
      write_hyd_file();
      write_nas_file(t_now);
      
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
		ok_target_info[i] = true;
		//MOOSTrace("got %s\n", Msg.m_sKey.c_str());
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

bool CSimPassiveSonar::generate_noise_timeseries()
{

  double rhow = 1000;
  //  double acc_noise_std = noise_std*M_PI_2*sampling_frequency/rhow/1500.0/9.806;
  // double acc_noise_std = noise_std/(rhow*water_c*9.806 *sqrt(3));
  double acc_noise_std = noise_std/(rhow*water_c*sqrt(3));  //Removed g. AJP 11/16/2010

  for (int phone_iter=0; phone_iter < num_array_elements; phone_iter++)
    {
      for (int sample_iter=0; sample_iter < num_samples; sample_iter++)
	{
	  gsl_matrix_set(noise_timeseries, phone_iter, sample_iter, 
			 gsl_ran_gaussian(gsl_rng_1,noise_std)); 
	  if (vsa)
	    {
	      // Vector components
	      gsl_matrix_set(noise_timeseries_x, phone_iter, sample_iter, 
			     gsl_ran_gaussian(gsl_rng_1,acc_noise_std)); 
	      gsl_matrix_set(noise_timeseries_y, phone_iter, sample_iter, 
			     gsl_ran_gaussian(gsl_rng_1,acc_noise_std)); 
	      gsl_matrix_set(noise_timeseries_z, phone_iter, sample_iter, 
			     gsl_ran_gaussian(gsl_rng_1,acc_noise_std));
	    }
	}
      if (vsa)
	{
	  // differentiate 
	  double x_prev = gsl_matrix_get(noise_timeseries_x,phone_iter,0);
	  double y_prev = gsl_matrix_get(noise_timeseries_y,phone_iter,0);
	  double z_prev = gsl_matrix_get(noise_timeseries_z,phone_iter,0);
	  
	  for (int i=1; i < num_samples; i++)
	    {
	      double x_curr = gsl_matrix_get(noise_timeseries_x,phone_iter,i);
	      double y_curr = gsl_matrix_get(noise_timeseries_y,phone_iter,i);
	      double z_curr = gsl_matrix_get(noise_timeseries_z,phone_iter,i);
	      gsl_matrix_set(noise_timeseries_x, phone_iter,i,(x_curr - x_prev)*sampling_frequency);
	      gsl_matrix_set(noise_timeseries_y, phone_iter,i,(y_curr - y_prev)*sampling_frequency);
	      gsl_matrix_set(noise_timeseries_z, phone_iter,i,(z_curr - z_prev)*sampling_frequency);
	      x_prev = x_curr;
	      y_prev = y_curr;
	      z_prev = z_curr;
	    }
	  gsl_matrix_set(noise_timeseries_x, phone_iter,0,x_prev);
	  gsl_matrix_set(noise_timeseries_y, phone_iter,0,y_prev);
	  gsl_matrix_set(noise_timeseries_z, phone_iter,0,z_prev);
	}
    }
      return true;
}


bool CSimPassiveSonar::generate_tones_timeseries()
{
  // The code in this function has been optimized for readability, 
  // not efficiency.

  // We have a cylindrical wave eminating from the source, 
  // so our wave looks like: 1/sqrt(range) * cos(-(k*range - omega*time));

  if (got_source_loc && source_status )
  {
    double range_ray = eig.range;
    double depth_ray = eig.receiver_depth;
    int num_rays     = eig.num_rays;
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
      double delta_r = range - range_ray;
      double delta_dep = - array_z.at(phone_iter) - depth_ray;  
      double delta_z = - delta_dep;
      // Computing vector components in absolute coordinates
      double costh = cos(atan2(range_y, range_x));
      double sinth = sin(atan2(range_y, range_x));
      for (int i_ray=0; i_ray<num_rays; i_ray++)
	{
	  // Elevation angle projection
	  double dip_ray = eig.bundle[i_ray].dip_receiver;
	  double p_ray = eig.bundle[i_ray].amp;
	  double ph_ray = eig.bundle[i_ray].phase;
	  double tau_ray = eig.bundle[i_ray].tau;
	  double cosel = cos(dip_ray);
	  // Positive dip angle for downgoing wave => k_z neg 
	  double sinel = - sin(dip_ray);
	  double rhow = 1000;
	  // double g = 9.806; //Removed g. AJP 11/16/2010
	  
	  //To keep tone continuous across snapshots
	  double t0 = frame_number*num_samples/sampling_frequency;
	  
	  for (int sample_iter=0; sample_iter < num_samples; sample_iter++)
	    {
	      double sample_time = (double)sample_iter/sampling_frequency + t0;
	      
	      double sample_val = 0;
	      double sample_val_x = 0;
	      double sample_val_y = 0; 
	      double sample_val_z = 0;

	      for (int tone_iter=0; tone_iter < tone_freqs.size(); tone_iter++)
		{
		  double k = tone_ks.at(tone_iter);
		  double k_r = k * cosel;
		  double k_z = k * sinel;
		  double omega = 2*M_PI*tone_freqs.at(tone_iter);
		  double wt = (omega*sample_time);
		  double p_amp = p_ray*cos(-(ph_ray+k_r*delta_r+k_z*delta_z  - omega*(sample_time - tau_ray))); 
		  //double v_amp = p_ray*k* cos(-(ph_ray+k_r*delta_r+k_z*delta_dep  - omega*(sample_time - tau_ray)) + M_PI_2)/(rhow*g); 
		  double v_amp = p_ray*k* cos(-(ph_ray+k_r*delta_r+k_z*delta_dep  - omega*(sample_time - tau_ray)) - M_PI_2)/(rhow); //Changed phase from +M_PI_2 to -M_PI_2 and removed g. AJP 11/16/2010
		  sample_val   += tone_amps_Pa.at(tone_iter)*p_amp;
		  if (vsa)
		    {
		      sample_val_x += tone_amps_Pa.at(tone_iter)*v_amp*costh*cosel;
		      sample_val_y += tone_amps_Pa.at(tone_iter)*v_amp*sinth*cosel;
		      sample_val_z += tone_amps_Pa.at(tone_iter)*v_amp*sinel;
		    }
		}

	      if (i_ray > 0)
		{
		  sample_val   += gsl_matrix_get(tones_timeseries  ,phone_iter,sample_iter);
		  if (vsa)
		    {
		      double sig_x = gsl_matrix_get(tones_timeseries_x,phone_iter,sample_iter);
		      double sig_y = gsl_matrix_get(tones_timeseries_y,phone_iter,sample_iter);
		      double sig_z = gsl_matrix_get(tones_timeseries_z,phone_iter,sample_iter);
		      // projection onto physical orientations
		      sample_val_x += sig_x*vs_costh - sig_y*vs_sinth;
		      sample_val_y += (sig_x*vs_sinth + sig_y*vs_costh)*vs_cosel + sig_z*vs_sinel;
		      sample_val_z += sig_z*vs_cosel - (sig_x*vs_sinth + sig_y*vs_costh)*vs_sinel;
		    }
		}

	      gsl_matrix_set(tones_timeseries, phone_iter, sample_iter, 
			     sample_val);
	      if (vsa)
		{
		  gsl_matrix_set(tones_timeseries_x, phone_iter, sample_iter, 
				 sample_val_x); 
		  gsl_matrix_set(tones_timeseries_y, phone_iter, sample_iter, 
				 sample_val_y); 
		  gsl_matrix_set(tones_timeseries_z, phone_iter, sample_iter, 
			     sample_val_z); 
		}
	    } 
	}
      
      if(phone_iter == num_array_elements-1)
	{
        MOOSTrace(
            "Source is on\nCurrent range is %.0f m, TL is %0.f dB\n", range, 
            tl_est);
	}
    }
  }
  return true; 
}

bool CSimPassiveSonar::generate_bb_timeseries()
{
  
  // A good chunk of the stuff below could be done once the .moos file is read
  // in, or just one when new mail is recieved. But I do it here for clarity.
  
  // The basic algorithm here is to make the fourier coefficients for the 
  // source with random phases, then calculate the fourier coefficients for
  // the hydrophone with the proper phase delays, then take the ifft to 
  // generate a time series for each hydrophone. 
  
  
  if (got_source_loc && source_status && got_source_bb_info)
    {
      double f_upper = source_bb_freq + source_bb_bw/2;
      double f_lower = source_bb_freq - source_bb_bw/2;
      
      // limit f_upper and f_lower if it's something weird. 
      if (f_upper > sampling_frequency/2)
	f_upper = sampling_frequency/2;
      if (f_lower < 0 )
	f_lower = 0;
      
      vector<double> source_spect_mag(num_freqs,0); 
      vector<double> source_spect_phase(num_freqs,0); 
      
      source_spect_mag.at(0) = 0;
      source_spect_phase.at(0) = 0;
      
      for(int f_iter=0; f_iter < num_freqs; f_iter++)
	{
	  double f = f_iter/(nfft/sampling_frequency);
	  if (f < f_upper && f > f_lower)
	    {
	      // Scale by nfft, because of the definition of gsl's inverse fft.
	      //     	      source_spect_mag.at(f_iter) = source_bb_amp*nfft; 
	      // I don't think that's right should be sqrt(nfft/f_s). HS
	      // source_spect_mag.at(f_iter) = source_bb_amp*sqrt((double) nfft/sampling_frequency);
	      // or 
	      //source_spect_mag.at(f_iter) = source_bb_amp*sqrt((double) nfft); 

  	      source_spect_mag.at(f_iter) = source_bb_amp*sqrt(((double) nfft)*sampling_frequency*0.5); //Correction AJP 3 NOV 2010 (Generate broadband source with the correct spectral level.)

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
	      source_spect_phase.at(f_iter) = 2*M_PI*gsl_rng_uniform(gsl_rng_1);
	    }
	}
      
      //MOOSTrace("source_spect_phase\n");
      //print_vec(source_spect_phase);
      
      // Now we must now take the source specturm and add the appropriate phase 
      // These fourier_coeff matricies are the spectrums for each hydrophone. 
      
      double gsl_data[nfft];
      double gsl_data_x[nfft];
      double gsl_data_y[nfft];
      double gsl_data_z[nfft];

      double range_ray = eig.range;
      double depth_ray = eig.receiver_depth;
      int num_rays     = eig.num_rays;

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
	  double delta_r = range - range_ray;
	  double delta_dep = - array_z.at(phone_iter) - depth_ray;  
	  double delta_z = - delta_dep;

	  // Computing vector components in absolute coordinates
	  double costh = cos(atan2(range_y, range_x));
	  double sinth = sin(atan2(range_y, range_x));
	  for (int i=0; i<nfft; i++)
	    {
	      gsl_data[i]   = 0.0;
	      if (vsa)
		{
		  gsl_data_x[i] = 0.0;
		  gsl_data_y[i] = 0.0;
		  gsl_data_z[i] = 0.0;
		}
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
	      
	      // frequency loop
	      for(int f_iter=0; f_iter < num_freqs; f_iter++)
		{
		  double f = f_iter/(nfft/sampling_frequency);
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
		  
		  
		  gsl_data[f_iter]        += p_mag * cos(p_pha);
		  gsl_data[nfft-f_iter]   += p_mag * sin(p_pha);
		  if (vsa)
		    {
		      // vector components
		      gsl_data_x[f_iter]      += x_mag * cos(x_pha);
		      gsl_data_x[nfft-f_iter] += x_mag * sin(x_pha);
		      gsl_data_y[f_iter]      += y_mag * cos(y_pha);
		      gsl_data_y[nfft-f_iter] += y_mag * sin(y_pha);
		      gsl_data_z[f_iter]      += z_mag * cos(z_pha);
		      gsl_data_z[nfft-f_iter] += z_mag * sin(z_pha);
		    }
		}
	    }
	  
	  gsl_fft_halfcomplex_radix2_inverse (gsl_data, 1, nfft);
	  if (vsa)
	    {
	      gsl_fft_halfcomplex_radix2_inverse (gsl_data_x, 1, nfft);
	      gsl_fft_halfcomplex_radix2_inverse (gsl_data_y, 1, nfft);
	      gsl_fft_halfcomplex_radix2_inverse (gsl_data_z, 1, nfft);
	    }
	  for(int sample_iter=0; sample_iter < num_samples; sample_iter++)
	    {
	      gsl_matrix_set(bb_timeseries, phone_iter, sample_iter, 
			     gsl_data[sample_iter]);
	      if (vsa)
		{
		  // projection onto physical orientations
		  double sig_x = gsl_data_x[sample_iter];
		  double sig_y = gsl_data_y[sample_iter];
		  double sig_z = gsl_data_z[sample_iter];

		  gsl_data_x[sample_iter]= sig_x*vs_costh - sig_y*vs_sinth;
		  gsl_data_y[sample_iter]= (sig_x*vs_sinth + sig_y*vs_costh)*vs_cosel + sig_z*vs_sinel;
		  gsl_data_z[sample_iter]= sig_z*vs_cosel - (sig_x*vs_sinth + sig_y*vs_costh)*vs_sinel;

		  gsl_matrix_set(bb_timeseries_x, phone_iter, sample_iter, 
				 gsl_data_x[sample_iter]);
		  gsl_matrix_set(bb_timeseries_y, phone_iter, sample_iter, 
				 gsl_data_y[sample_iter]);
		  gsl_matrix_set(bb_timeseries_z, phone_iter, sample_iter, 
				 gsl_data_z[sample_iter]);
		}
	    }
	}
    }
  
  return true;
}

bool CSimPassiveSonar::generate_corr_noise_timeseries()
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
      //double f = f_iter/(nfft/sampling_frequency);  //Appears to be unused, AJP
            
      vector<complex<double> > realization;
      noise_covar[f_iter].realization(realization);
      
      if (debug && f_iter == 2000)
	{
	  vector<complex<double> > chollow;
	  double frq;
	  noise_covar[f_iter].getCholLow(num_sensors, frq, chollow);
	  for (int i=1; i<num_sensors; i++)
	    for (int j=1; j<=i; j++)
	      {
		stringstream ss;
		ss << "chol(" << i << ", " << j << ")= " 
		   << chollow[i*(i+1)/2-1-(i-j)];
		string str = ss.str();
		MOOSTrace("f_iter = %d, %s \n",f_iter, str.c_str());
	      }
	}
      //      MOOSTrace("got realization\n");

      for (int sensor_iter=0; sensor_iter< num_sensors; sensor_iter++)
	{
	    noise_realization.push_back(realization[sensor_iter]);

	    if (debug && f_iter == 2000)
	      {
		stringstream ss;
		ss << "trf(" << sensor_iter+1 << ")= " << realization[sensor_iter];
		string str = ss.str();
		MOOSTrace("f_iter = %d, %s \n",f_iter, str.c_str());
	      }
	}
    }
  MOOSTrace("Realizations done\n");
 
  double gsl_data[nfft];
  double gsl_data_x[nfft];
  double gsl_data_y[nfft];
  double gsl_data_z[nfft];
  
  
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

      for (int i=0; i<nfft; i++)
	{
	  gsl_data[i]   = 0.0;
	  if (vsa)
	    {
	      gsl_data_x[i] = 0.0;
	      gsl_data_y[i] = 0.0;
	      gsl_data_z[i] = 0.0;
	    }
	}
      
      for(int f_iter=1; f_iter < num_freqs; f_iter++)
	{
	  int indx = elem_iter*sensors_per_element + num_sensors * f_iter;
	  gsl_data[f_iter]        += noise_realization[indx].real();
	  gsl_data[nfft-f_iter]   += noise_realization[indx].imag();
	  if (vsa )
	    {
	      // x component
	      indx = elem_iter*sensors_per_element + 1 + num_sensors * f_iter;
	      gsl_data_x[f_iter]        += noise_realization[indx].real();
	      gsl_data_x[nfft-f_iter]   += noise_realization[indx].imag();
	      // y component
	      indx = elem_iter*sensors_per_element + 2 + num_sensors * f_iter;
	      gsl_data_y[f_iter]        += noise_realization[indx].real();
	      gsl_data_y[nfft-f_iter]   += noise_realization[indx].imag();
	      // z component
	      indx = elem_iter*sensors_per_element + 3 + num_sensors * f_iter;
	      gsl_data_z[f_iter]        += noise_realization[indx].real();
	      gsl_data_z[nfft-f_iter]   += noise_realization[indx].imag();

	    }


	}
	  
      gsl_fft_halfcomplex_radix2_inverse (gsl_data, 1, nfft);
      if (vsa)
	{
	  gsl_fft_halfcomplex_radix2_inverse (gsl_data_x, 1, nfft);
	  gsl_fft_halfcomplex_radix2_inverse (gsl_data_y, 1, nfft);
	  gsl_fft_halfcomplex_radix2_inverse (gsl_data_z, 1, nfft);
	}
  
      for(int sample_iter=0; sample_iter < num_samples; sample_iter++)
	{
	  gsl_matrix_set(noise_timeseries, elem_iter, sample_iter, 
		     gsl_data[sample_iter]);
	  if (vsa)
	    {
	      // projection onto physical orientations
	      double sig_x = gsl_data_x[sample_iter];
	      double sig_y = gsl_data_y[sample_iter];
	      double sig_z = gsl_data_z[sample_iter];
	      // surface noise generated in local horizontal
	      // array coordinates. Projection only in vertical.
	      gsl_data_x[sample_iter]= sig_x;
	      gsl_data_y[sample_iter]= sig_y*vs_cosel 
		+ sig_z*vs_sinel;
	      gsl_data_z[sample_iter]= sig_z*vs_cosel 
		- sig_y*vs_sinel;
	      
	      gsl_matrix_set(noise_timeseries_x, elem_iter, sample_iter, 
			     gsl_data_x[sample_iter]);
	      gsl_matrix_set(noise_timeseries_y, elem_iter, sample_iter, 
			     gsl_data_y[sample_iter]);
	      gsl_matrix_set(noise_timeseries_z, elem_iter, sample_iter, 
			     gsl_data_z[sample_iter]);
	    }
	}
    }
  
  return true;
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
      
      if(array_type == "DURIP")
	{
	  // For DURIP, print out first and last sensors' info only.
	  fprintf (nas_data_file, "%8.4f %8.4f 0.0 %8.4f\n", array_heading.at(0), array_pitch.at(0), fabs(array_z.at(0)));
	  fprintf (nas_data_file, "%8.4f %8.4f 0.0 %8.4f\n", array_heading.at(num_array_elements-1), 
		   array_pitch.at(num_array_elements-1), fabs(array_z.at(num_array_elements-1)));
	}
      
      fclose (nas_data_file);
      MOOSTrace("Wrote file: %s \n", nonacoustic_filename_buff);
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
      source_bb_amp = 1e-6*pow(10,source_bb_spl/20);
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
          tone_amps_Pa.push_back(pow(10, Targets[i].tone_dbl[j]/20)*pow(10, -6));
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
  new_contact.ready = true;
  new_contact.ray_table = eig_table;
  acoustic_contacts.push_back(new_contact);
  MOOSTrace("Contact %s eigenrays updated\n",cn.c_str());
  MOOSTrace("Number of ranges: %d\n",eig_table.num_ranges);
  MOOSTrace("Number of depths: %d\n",eig_table.num_depths);
  return(true);
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

bool CSimPassiveSonar::requestRayTrace(std::string cn, 
					double range, double depth, double freq)
{

  for (int i = 0; i < acoustic_contacts.size(); i++)
    {
      if (MOOSStrCmp(acoustic_contacts[i].contact,cn))
	{
	  // if we waited more than 30 secs, iBellhop must have missed request
	  if (readyAcousticContact(cn) || (MOOSTime() - acoustic_contacts[i].last_request) > 30.0 )
	    {
	      // Request raytracing
	      string acoustic_contact = cn + "_SIM";

	      double depth1 = depth - 0.5 * tan(array_avg_pitch)
					   * raytracing_interval;
	      double depth2 = depth + 0.5 * tan(array_avg_pitch)
					   * raytracing_interval;
	      double range1 = range - 0.5 * raytracing_interval;
	      double range2 = range + 0.5 * raytracing_interval;

	      double direct_theta = atan2(depth,range)*180.0/M_PI;
	      double theta_max = 60.0;
	      if (direct_theta > 0.5*theta_max)
		theta_max = 0.5*(direct_theta+90.0);


	      std::stringstream ss;

	      ss << "env { \n" 
		 << " adaptive_info { \n"
		 << "   contact: \"" << acoustic_contact << "\"\n"
		 << "   look_ahead_seconds: " << 1.0 << "\n"
		 << "   read_shd: false \n"
		 << "   auto_receiver_ranges: false \n"
		 << "   auto_source_depth: true\n"
		 << "  } \n"
		 << " output { \n"
		 << "  type: ARRIVAL_TIMES \n"
		 << "  } \n"
		 << " freq: " << freq
		 << " receivers { \n"
		 << "   number_in_depth:" << 2 << "\n"
		 << "   number_in_range:" << 2 << "\n"
		 << "   first {\n"
		 << "     range:" << range1 << "\n"
		 << "     depth:" << depth1 << "\n"
		 << "    } \n"
		 << "   last {\n"
		 << "     range:"  << range2 << "\n" 
		 << "     depth:"  << depth2 << "\n"
		 << "    } \n"
		 << "  }\n"  
		 << " beams { \n"
		 << "   theta_max:"      << int(theta_max) << "\n" 
		 << "   theta_min:"      << int(-theta_max) << "\n"
		 << "  } \n"
		 << " } \n";
	      /*
	      ss << "env.adaptive_info.contact=" << acoustic_contact
		 << ",env.adaptive_info.look_ahead_seconds=" << 1.0
		 << ",env.adaptive_info.read_shd=false"
		 << ",env.adaptive_info.auto_receiver_ranges=false"
		 << ",env.adaptive_info.auto_source_depth=true"
		 << ",env.output.type=arrival_times"
		 << ",env.freq=" << freq
		 << ",env.receivers.number_in_depth=" << 2
		 << ",env.receivers.number_in_range=" << 2 
		 << ",env.receivers.first.range=" << range1
		 << ",env.receivers.last.range="  << range2 
		 << ",env.receivers.first.depth=" << depth1
		 << ",env.receivers.last.depth="  << depth2 
		 << ",env.beams.theta_max="       << int(theta_max) 
		 << ",env.beams.theta_min="       << int(-theta_max) ;
	      */
	      m_Comms.Notify("BELLHOP_REQUEST", ss.str());                 
	      string str = ss.str();
	      MOOSTrace("Requesting raytracing: %s\n",str.c_str());
	      // Wait for Bellhop before making next request
	      
	      acoustic_contacts[i].ready = false;
	      acoustic_contacts[i].last_request = MOOSTime();	    
	    } 
	  return(true);
	}
    }
  return(false);
}

double CSimPassiveSonar::getAmbientNoise(double depth, 
					  vector<double> dom_nl,
					  vector<double> noise_db)
{
  double nl;
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
  // generate isotropic noise covariance matrix
  // first time, following ones just scaled, avoiding cholesky factorization
  //
  if (noise_direct)
    {

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
	      double a_intens = p_intens*pow(k,2)
		/(3*pow(rho,2));
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
	  double amp_fac = pow(10.0,(nl-nl_prev)/20.0);
	  for (int k = 0; k< num_freqs; k++)
	    {
	      noise_covar[k].scaleCovMat(amp_fac);
	    }
	  MOOSTrace("Scaled covariance matrices, factor = %f\n",amp_fac);
	}
      nl_prev = nl;

      // if pitch has changed more than 5 deg recompute 
      // covariances for frequency interval of interest      

      if (fabs(array_avg_pitch-array_prev_pitch) 
	  >= 5.0*M_PI/180.0 )
	{    
	  MOOSTrace("Building correlated noise covariances, i_first=%d, i_last=%d\n", i_first,i_last);

	  vector<double> delta_r;
	  vector<double> delta_z;

	  vector<double> delta_y;
	  vector<double> delta_x;
	  vector<double> delta_R;
	  for (int j = 0; j < num_array_elements; j++)
	    for (int i = 0; i < num_array_elements; i++)
	      {
		//int indx = i + num_array_elements * j;		//Unused AJP
		//delta_r.push_back(sqrt(pow(array_x[i]-array_x[j],2) + pow(array_y[i]-array_y[j],2))); //Horizontal distance between sensor i and sensor j
		delta_z.push_back(array_z[i] - array_z[j]);
		
		if (ISOTROPIC)
		  {
		    delta_x.push_back(array_x[i] - array_x[j]);
		    delta_y.push_back(array_y[i] - array_y[j]);
		    delta_R.push_back(sqrt(pow(array_x[i]-array_x[j],2) + pow(array_y[i]-array_y[j],2) +  pow(array_z[i]-array_z[j],2))); //Distance between sensor i and sensor j
		  }
		else
		  {
		    delta_r.push_back(sqrt(pow(array_x[i]-array_x[j],2) + pow(array_y[i]-array_y[j],2))); //Horizontal distance between sensor i and sensor j
		  }
	      }

	  for (int k = i_first; k< i_last; k++)
	    {
	      double freq = k*delta_f; 
	      vector<complex<double> > cov;
	      CovMat newcov;
	      
	      cov.clear();
	      cov = vecUtils.zzeros(ndim,ndim);
	      if (k > 0)
		{
		  if (debug)
		      MOOSTrace("Before initialize(), frequency index = %d, water_c = %lf m/s, freq = %lf Hz\n",k,water_c,freq);
		  compCov.initialize(freq,water_c,theta,S);
		  double norm = 0;
		  if (vsa)
		    {
		      for (int i = 0; i < num_array_elements; i++)
			{
			  for (int j = 0; j <= i; j++)
			    {
			      vector<complex<double> > vsa_cov;
			      int indx = i + num_array_elements * j;
			      if (ISOTROPIC)
				  compCov.vsaIsotropicCovariance(delta_R[indx], delta_x[indx], delta_y[indx], delta_z[indx], vsa_cov);
			      else
			      	  compCov.vsaCovariance(delta_r[indx], delta_z[indx],vsa_cov);
			      
			      for (int jj=0; jj<4; jj++)
				{
				  int indx_vsa = 4*i + ndim * (4*j+jj);
				  for (int ii=0; ii<4; ii++)
				    cov[indx_vsa+ii] = vsa_cov[ii+jj*4];
				}
			    }

			  norm += cov[4*i*(1+ndim)].real();
			  if (debug2 && k == 2000)
			  	MOOSTrace("cov[%d].real=%lf, norm=%lf\n", 4*i*(1+ndim),cov[4*i*(1+ndim)].real(),norm);
			}
		      norm /= num_array_elements;
		      if (debug2 && k == 2000)
			  MOOSTrace("Final norm=%lf\n", norm);

		    }
		  else
		    {
		      for (int i = 0; i < num_array_elements; i++)
			{
			  for (int j = 0; j <= i; j++)
			    {
			      int indx = i + num_array_elements * j;
			      complex<double> hyd_cov;
			      if (ISOTROPIC)
			          compCov.hydIsotropicCovariance(delta_R[indx], hyd_cov);
			      else
			          compCov.hydCovariance(delta_r[indx], delta_z[indx],hyd_cov);
			      cov[indx] = hyd_cov;
			    }
			  norm += cov[i*(1+ndim)].real();
			}
		      norm /= ndim;
		    }
		  //		  MOOSTrace("freq=%f, scaling norm =%f, p_intens=%f\n",freq,norm,p_intens);
		  // now scale covariances
		  double scale = p_intens/norm;
		  if (debug2 && k == 2000)
		      MOOSTrace("p_intens=%lf, scale=%lf\n", p_intens,scale);
		  for (int i=0; i<ndim*ndim; i++)
		    cov[i] *= scale;
		  
		  if (debug2 && (k==500 || k==2000))
		    {
		      for (int i = 0; i < 4; i++)
			{
			      int j=i;
			      int indx = i + ndim* j;
			      stringstream ss;
			      ss << "cov(" << i+1 << "," << j+1 << ")= " << cov[indx];
			      string str = ss.str();
			      MOOSTrace("f = %f, %s \n",freq, str.c_str());
			}
		    }
		  if (debug && k == 2000)
		    {
		      for (int i = 0; i < ndim; i++)
			{
			  for (int j = 0; j <= i; j++)
			    {
			      int indx = i + ndim* j;
			      stringstream ss;
			      ss << "cov(" << i+1 << "," << j+1 << ")= " << cov[indx];
			      string str = ss.str();
			      MOOSTrace("f = %f, %s \n",freq, str.c_str());
			    }
			}
		    }
		}
	      newcov.putCovMat(ndim, freq, cov);
	      noise_covar[k] = newcov;
	    }
	  MOOSTrace("Built covariance matrices, pitch=%f degrees\n",
		    array_avg_pitch*180.0/M_PI);
	  array_prev_pitch = array_avg_pitch;
	}
    }
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
  goby::util::val_from_string(depths_nl, noise_in, "depths");
  goby::util::val_from_string(nls, noise_in, "NL");
  
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
  goby::util::val_from_string(depths_nl, noise_in, "theta");
  goby::util::val_from_string(nls, noise_in, "S");
  
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

// 
// ComputeCovariance class
// Computes covariances for hydrophone or vector sensor pairs
// using angular integrals following Andrew Poulsen, 
// Oct. 1, 2009.
// Note that the azimuthally symmetric nose is computed in
// local horizontal array coordinates, i.e. 
// delta_x = 0
// delta_y = R_h
bool ComputeCovariance::initialize(double freq, 
				   double sound_speed, 
				   vector<double> theta, 
				   vector<double> Sin)
{ 
  // sets up the vertical wavenumber intensity distribution
  // to be used by integrals
  c = sound_speed;
  rho = 1000;
  omega = 2 * M_PI * freq;
  k = omega/c;
  n_th = theta.size();
  k_z.clear();
  k_r.clear();
  S.clear();
  for (unsigned int i = 0; i<n_th; i++)
    {
      k_z.push_back(k*cos(theta[i]));
      k_r.push_back(k*sin(theta[i]));
      S.push_back(Sin[i]);
    } 
  
  return(true);
}

bool ComputeCovariance::hydCovariance(double delta_R, 
				      double delta_z, 
				      complex<double> & cov)
{

  cov = complex<double>(0.0,0.0);
  double pfac = -0.5/k; 

  vector<complex<double> > cfun;
  vector<complex<double> > cex;
  vector<double> arg;

  for (unsigned int i = 0; i<n_th; i++)
    {
      cfun.push_back(pfac * S[i]
		     *besgen.bessel_J(0,k_r[i]*delta_R));

      cex.push_back(complex<double>(cos(k_z[i]*delta_z),
				    sin(k_z[i]*delta_z)));
      arg.push_back(k_z[i]*delta_z);
    }
  cov = filon(k_z, cfun, cex, arg);
  return(true);
}

/*
bool ComputeCovariance::hydCovariance(double delta_R, 
				      double delta_z, 
				      complex<double> & cov)
{
  cov = complex<double>(0.0,0.0);
  double fac = -0.5/k;
  complex<double> delta_cov = fac*S[0]
    *besgen.bessel_J(0,k_r[0]*delta_R)
    *complex<double>(cos(k_z[0]*delta_z),
		     sin(k_z[0]*delta_z));
  for (unsigned int i = 1; i<n_th; i++)
    {
      complex<double> delta_prev = delta_cov;        
      delta_cov = fac*S[i]
	*besgen.bessel_J(0,k_r[i]*delta_R)
	*complex<double>(cos(k_z[i]*delta_z),
			 sin(k_z[i]*delta_z));
      cov += 0.5*(delta_prev + delta_cov) * (k_z[i]-k_z[i-1]); 
    } 
  return(true);
}
*/
bool ComputeCovariance::vsaCovariance(double delta_R, 
				      double delta_z,
				      vector<complex<double> >& cov)
{
  if (cov.size() != 16)
    {
      cov.clear();
      for (unsigned int i=0; i<16; i++)
	cov.push_back(complex<double>(0,0));
    }

  vector<complex<double> > cex;
  vector<double> arg;
  for (unsigned int i = 0; i<n_th; i++)
    {
      cex.push_back(complex<double>(cos(k_z[i]*delta_z),
				    sin(k_z[i]*delta_z)));
      arg.push_back(k_z[i]*delta_z);
    }

  // p*p'
  {
    double pfac = -0.5/k; 
    
    vector<complex<double> > cfun;
    
    for (unsigned int i = 0; i<n_th; i++)
      {
	cfun.push_back(pfac * S[i]
		       *besgen.bessel_J(0,k_r[i]*delta_R));
      }
    cov[indx(1,1)] = filon(k_z, cfun, cex, arg);
  }
  // ax*p' is 0
  //   
  // ay*p'
  if (fabs(delta_R) > 1e-3)
    {
      double fac = -0.5/(rho*k);
      vector<complex<double> > cfun;
      
      for (unsigned int i = 0; i<n_th; i++)
	{
	  cfun.push_back(fac * S[i] *k_r[i]
			 *besgen.bessel_J(1,k_r[i]*delta_R));
	}
      cov[indx(3,1)] = filon(k_z, cfun, cex, arg);
      cov[indx(1,3)] = -cov[indx(3,1)];
    }
  //   
  // az*p'
  {
    complex<double> fac = 0.5
      *complex<double>(0,1)/(rho*k);
    vector<complex<double> > cfun;
    
    for (unsigned int i = 0; i<n_th; i++)
      {
	cfun.push_back(fac * S[i] *k_z[i]
		       *besgen.bessel_J(0,k_r[i]*delta_R));
      }
    cov[indx(4,1)] = filon(k_z, cfun, cex, arg);
    cov[indx(1,4)] = -cov[indx(4,1)];
  }
  //   
  // ay*ax'=0
  //   
  // az*ax'=0
  //   
  // az*ay'
  if (fabs(delta_R) > 1e-3)
    {
      complex<double> fac = -0.5*complex<double>(0,1)/(pow(rho,2)*k);
      vector<complex<double> > cfun;
      
      for (unsigned int i = 0; i<n_th; i++)
	{
	  cfun.push_back(fac * S[i] *k_r[i]*k_z[i]
			 *besgen.bessel_J(1,k_r[i]*delta_R));
	}
      cov[indx(4,3)] = filon(k_z, cfun, cex, arg);
      cov[indx(3,4)] = cov[indx(4,3)];
    }
  //   
  // ax*ax'
  if (fabs(delta_R) > 1e-3)
    {
      double fac = -0.5/(pow(rho,2)*k*delta_R);
      vector<complex<double> > cfun;
      
      for (unsigned int i = 0; i<n_th; i++)
	{
	  cfun.push_back(fac * S[i] *k_r[i]
			 *besgen.bessel_J(1,k_r[i]*delta_R));
	}
      cov[indx(2,2)] = filon(k_z, cfun, cex, arg);
    } 
  else
    {
      complex<double> fac = -0.25/(pow(rho,2)*k);
      vector<complex<double> > cfun;
      
      for (unsigned int i = 0; i<n_th; i++)
	{
	  cfun.push_back(fac * S[i] *pow(k_r[i],2));
	}
      cov[indx(2,2)] = filon(k_z, cfun, cex, arg);
    }
  //   
  // vy*vy'
  {
    double fac = -0.25/(pow(rho,2)*k);
    vector<complex<double> > cfun;
    
    for (unsigned int i = 0; i<n_th; i++)
      {
	cfun.push_back(fac * S[i] *pow(k_r[i],2)
		       * (besgen.bessel_J(0,k_r[i]*delta_R)
			  -besgen.bessel_J(2,k_r[i]*delta_R)));
      }
    cov[indx(3,3)] = filon(k_z, cfun, cex, arg);

  }   
  //   
  // vz*vz'
  {
    double fac = -0.5/(pow(rho,2)*k);
    vector<complex<double> > cfun;
    
    for (unsigned int i = 0; i<n_th; i++)
      {
	cfun.push_back(fac * S[i] *pow(k_z[i],2)
		       * besgen.bessel_J(0,k_r[i]*delta_R));
      }
    cov[indx(4,4)] = filon(k_z, cfun, cex, arg);
  }
  return(true);
}

complex<double> ComputeCovariance::conjg(complex<double> a)
{
  return(complex<double>(a.real(),-a.imag()));
}

int ComputeCovariance::indx(int i, int j)
{
  return((i-1)+4*(j-1));
}


bool ComputeCovariance::vsaIsotropicCovariance(double delta_R, double delta_x, double delta_y, double delta_z, vector<complex<double> >& cov)
  {
    if (cov.size() != 16)
      {
	cov.clear();
	for (unsigned int i=0; i<16; i++)
	  cov.push_back(complex<double>(0,0));
      }

    if (fabs(delta_R) > 1e-3)
      {
    	//double j0=spherical_besgen.sphbes_j(0,k*delta_R);
    	//double j1=spherical_besgen.sphbes_j(1,k*delta_R);
    	//double j2=spherical_besgen.sphbes_j(2,k*delta_R);
	double argument = k*delta_R;
    	double j0=boost::math::sph_bessel(0,argument);
	double j1=boost::math::sph_bessel(1,argument);
	double j2=boost::math::sph_bessel(2,argument);
    	//MOOSTrace("j0(%.15e)=%.15e, j1(%.15e)=%.15e, j2(%.15e)=%.15e\n",k*delta_R,j0,k*delta_R,j1,k*delta_R,j2);
    	double temp0=k/(rho*delta_R);
    	double temp1=j1*temp0;
    	double temp2=j2*pow(temp0,2); 
    	double temp3=pow(k/rho,2); 
    	double temp4=j1/(k*delta_R);

	
    	cov[indx(1,1)] = j0;
    	cov[indx(1,2)] = -delta_x*temp1;
    	cov[indx(1,3)] = -delta_y*temp1;
    	cov[indx(1,4)] = -delta_z*temp1;
    	cov[indx(2,1)] = -cov[indx(1,2)];
    	cov[indx(2,2)] = temp3*(temp4-j2*pow(delta_x/delta_R,2));
    	cov[indx(2,3)] = -delta_x*delta_y*temp2;
    	cov[indx(2,4)] = -delta_x*delta_z*temp2;
    	cov[indx(3,1)] = -cov[indx(1,3)];
    	cov[indx(3,2)] = cov[indx(2,3)];
    	cov[indx(3,3)] = temp3*(temp4-j2*pow(delta_y/delta_R,2));
    	cov[indx(3,4)] = -delta_y*delta_z*temp2;
    	cov[indx(4,1)] = -cov[indx(1,4)];
    	cov[indx(4,2)] = cov[indx(2,4)];
    	cov[indx(4,3)] = cov[indx(3,4)]; 
    	cov[indx(4,4)] = temp3*(temp4-j2*pow(delta_z/delta_R,2));
      }
    else
      {
        double temp5=pow(k/rho,2)/3.0;
	cov[indx(1,1)] = 1;
	cov[indx(2,2)] = temp5;
	cov[indx(3,3)] = temp5;
	cov[indx(4,4)] = temp5;
      }
    return(true);
  }

bool ComputeCovariance::hydIsotropicCovariance(double delta_R, complex<double> & cov)
{
  cov = spherical_besgen.sphbes_j(0,k*delta_R);

  return(true);
}

complex<double> ComputeCovariance::filon(vector<double> x,
					 vector<complex<double> > cfun,
					 vector<complex<double> > cex,
					 vector<double> arg)
{
  complex<double> ai = complex<double>(0,1);
  complex<double> csum = complex<double>(0,0); 
  int n = x.size();
  for (int i = 0; i < n-1; i++)
    {
      double dx = x[i+1]-x[i];
      if (arg[i+1] != arg[i])
	{
	  complex<double> darg = -ai/(arg[i+1]-arg[i]);
	  complex<double> dfun = cfun[i+1] - cfun[i];
	  complex<double> dex  = cex[i+1] - cex[i];
	  complex<double> dprod = cfun[i+1]*cex[i+1] 
	    - cfun[i]*cex[i];
	  csum += dx*darg*(dprod-darg*dfun*dex);
	}
      else
	csum += 0.5*dx*(cfun[i+1]*cex[i+1]+cfun[i]*cex[i]);
    }
  return(csum);
}

