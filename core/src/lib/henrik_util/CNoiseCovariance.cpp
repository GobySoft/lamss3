// H. Schmidt, henrik@mit.edu 01.20.10
// Laboratory for Autonomous Marine Sensing Systems (LAMSS)
// Massachusetts Institute of Technology 
// 
// CNoiseCovariance.cpp  
//
// see the readme file within this directory for information
// pertaining to usage and purpose of this script.
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


#include "CNoiseCovariance.h"
#include <boost/lexical_cast.hpp>
#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/math/special_functions.hpp>

using namespace std;

//
// Noise Covariance class
// Implementing Andrew Poulsens spherical harmonics representation of
// noise covariance between two hydrophones or vector sensors
// Takes as input the vertical noise directionality
//

// 
// ComputeCovariance class
// Computes covariances for hydrophone or vector sensor pairs
// using angular integrals following Andrew Poulsen, 
// Oct. 1, 2009.
// Note that the azimuthally symmetric noise is computed in
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

