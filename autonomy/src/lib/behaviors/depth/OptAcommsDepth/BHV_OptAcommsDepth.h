// Copyright 2019-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BHV_OPTCOHACOUSTDEPTH_HEADER
#define BHV_OPTCOHACOUSTDEPTH_HEADER

#include "goby/moos/moos_geodesy.h"
#include "netsim/acousticstoolbox/iBellhop_messages.pb.h"
#include "netsim/acousticstoolbox/svp_request_response.pb.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <fstream>
#include <string>
#include <vector>

#include "ConditionalParam.h"
#include "IvPBehavior.h"
#include "lamss/core/henrik_util/CEigenRays.h"
#include "lamss/core/henrik_util/CRapRays.h"

inline std::ostream& operator<<(std::ostream& out, const std::vector<double>& t)
{
    for (std::vector<double>::const_iterator it = t.begin(), nt = t.end(); it != nt; ++it)
    {
        if (it != t.begin())
            out << ",";
        out << *it;
    }
    return out;
}

namespace lamss
{
namespace autonomy
{

class BHV_OptAcommsDepth : public IvPBehavior
{
  public:
    BHV_OptAcommsDepth(IvPDomain);
    ~BHV_OptAcommsDepth(){};

    IvPFunction* onRunState();
    bool setParam(std::string, std::string);

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

    bool valid_ivp;
    bool valid_tl;
    bool valid_nl;
    bool valid_ag;

    bool debug;
    bool debug2;
    bool new_tl;
    bool new_ivp;

    int request_id;
    std::string optimize;

    std::string sonar;
    int tl_factor;
    double adapt_tl_threshold;
    double tl_threshold;
    int multipath_penalty;
    int bounce_penalty;

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
    lamss::core::CEigenRayTable eigenray_table;
    lamss::core::CEigenRay eig;
    // Rap acoustics
    double surface_sound_speed;
    double sound_speed_gradient;
    double water_depth;
    lamss::core::CRapRays rap_trace;

    // SVP updates
    goby::moos::CMOOSGeodesy m_Geodesy;
    void requestSVPUpdate();
    bool handleSVPResponse(std::string cn);
    netsim::SVP::protobuf::Response svp_response;
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

} // namespace autonomy
} // namespace lamss

#endif
