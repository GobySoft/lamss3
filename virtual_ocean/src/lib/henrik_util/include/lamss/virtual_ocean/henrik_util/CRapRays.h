// H. Schmidt, henrik@mit.edu 07.02.10
// Laboratory for Autonomous Marine Sensing Systems (LAMSS)
// Massachusetts Institute of Technology 
// 
// CRapRays.h  
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

#ifndef __CRAPRAYS_h__
#define __CRAPRAYS_h__

#include "MOOSLIB/MOOSLib.h"
// #include "AngleUtils.h"

#include <math.h>
#include <vector>
 
class CRapRays 
{
 public:

  bool   setEnv(double c_surface, double c_gradient, 
		double water_depth, double z_rap_max); 
  bool rapRay(double x0, double y0, double z0, 
	      double x, double y, double z, 
	      double& theta0, double& p_tl, double& tau, bool& turn);
  bool rapDepth(double x, double y, double z, 
		double x0, double y0, double& z_rap);
  bool rapRange(double x0, double y0, double z0, 
		double z, double th0_deg, double& r_rap);
  bool maxRapRange(double sonar_depth, double target_depth, double& range);
  bool rapDipRange(double z0, double z, 
		   double tau, double& th0_deg, double& r_rap);
  bool rapGeoTrack(double x0, double y0, 
		   double hdg0, double spd0, 
		   double brg, double brg_rate,
		   double r, double r_rate,
		   double & x, double & y,
		   double & hdg, double & spd);
  bool rapFootprint(double elev_deg, 
		    double my_depth, 
		    double rap_depth,
		    double beam_width, 
		    double &range_center, 
		    double& range_3db_inner, 
		    double& range_3db_outer);

 protected:

  double logTerm(double c, double a);

  double c_0;
  double c_grad;
  double w_depth;
  double z_max;
};
  
#endif /* __CRAPRAYS_h__ */
