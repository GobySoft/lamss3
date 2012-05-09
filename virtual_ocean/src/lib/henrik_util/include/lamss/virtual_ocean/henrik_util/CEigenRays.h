// H. Schmidt, henrik@mit.edu 08.18.10
// Laboratory for Autonomous Marine Sensing Systems (LAMSS)
// Massachusetts Institute of Technology 
// 
// CEigenRays.h 
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

#ifndef __CEIGENRAYS_h__
#define __CEIGENRAYS_h__

#include "MOOSLIB/MOOSLib.h"
// #include "AngleUtils.h"

#include <math.h>
#include <complex>
#include <vector>
#include <string.h>
#include <string>
#include <fstream>
 
class CRay 
{
 public:
  double amp;
  double phase;
  double tau;
  double dip_source;
  double dip_receiver;
  int surface_bounces;
  int bottom_bounces;
};

class CEigenRay 
{
 public:

  void clear();

  double source_depth;
  double receiver_depth;
  double freq;
  double range;
  int num_rays;
  std::vector<CRay> bundle;

 protected:

};
  
class CEigenRayTable 
{
 public:

  bool readEigenRayTable(std::string arr_file_path, int max_num_bounces); 
  bool getEigenRays(double req_range, double req_depth, 
		   double& act_range, double& act_depth,
		   CEigenRay& eig); 
  bool putEigenRay(double sd, double rd, double f, double r, CRay ray);
  int getDepths(std::vector<double>& depths);
  int getRanges(std::vector<double>& ranges);
  std::vector<CEigenRay> getEigenRayTable(std::string arr_file_path, int max_num_bounces);
 

  void clear();

  double source_depth;
  int num_ranges;
  int num_depths;

 protected:
  std::vector<double> range;
  std::vector<double> depth;
  std::vector<CEigenRay> eigenray_table;


};
  
#endif /* __CEIGENRAYS_h__ */
