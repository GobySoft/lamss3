// H. Schmidt, henrik@mit.edu 07.02.10
// Laboratory for Autonomous Marine Sensing Systems (LAMSS)
// Massachusetts Institute of Technology 
// 
// CRapRays.cpp  
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


#include "CRapRays.h"

using namespace std;

bool CRapRays::setEnv(double c_surface, double c_gradient, 
		      double water_depth, double z_rap_max)
{
  c_0 = c_surface;
  c_grad = c_gradient;
  z_max = z_rap_max;
  w_depth = water_depth;

  return(true);
}
 
bool CRapRays::rapRay(double x0, double y0, double z0, double x, double y, double z, double& theta0, double& p_tl, double& tau, bool& turn)
{
  double eps = 1e-3;
  double r = sqrt(pow(x-x0,2)+pow(y-y0,2));
  double c_s = (c_0 + c_grad * z0);

  // MOOSTrace("Entering rapRay \n");

  if (c_grad == 0)
    {
      theta0 = atan2(z-z0,r)*180.0/M_PI;
      double s = sqrt(pow(r,2) + pow (z-z0,2));
      p_tl = 1.0/s;
      tau = s/c_0;
      return(true);
    }
  else
    {
      double th0_0 = 0.1;
      double cth0 = cos(th0_0);
      double sth0 = sin(th0_0);
      double R = c_s/(c_grad * cth0);
      double sth = (R* sth0 -r)/R;
      double cth = sqrt(1.0-pow(sth,2));
      double z_0 =  R*cth - c_0 /c_grad;
      
      double th0_1 = 0.2;
      cth0 = cos(th0_1);
      sth0 = sin(th0_1);
      R = c_s/(c_grad * cth0);
      sth = (R* sth0 -r)/R;
      cth = sqrt(1.0-pow(sth,2));
      double z_1 =  R*cth - c_0 /c_grad;
      
      int i = 1;

      while (fabs(z-z_1) > eps && fabs(th0_1 - th0_0) > eps && i < 10)
	{
	  double dzdt = (z_1 - z_0)/(th0_1 - th0_0);
	  double th0 = th0_1 - (z_1-z)/dzdt;
	  th0_0 = th0_1;
	  th0_1 = th0;
	  cth0 = cos(th0_1);
	  sth0 = sin(th0_1);
	  R = c_s / (c_grad * cth0);
	  sth = (R * sth0 -r)/R;
	  cth = sqrt(1.0-pow(sth,2));
	  z_0 = z_1;
	  z_1 =  R*cth - c_0 /c_grad;
	  //	  MOOSTrace("eigenray: i = %d,  z = %f, th = %f \n",i,z_1,th0_1*180.0/M_PI);
	  i++;
	}
      // pressure of eigenray      
      double th = atan2(sth,cth);
      double s = R*(th0_1 - th);
      double c_r = c_0 + c_grad * z;

      double delta = 0.01;
      double th1 = th0_1 + delta;
      double R1 = c_s/(c_grad * cos(th1));
      double r1 = R1 * ( sin(th1) + sin(s/R1-th1) );
 
      double J_s = r *(r1 - r)/(delta * sth);
      double a = cth0/c_s;
      // ray travel time
      // going through turning point?
      if (turn = (sth*sth0 < 0))
	{
	  double z_turn = R -c_0/c_grad;

	  if (z_turn > w_depth || z_turn < 0.0)
	    return(false);
	  // travel time
	  double c_turn = c_0 + c_grad*z_turn;
	  tau = fabs((logTerm(c_s,a) + logTerm(c_r,a))/c_grad);
	  MOOSTrace("ray turning depth, z_turn = %f \n",z_turn);
	}
      else
	tau = fabs((logTerm(c_s,a) - logTerm(c_r,a))/c_grad);

      p_tl = sqrt(fabs(c_r * cth0/(c_s * J_s))); 
      theta0 = th0_1*180.0/M_PI;
      MOOSTrace("eigenray: th0 = %f, s = %f m, p * s = %f, tau = %f \n",
		theta0,s,p_tl*s,tau);
      return(true);
    }
}

bool CRapRays::rapDepth(double x, double y, double z, 
			double x0, double y0, double& z_rap)
{
  if ( c_grad * (z_max - z) <= 0.0 )
    return(false);
  else
    {
      double r = sqrt(pow(x-x0,2)+pow(y-y0,2));
      double c_z_max = c_0 + z_max * c_grad;
      double R = c_z_max / c_grad;
      // target grazing angle
      double th_rap = acos( (c_0 + z * c_grad) / c_z_max );
      double r_rap = R * sin(th_rap);
      // within rap cone, then max depth
      if (r <= r_rap)
	z_rap = z_max;
      else
	{
	  double sth = (R*sin(th_rap) - r)/R;   
	  double cth = sqrt(1.0-pow(sth,2));
	  z_rap =  R*cth - c_0 /c_grad;
	}
      MOOSTrace("r_rap = %f, z_rap = %f \n", r_rap,z_rap);
      return(true);
    }
}

bool CRapRays::rapRange(double x0, double y0, double z0, 
			double z, double th0_deg, double& r_rap)
{
  double th0 = M_PI*th0_deg/180.0;
  double cth0 = cos(th0);
  double sth0 = sin(th0);
  double c_s = c_0 + z0 * c_grad;
  double a = cth0/c_s;
  double R = 1.0/(a*c_grad);
  double cth = (z + c_0/c_grad)/R;

  if (c_grad < 0)
    return(false);
  else if (z < z0)
    {
      // first crossing coming from below (th < 0)
      double sth = - sqrt(1.0 - pow(cth,2));
      r_rap = R * (sth0 - sth);
      return(true);
    } 
  else if (th0 > 0)
    {
      // first crossing coming from above
      double sth = sqrt(1.0 - pow(cth,2));
      r_rap = R * (sth0 - sth);
      return(true);
    } 
  else
    {
      // no crossing ray
      return(false);
    }
}
double CRapRays::logTerm(double c, double a)
{
  return(log((1+sqrt(1.0 - pow(a*c,2)))/(a*c)));
}

bool CRapRays::rapGeoTrack(double x0, double y0, 
			   double hdg0, double spd0, 
			   double brg, double brg_rate,
			   double r, double r_rate,
			   double & x, double & y,
			   double & hdg, double & spd)
{
  double brg_ang = (90-brg)*M_PI/180.0;
  double brg_rate_ang = - brg_rate * M_PI/180.0;
  double h_ang = (90-hdg0)*M_PI/180.0;
  x = x0 + r * cos(brg_ang);
  y = y0 + r * sin(brg_ang);
  double vx_rel = r_rate * cos(brg_ang) - r*brg_rate_ang*sin(brg_ang);
  double vy_rel = r_rate * sin(brg_ang) + r*brg_rate_ang*cos(brg_ang);
  double vx = spd0 * cos(h_ang) + vx_rel;
  double vy = spd0 * sin(h_ang) + vy_rel;
  hdg = 90 - atan2(vy,vx)*180.0/M_PI;
  if (hdg >= 360.0)
    hdg -= 360.0;
  else if (hdg < 0.0)
    hdg += 360.0;
  spd = sqrt(pow(vx,2) + pow(vy,2));

  return(true);
}
