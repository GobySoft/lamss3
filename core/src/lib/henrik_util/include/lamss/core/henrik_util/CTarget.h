// H. Schmidt, henrik@mit.edu 01.20.10
// Laboratory for Autonomous Marine Sensing Systems (LAMSS)
// Massachusetts Institute of Technology 
// 
// CTarget.h  
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


#ifndef __CTARGET_h__
#define __CTARGET_h__

#include "MOOSLIB/MOOSLib.h"
#include "MBUtils.h"
#include "MOOSGenLib/MOOSGenLib.h"

#include <math.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

 
class CTarget 
{
 public:

  bool setTargetValues(int tgt_num, bool tgt_on, std::vector<double> values);
  bool getTargetValues(int& tgt_num, bool& tgt_on, std::vector<double>& values);
  bool printTargetValues();
  std::string target_id();
  std::string target_state();

  bool target_on;
  int target_number;
  double target_utc;
  double target_x;
  double target_y;
  double target_spd;
  double target_hdg;
  double target_dep;
  double target_spl;
  double target_freq;
  double target_bw;
  double target_strength;
  double target_str;
  std::vector<double> tone_freq;
  std::vector<double> tone_dbl;
  

  double target_amp() {pow(10.0,-6.0)*pow(10.0,target_spl/20.0);};

};

#endif /* __CTARGET_h__ */
