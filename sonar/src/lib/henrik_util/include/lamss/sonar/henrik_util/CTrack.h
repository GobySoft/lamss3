// H. Schmidt, henrik@mit.edu 01.20.10
// Laboratory for Autonomous Marine Sensing Systems (LAMSS)
// Massachusetts Institute of Technology 
// 
// CTrack.h  
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

#ifndef __CTRACK_h__
#define __CTRACK_h__

#include "MOOS/libMOOS/MOOSLib.h"
#include "MBUtils.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"

#include <math.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
 
class CTrack 
{
 public:

  bool setTrack(int num, int prio, int templ, int state, 
		int ips, double array_x, double array_y, 
		double bearing, double sigma, double rate, 
		double rate_sigma, double snr, 
		std::vector<double>& features);
  bool getTrack(int& num, int& prio,int& templ, int& state, 
		int& ips, double& array_x, double& array_y, 
		double& bearing, double& sigma, double& rate,
		double& rate_sigma, double& snr, 
		double& utc, std::vector<double>& features);
  bool postTrack(std::string& moosvar,std::string& svar);
  bool decodeTrack(std::string);
  std::string bearingState(int node, 
			   double nav_x, double nav_y);
  bool clrTrack();
  bool bearingRate(double bearing, double sigma, 
		   double & rate, double & rate_sigma);
  bool dipAngleRate(double dipangle, 
		     double dipangle_sigma, 
		     double & dipangle_rate, 
		     double & dipangle_rate_sigma);
  bool setDipAngle(double dep,
		   double dipangle, 
		   double dipangle_sigma, 
		   double dipangle_rate, 
		   double dipangle_rate_sigma);
  bool getDipAngle(double& dep,
		   double& dipangle, 
		   double& dipangle_sigma, 
		   double& dipangle_rate, 
		   double& dipangle_rate_sigma);
  bool postDipAngle(std::string& moosvar,std::string& svar);

  int trk_num;

 protected:

  int trk_prio; // 1-10, 10 highest.
  int trk_template;
  int trk_state; // 0: Invalid, 1: Relative, 2: Bbsolute.
  int trk_ips;   // -1: Port, 1: Starboard, 0: Ambiguous
  double trk_x; // array x corresponding to track
  double trk_y; // array y corresponding to track
  double trk_dep; // array depth corresponding to track
  double trk_bearing;
  double trk_sigma;
  double trk_rate;
  double trk_rate_sigma;
  double trk_snr;
  double trk_utc;
  int trk_nfeat;
  std::vector<double> trk_features;
  bool new_trk;
  // dipangle added 100702
  double trk_dip;
  double trk_dip_rate;
  double trk_dip_sigma;
  double trk_dip_rate_sigma;
};
  
#endif /* __CTRACK_h__ */
