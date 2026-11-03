// Copyright 2015
// Henrik Schmidt
// Massachusetts Institute of Technology

#include <dlfcn.h>

#include <numeric>

#include <boost/thread.hpp>

#include "goby/acomms/connect.h"
#include "goby/time/legacy.h"
#include "goby/util/debug_logger.h"

#include "LamssAEL.h"

#include "goby/moos/moos_protobuf_helpers.h"

using namespace goby::util::logger;
using namespace goby::util::logger_lock;
namespace gpb = goby::moos::protobuf;
using goby::glog;
using goby::common::goby_time;

const double pi = 3.14159;

boost::shared_ptr<LamssAELConfig> master_config;
LamssAEL* LamssAEL::inst_ = 0;
void* LamssAEL::driver_library_handle_ = 0;

int main(int argc, char* argv[]) { return goby::moos::run<LamssAEL>(argc, argv); }

LamssAEL* LamssAEL::get_instance()
{
    if (!inst_)
    {
        master_config.reset(new LamssAELConfig);
        inst_ = new LamssAEL(*master_config);
    }
    return inst_;
}

LamssAEL::LamssAEL(LamssAELConfig& cfg)
    : goby::moos::GobyMOOSApp(&cfg),
      cfg_(cfg),
      nav_count_(0),
      filt_set_(false),
      new_origin_(false),
      got_nav_(false)
{
    set_ignore_stale(false);

    // set up subscriptions before starting separate thread
    subscribe("NAV_DATA", &LamssAEL::handle_nav_data, this);
    subscribe("LAT_ORIGIN", &LamssAEL::handle_lat_origin, this);
    subscribe("LONG_ORIGIN", &LamssAEL::handle_lon_origin, this);
    //
    glog.is(VERBOSE) && glog << cfg_.DebugString() << std::endl;

    // intialize MOOSGeodesy

    m_Geodesy.Initialise(cfg_.common().lat_origin(), cfg_.common().lon_origin());

    pitch_filter.configAvg(cfg_.filter_length());
    roll_filter.configAvg(cfg_.filter_length());
    heading_filter.configAvg(cfg_.filter_length());
    filt_set_ = true;
    glog << "initialized filters" << std::endl;

    // for mooring, set last_nav_

    if (cfg_.is_mooring())
        set_mooring_nav();
}

void LamssAEL::set_mooring_nav()
{
    if (cfg_.has_deploy_x() && cfg_.has_deploy_y())
    {
        last_nav_.mutable_local_fix()->set_x(cfg_.deploy_x());
        last_nav_.mutable_local_fix()->set_y(cfg_.deploy_y());
        double lat;
        double lon;
        m_Geodesy.UTM2LatLong(last_nav_.local_fix().x(), last_nav_.local_fix().y(), lat, lon);
        last_nav_.mutable_global_fix()->set_lat(lat);
        last_nav_.mutable_global_fix()->set_lon(lon);
    }
    else if (cfg_.has_deploy_lon() && cfg_.has_deploy_lat())
    {
        last_nav_.mutable_global_fix()->set_lat(cfg_.deploy_lat());
        last_nav_.mutable_global_fix()->set_lon(cfg_.deploy_lon());
        double x;
        double y;
        m_Geodesy.LatLong2LocalUTM(last_nav_.global_fix().lat(), last_nav_.global_fix().lon(), y,
                                   x);
        last_nav_.mutable_local_fix()->set_x(x);
        last_nav_.mutable_local_fix()->set_y(y);
    }
    else
    {
        got_nav_ = false;
        return;
    }
    last_nav_.mutable_local_fix()->set_z(-cfg_.deploy_depth());
    last_nav_.mutable_global_fix()->set_depth(cfg_.deploy_depth());
    last_nav_.mutable_pose()->set_heading(cfg_.deploy_heading());
    nav_count_++;
    got_nav_ = true;
}

void LamssAEL::loop()
{
    double curr_time = MOOSTime();

    // Initialize whewn both LAT and LONG received.
    if (new_origin_)
    {
        m_Geodesy.Initialise(lat_origin_, lon_origin_);
        if (cfg_.is_mooring())
            set_mooring_nav();
        new_origin_ = false;
    }

    if (got_nav_)
    {
        std::string serialized;
        serialize_for_moos(&serialized, computeAEL());
        publish(cfg_.ael_preample() + "_NAV", serialized);
        // publish_pb(cfg_.ael_preample()+"_NAV", computeAEL());
    }

    // process some events at a slower rate
    const int slow_loop_period = 10;
    if (curr_time > last_slow_loop_t_ + slow_loop_period)
        slow_loop(curr_time);
}

void LamssAEL::slow_loop(double curr_time)
{
    // Here we should add slow loop actions.

    last_slow_loop_t_ = curr_time;
}

void LamssAEL::handle_lat_origin(const CMOOSMsg& msg)
{
    double new_lat = msg.GetDouble();
    if (!isnan(new_lat))
    {
        lat_origin_ = new_lat;
        new_origin_ = true;
    }
}

void LamssAEL::handle_lon_origin(const CMOOSMsg& msg)
{
    double new_lon = msg.GetDouble();
    if (!isnan(new_lon))
    {
        lon_origin_ = new_lon;
        new_origin_ = true;
    }
}

void LamssAEL::handle_nav_data(const CMOOSMsg& msg)
{
    goby::middleware::frontseat::protobuf::NodeStatus data;
    parse_for_moos(msg.GetString(), &data);

    last_nav_ = data;

    if (filt_set_)
    {
        pitch_filter.setVal(last_nav_.pose().pitch() * pi / 180);
        roll_filter.setVal(last_nav_.pose().roll() * pi / 180);
        heading_filter.setVal(last_nav_.pose().heading());
    }

    nav_count_++;
    if (nav_count_ >= cfg_.filter_length())
        got_nav_ = true;
}

ArrayNavigation LamssAEL::computeAEL()
{
    ArrayNavigation ael_nav;
    double curr_time = MOOSTime();

    double nav_x = last_nav_.local_fix().x();
    double nav_y = last_nav_.local_fix().y();
    double nav_dep = last_nav_.global_fix().depth();
    double nav_hdg = angle360(heading_filter.getAvg());
    double nav_pitch = pitch_filter.getAvg();
    double nav_roll = roll_filter.getAvg();
    double nav_spd = last_nav_.speed().over_ground();
    if (cfg_.array_type() == LamssAELConfig::ARRAY_TYPE_FIXED || nav_spd <= 0)
    {
        double theta = (90 - nav_hdg) * M_PI / 180.;
        double ael_x = nav_x + cfg_.array_center_offset() * cos(theta) * cos(nav_pitch);
        double ael_y = nav_y + cfg_.array_center_offset() * sin(theta) * cos(nav_pitch);
        double ael_heading = nav_hdg;
        double ael_pitch = nav_pitch;
        publish(cfg_.ael_preample() + "_PITCH", ael_pitch);
        publish(cfg_.ael_preample() + "_HEADING", ael_heading);
        publish(cfg_.ael_preample() + "_X", ael_x);
        publish(cfg_.ael_preample() + "_Y", ael_y);

        ael_nav.set_ael_utc(curr_time);
        ael_nav.set_ael_x(ael_x);
        ael_nav.set_ael_y(ael_y);
        ael_nav.set_ael_z(-nav_dep);
        ael_nav.set_ael_heading(nav_hdg);
        ael_nav.set_ael_pitch(nav_pitch);
        ael_nav.set_ael_roll(nav_roll);
        return (ael_nav);
    }
    else
    {
        double time_ago;
        if (nav_spd > 0)
        {
            // first add new navigation info
            Nav newnav;
            newnav.nav_x = nav_x;
            newnav.nav_y = nav_y;
            newnav.nav_spd = nav_spd;
            newnav.nav_hdg = nav_hdg;
            newnav.nav_dep = nav_dep;
            newnav.nav_pitch = nav_pitch;
            newnav.nav_utc = curr_time;
            nav_hist.push_back(newnav);

            time_ago = -cfg_.array_center_offset() / nav_spd;
            glog.is(DEBUG1) && glog << "looking back" << time_ago << " seconds" << std::endl;
            // First prune nav record to include nav at array offset point
            std::vector<Nav> nav_buf;
            for (int i = 0; i < nav_hist.size(); i++)
            {
                if (curr_time - time_ago < nav_hist[i].nav_utc)
                {
                    glog.is(DEBUG1) && glog << "Pruning nav older than " << time_ago << "seconds"
                                            << std::endl;
                    for (int j = i; j < nav_hist.size(); j++) nav_buf.push_back(nav_hist[j]);
                    nav_hist = nav_buf;
                    glog.is(DEBUG1) && glog << "nav record length " << nav_hist.size() << std::endl;
                    // now take oldest and use as ael
                    double ael_x = nav_hist[0].nav_x;
                    double ael_y = nav_hist[0].nav_y;
                    double ael_heading = nav_hist[0].nav_hdg;
                    double ael_pitch = nav_hist[0].nav_pitch;
                    publish(cfg_.ael_preample() + "_PITCH", ael_pitch);
                    publish(cfg_.ael_preample() + "_HEADING", ael_heading);
                    publish(cfg_.ael_preample() + "_X", ael_x);
                    publish(cfg_.ael_preample() + "_Y", ael_y);

                    ael_nav.set_ael_utc(curr_time);
                    ael_nav.set_ael_x(nav_hist[0].nav_x);
                    ael_nav.set_ael_y(nav_hist[0].nav_y);
                    ael_nav.set_ael_z(-nav_hist[0].nav_dep);
                    ael_nav.set_ael_heading(nav_hist[0].nav_hdg);
                    ael_nav.set_ael_pitch(nav_hist[0].nav_pitch);
                    ael_nav.set_ael_roll(nav_hist[0].nav_roll);
                    return (ael_nav);
                }
            }
        }
    }
    return (ael_nav);
}

double LamssAEL::angle360(double angle)
{
    double res = angle;
    //  unwrap angles
    while (res >= 360.0) res -= 360.0;
    while (res < 0.0) res += 360.0;

    return (res);
}
