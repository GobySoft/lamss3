// copyright 2011 H. Schmidt henrik@mit.edu
// 
// this file is part of lamss-shared, a collection of MOOS
// processes for underwater autonomy and acoustic simulation
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

#include "SimNoiseCovariance.h"
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

CSimNoiseCovariance::CSimNoiseCovariance()
{
  MOOSTrace("====================================\n");
  MOOSTrace("\n");
  MOOSTrace("          pSimNoiseCovariance\n");
  MOOSTrace("\n");
  MOOSTrace("          V 2.0 9/29/2010 \n");
  MOOSTrace("\n");
  MOOSTrace("         Henrik Schmidt, MIT\n");
  MOOSTrace("Sep. 2009, H. Schmidt. Multiple targets \n");
  MOOSTrace("Jul. 2010, HS.:Adapted to DSOP convention \n");   
  MOOSTrace("Sep. 2010, HS.:Interfaced to Bellhop for HiFi simulation \n");   
  MOOSTrace("====================================\n");

  // initialize a bunch of variables  

  vsa = false;

  water_c = c_0;

  ok_array_heading = false;
  ok_array_pitch = false;
  ok_hydx = false;
  ok_hydy = false;
  ok_hydz = false;
  got_array_info = false;
  
  debug = false;
  debug2 = false;

  start_time = MOOSTime();

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
  covariance_requested = false;

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

CSimNoiseCovariance::~CSimNoiseCovariance()
{
}

bool CSimNoiseCovariance::OnStartUp()
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
    MOOSTrace("pSimNoiseCovariance:: num_array_elements not specified - FAIL\n");
    exit(EXIT_FAILURE);
  }

  if(m_MissionReader.GetConfigurationParam("base_dir_name",sVal))
    {
      base_dir_name = sVal.c_str();
    }
  else
    {
      MOOSTrace("pSimNoiseCovariance:: base_dir_name not specified - FAIL\n");
      exit(EXIT_FAILURE);
    }
  
if(m_MissionReader.GetConfigurationParam("array_type",sVal))
  {
    array_type = sVal.c_str();
    vsa = (array_type == "VSA");
  }
  else
  {
    MOOSTrace("pSimNoiseCovariance:: array_type not specified - FAIL\n");
    exit(EXIT_FAILURE);
  }

  if(m_MissionReader.GetConfigurationParam("water_depth",sVal))
  {
    water_depth = atof(sVal.c_str());
  }
  else
  {
    MOOSTrace("pSimNoiseCovariance:: No water_depth - FAIL\n");
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
	MOOSTrace("pSimNoiseCovariance:: no noise_level- FAIL\n");
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
    
    return(true);
}

bool CSimNoiseCovariance::OnConnectToServer()
{
  m_Comms.Register( "ARRAY_X",0);
  m_Comms.Register( "ARRAY_Y",0);
  m_Comms.Register( "ARRAY_Z",0);
  m_Comms.Register( "ARRAY_HEADING",0);
  m_Comms.Register( "ARRAY_PITCH",0);

  m_Comms.Register( "AMBIENT_NL_DEPTH",0);
  m_Comms.Register( "NOISE_VERT_DIR",0);
  m_Comms.Register( "COVARIANCE_REQUEST",0);

  return(true);
}

bool CSimNoiseCovariance::Iterate()
{

  //  MOOSTrace("Iterate \n ");
  got_array_info = (ok_hydx && ok_hydy 
      && ok_hydz);

  // Output error messages 
  if(!got_array_info)
    {
      MOOSTrace("ARRAY_[X, Y or Z] *not* in the MOOSDB.\n");
      MOOSTrace("Waiting until all required variables are in the MOOSDB...\n\n");
    }
  else
    {  
      // calculate the hydrophone positions every iteration in case tow position
      // and relative hydrophone locations are published asychronously.
      for (int i=0; i < num_array_elements; i++)
	{
	  array_x.at(i) = array_x_rel.at(i);
	  array_y.at(i) = array_y_rel.at(i);
	  array_z.at(i) = array_z_rel.at(i);
	}
      
      // get the noise if profile is available
      if (covariance_requested)
	{
	  MOOSTrace("calling computeNoiseCovariance\n");
	  double nl = computeNoiseCovariance(-ael_z,depth_nl,noise_db); 
	  MOOSTrace("Local ambient noise in dB: %f \n",nl);
	}
    }
  return(true);
}

//-------------------------------------------------------------
// Procedure: OnNewMail

bool CSimNoiseCovariance::OnNewMail(MOOSMSG_LIST &NewMail)
{
  MOOSMSG_LIST::iterator p;
  for(p = NewMail.begin(); p != NewMail.end(); p++) 
    {
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
	    {
	      MOOSTrace("noise_direct=true\n");
	      theta.clear();
	      S.clear();
	      
	      for (int i=0; i < theta_deg.size(); i++)
		{
		  theta.push_back(theta_deg[i]*M_PI/180.0);
		  S.push_back(pow(10.0,S_dB[i]/10));
		}
	    }
	  else
    	    MOOSTrace("noise_direct=false\n");
	}	  
      else if(Msg.m_sKey == "COVARIANCE_REQUEST")
	{
	  covariance_requested = true;
	  string bite;
	  if (MOOSValFromString(bite,Msg.m_sVal,"nfft"))
	    {
	      nfft = atoi(bite.c_str());
	      if (MOOSValFromString(bite,Msg.m_sVal,"sampling_frequency"))
		sampling_frequency = atof(bite.c_str());
	      num_freqs = nfft/2 + 1; 
	      MOOSTrace("Noise covariance for %0.3f seconds of data\n", nfft/sampling_frequency);
	    }

	}
    }
  return(true);
}
  


int CSimNoiseCovariance::str2int_vec(string msg, vector<int>& values)
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

double CSimNoiseCovariance::computeNoiseCovariance(double depth, 
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

  if (noise_direct || ISOTROPIC)
    {

      int ndim;
      if (vsa)
	ndim = 4*num_array_elements;
      else
	ndim = num_array_elements;
      if (debug)
          MOOSTrace("nl=%lf, Fs=%lf, nfft=%d\n",nl,sampling_frequency,nfft);
      // 1e-12 factor removed for compatibility with active simulator
      //      double p_intens = 1e-12 * pow(10.0,nl/10) * sampling_frequency * 0.5 * nfft;
      double p_intens = pow(10.0,nl/10) * sampling_frequency * 0.5 * nfft;
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

      if (covariance_requested)
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
		delta_z.push_back(array_z[i] - array_z[j]);
		
		if (ISOTROPIC)
		  {
		    delta_x.push_back(array_x[i] - array_x[j]);
		    delta_y.push_back(array_y[i] - array_y[j]);
		    //Distance between sensor i and sensor j
		    delta_R.push_back(sqrt(pow(array_x[i]-array_x[j],2) 
					   + pow(array_y[i]-array_y[j],2) 
					   +  pow(array_z[i]-array_z[j],2))); 
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

	  for (int k = i_first; k<= i_last; k++)
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
		  
		}
	      newcov.putCovMat(ndim, freq, cov);
	      noise_covar.push_back(newcov);
	    }
	  // Write file with covariances
	  MOOSTrace("Built covariance matrices, pitch=%f degrees\n",
		    array_avg_pitch*180.0/M_PI);
	  write_covmat_file(MOOSTime(), nl, i_first, i_last);

	  covariance_requested = false;
	}
    }
  return(nl);
}

bool CSimNoiseCovariance::parseNoiseProfile( string noise_in, 
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

bool CSimNoiseCovariance::parseNoiseDirectionality( string noise_in, 
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

  if ( !val_from_string(nls, noise_in, "S"))
    return(false);
  
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

bool CSimNoiseCovariance::write_covmat_file(double t_now, double nl_now, int i_first, int i_last)
{
  char covmat_filename_buff[1000]; 
  string covmat_name = "NoiseCovariance.cov";
  // Sure hope nobody wants a filename longer than 1000 chars
  sprintf(covmat_filename_buff,"%s/%s",base_dir_name.c_str(),covmat_name.c_str());
  
  FILE * cov_data_file;
  if ( (cov_data_file = fopen(covmat_filename_buff, "wb")) == NULL)
    {
      MOOSTrace("\nUnable to open a file:  %s \nFAIL\n", covmat_filename_buff);
      MOOSTrace("Does the directory exist?\nDoes base_dir_name begin and end with a forward slash?\nDo you have permission to write to that directory?\n\n");
      exit(EXIT_FAILURE);
    }
  else
    {
      // Compute correlated noise only for spectrum of 
      // interest
      double delta_f = sampling_frequency/nfft;

      fwrite(&t_now, sizeof(double), 1 , cov_data_file);
      fwrite(&nl_now, sizeof(double), 1 , cov_data_file);
      fwrite(&minimum_frequency, sizeof(double), 1 , cov_data_file);
      fwrite(&minimum_frequency, sizeof(double), 1 , cov_data_file);
      fwrite(&delta_f, sizeof(double), 1 , cov_data_file);
      fwrite(&i_first, sizeof(int), 1 , cov_data_file);
      fwrite(&i_last, sizeof(int), 1 , cov_data_file);

      int num_sensors;
      if (vsa)
	num_sensors = 4*num_array_elements;
      else
	num_sensors = num_array_elements;
      fwrite(&num_sensors, sizeof(int), 1 , cov_data_file);

      for (int f_iter=i_first; f_iter <= i_last; f_iter++)
	{
	  
	  vector<complex<double> > covmat;
	  vector<complex<double> > chollow;
	  double f;

	  noise_covar[f_iter-i_first].getCovLow(num_sensors, f, covmat);
	  noise_covar[f_iter-i_first].getCholLow(num_sensors, f, chollow);
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
      MOOSTrace("Wrote file: %s \n", covmat_filename_buff);
      m_Comms.Notify("COVARIANCE_FILE",base_dir_name + "/" + covmat_name);
    }
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

