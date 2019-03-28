// copyright 2010 H. Schmidt henrik@mit.edu
// 
// this file is part of lib_behaviors-henrik, a collection of pHelmIvP behaviors
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


#ifndef BHV_OPTCOHACOUSTDEPTH_HEADER
#define BHV_OPTCOHACOUSTDEPTH_HEADER

#include "lamss/lib_bellhop/iBellhop_messages.pb.h"
#include "lamss/lib_bellhop/svp_request_response.pb.h"
#include "ivp/MOOSGeodesy.h"

#include "IvPBehavior.h"
#include "ConditionalParam.h"
#include <lamss/lib_henrik_util/CEigenRays.h>
#include <lamss/lib_henrik_util/CRapRays.h>


std::ostream& operator<< (std::ostream& out, const std::vector<double> & t)
{
    for(std::vector<double>::const_iterator it = t.begin(), nt = t.end(); it != nt; ++it)
    {
        if(it != t.begin()) out << ",";
        out << *it;
    }    
    return out;
}


class BHV_OptCohAcoustDepth : public IvPBehavior {
public:
  BHV_OptCohAcoustDepth(IvPDomain);
  ~BHV_OptCohAcoustDepth() {};
  
  IvPFunction* onRunState();
  bool         setParam(std::string, std::string);

  private:


  boost::posix_time::ptime last_calc_time_;  
  boost::posix_time::time_duration calculate_interval_;
  std::string collaborator_;
  double current_range;
  double current_rate;
  int look_ahead_seconds_;
  double depth_envelope_;
  double depth_uncertainty_;
  double departure_range_;
  double approach_range_;
  double range_max;
  bool peak_filter_;
  int num_forecast_ranges;

  bool log_open;
  bool active_region;

  bool optimize_se;
  bool optimize_snr;
  bool optimize_tl;
  bool optimize_nl;

  bool get_nl;
  bool get_tl;
  bool get_ag;

  bool valid_tl;
  bool valid_nl;
  bool valid_ag;

  bool debug;
  bool new_tl;
  bool new_ivp;

  int request_id;
  std::string optimize;

  std::string sonar;
  int tl_factor;
  double adapt_tl_threshold;
  double tl_threshold;

  std::vector<double> dom, range;
  std::vector<double> dom_tl_save, range_tl_save;
  std::vector<double> elev_angle_save;
  int max_number_bounces;
  double depth_min;
  double depth_max;
  int number_depths;
  double theta_min;
  double theta_max;
  int number_theta;
  double max_pitch;
  double beam_width;
  double dip_min;
  double dip_max;  
  // Eigenrays
  CEigenRayTable eigenray_table;
  CEigenRay eig;
  // Rap acoustics
  double surface_sound_speed;
  double sound_speed_gradient;
  double water_depth;
  CRapRays rap_trace;

// SVP updates 
  CMOOSGeodesy m_Geodesy;
  void requestSVPUpdate();
  bool handleSVPResponse(std::string cn);
  SVP::Response svp_response;
  bool svp_ready;
  bool svp_requested;
  int svp_request_id;
  double last_svp_request_;
  double svp_depth_resolution;
  double svp_range_resolution;
  double mseas_update_interval;
  
  bool got_lat_origin;
  bool got_long_origin;
  bool geo_init;

    // Logging file
  std::ofstream file_out_;
};

extern "C" {
  IvPBehavior * createBehavior(std::string name, IvPDomain domain)
  {return new BHV_OptCohAcoustDepth(domain);}
}

#endif
