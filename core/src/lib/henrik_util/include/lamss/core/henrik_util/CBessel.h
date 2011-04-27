// H. Schmidt, henrik@mit.edu 07.02.10
// Laboratory for Autonomous Marine Sensing Systems (LAMSS)
// Massachusetts Institute of Technology 
// 
// CBessel.h  
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

#ifndef __CBESSEL_h__
#define __CBESSEL_h__

#include "MOOSLIB/MOOSLib.h"
#include <math.h>
#include <vector>

class CBesTable
{
 public:
  double interp(double x);
  double dx;
  double onodx;
  double x_min;
  double x_max;
  std::vector<double> value;
 protected:
};
  
class CBessel 
{
 public:
  bool initBessel(int max_order, 
		  double max_x);
  bool initSphBes(int max_order, 
		  double max_x);
  double bessel_J(int m, double x);
  double sphbes_j(int m, double x);

 protected:

  double bessj0(double x);
  double bessj1(double x);

  void bessjn(int n, double x, std::vector<double>& bjn);
  void sphbesjn(int n, double x, std::vector<double>& bjn );

  std::vector<CBesTable> bes_table;
  std::vector<CBesTable> sphbes_table;

};
  
#endif /* __CBESSEL_h__ */
