// t. schneider
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

#include "goby/acomms/protobuf/mm_driver.pb.h"
#include "goby/acomms/protobuf/modem_message.pb.h"
#include "goby/middleware/protobuf/frontseat.pb.h"
#include "goby/moos/goby_moos_app.h"
#include "goby/moos/moos_string.h"

#include "netsim/messages/netsim.pb.h"
#include "netsim/tcp/tcp_client.h"

#include "config.pb.h"

using goby::glog;
using namespace goby::util::logger;

boost::shared_ptr<InterfaceNetSimManagerConfig> master_config;

class InterfaceNetSimManager : public goby::moos::GobyMOOSApp
{
  public:
    static InterfaceNetSimManager* get_instance();
    static void delete_instance();

  private:
    InterfaceNetSimManager(InterfaceNetSimManagerConfig& cfg);
    ~InterfaceNetSimManager() {}

    void loop();
    void check_connected();
    void handle_frontseat_data(const goby::middleware::frontseat::protobuf::InterfaceData& data);
    void handle_node_report(const CMOOSMsg& msg);
    void handle_modem_receive(const goby::acomms::protobuf::ModemTransmission& rx_data);

  private:
    const InterfaceNetSimManagerConfig& cfg_;
    static InterfaceNetSimManager* inst_;

    boost::asio::io_service io_;
    std::shared_ptr<netsim::tcp_client> client_;

    int request_id_{0};

    struct NodeReport
    {
        double time;
        double lat;
        double lon;
        double depth;
        double speed;
        double heading;
    };

    // maps modem id -> modem_tcp_port
    std::map<int, int> modem_tcp_port_;
};

InterfaceNetSimManager* InterfaceNetSimManager::inst_ = 0;

InterfaceNetSimManager* InterfaceNetSimManager::get_instance()
{
    if (!inst_)
    {
        master_config.reset(new InterfaceNetSimManagerConfig);
        inst_ = new InterfaceNetSimManager(*master_config);
    }

    return inst_;
}

void InterfaceNetSimManager::delete_instance()
{
    if (inst_)
        delete inst_;
    inst_ = 0;
}

int main(int argc, char* argv[]) { return goby::moos::run<InterfaceNetSimManager>(argc, argv); }

InterfaceNetSimManager::InterfaceNetSimManager(InterfaceNetSimManagerConfig& cfg)
    : goby::moos::GobyMOOSApp(&cfg), cfg_(cfg), client_(netsim::tcp_client::create(io_))
{
    client_->connect(cfg_.netsim_manager_ip_addr(), cfg_.netsim_manager_tcp_port());
    client_->read_callback<netsim::protobuf::NetSimManagerResponse>(
        [](const netsim::protobuf::NetSimManagerResponse& t,
           const boost::asio::ip::tcp::endpoint& ep)
        {
            glog.is(DEBUG1) && glog << "Received response from [" << ep
                                    << "]: " << t.ShortDebugString() << std::endl;
        });

    register_timer(cfg_.reconnect_interval(), &InterfaceNetSimManager::check_connected, this);

    switch (cfg_.nav_source())
    {
        case InterfaceNetSimManagerConfig::IFRONTSEAT:
            subscribe_pb("IFS_DATA_IN", &InterfaceNetSimManager::handle_frontseat_data, this);
            break;
        case InterfaceNetSimManagerConfig::NODE_REPORT:
            subscribe(cfg_.node_report_var(), &InterfaceNetSimManager::handle_node_report, this);
            break;
    }

    subscribe_pb("ACOMMS_MODEM_RECEIVE", &InterfaceNetSimManager::handle_modem_receive, this);

    for (int i = 0, n = cfg_.modem_id_mapping_size(); i < n; ++i)
        modem_tcp_port_[cfg_.modem_id_mapping(i).modem_id()] =
            cfg_.modem_id_mapping(i).modem_tcp_port();
}

void InterfaceNetSimManager::loop()
{
    try
    {
        io_.poll();
    }
    catch (const boost::system::error_code& e)
    {
        glog.is(WARN) && glog << "Error: " << e.message() << std::endl;
    }
}

void InterfaceNetSimManager::check_connected()
{
    if (!client_->connected())
    {
        glog.is(WARN) && glog << "Client is not connected to remote netsim_manager server"
                              << std::endl;

        client_->connect(cfg_.netsim_manager_ip_addr(), cfg_.netsim_manager_tcp_port());
    }
}

void InterfaceNetSimManager::handle_node_report(const CMOOSMsg& msg)
{
    using goby::moos::val_from_string;
    const std::string& s = msg.GetString();

    std::string name;
    val_from_string(name, s, "NAME");

    std::string this_name =
        cfg_.has_node_report_name() ? cfg_.node_report_name() : cfg_.common().community();

    if (boost::iequals(this_name, name))
    {
        NodeReport nr;
        if (!val_from_string(nr.lat, s, "LAT"))
        {
            glog.is(WARN) && glog << "Node report missing LAT: " << s << std::endl;
            return;
        }
        if (!val_from_string(nr.lon, s, "LON"))
        {
            glog.is(WARN) && glog << "Node report missing LON: " << s << std::endl;
            return;
        }
        if (!(val_from_string(nr.depth, s, "DEPTH") || val_from_string(nr.depth, s, "DEP")))
        {
            glog.is(WARN) && glog << "Node report missing DEPTH: " << s << std::endl;
            return;
        }
        if (!val_from_string(nr.speed, s, "SPD"))
        {
            glog.is(WARN) && glog << "Node report missing SPD: " << s << std::endl;
            return;
        }
        if (!val_from_string(nr.heading, s, "HDG"))
        {
            glog.is(WARN) && glog << "Node report missing HDG: " << s << std::endl;
            return;
        }
        if (!(val_from_string(nr.time, s, "UTC_TIME") || val_from_string(nr.time, s, "TIME")))
        {
            glog.is(WARN) && glog << "Node report missing UTC_TIME: " << s << std::endl;
            return;
        }

        netsim::protobuf::NetSimManagerRequest req;
        req.set_id(request_id_++);
        auto& nav = *req.add_nav();
        nav.set_modem_tcp_port(cfg_.modem_tcp_port());
        nav.set_time(nr.time);
        nav.set_lat(nr.lat);
        nav.set_lon(nr.lon);
        nav.set_depth(nr.depth);
        nav.set_speed(nr.speed);
        nav.set_heading(nr.heading);

        if (client_->connected())
        {
            glog.is(DEBUG1) && glog << "Sent request (from " << cfg_.node_report_var()
                                    << "): " << req.ShortDebugString() << std::endl;
            client_->write(req);
        }
    }
}

void InterfaceNetSimManager::handle_frontseat_data(
    const goby::middleware::frontseat::protobuf::InterfaceData& data)
{
    if (data.has_node_status())
    {
        netsim::protobuf::NetSimManagerRequest req;
        req.set_id(request_id_++);
        auto& nav = *req.add_nav();
        nav.set_modem_tcp_port(cfg_.modem_tcp_port());
        nav.set_time(data.node_status().time());
        nav.set_lat(data.node_status().global_fix().lat());
        nav.set_lon(data.node_status().global_fix().lon());
        nav.set_depth(data.node_status().global_fix().depth());
        nav.set_speed(data.node_status().speed().over_ground());
        nav.set_heading(data.node_status().pose().heading());

        if (client_->connected())
        {
            glog.is(DEBUG1) && glog << "Sent request (from IFS_DATA_IN): " << req.ShortDebugString()
                                    << std::endl;
            client_->write(req);
        }
    }
}

void InterfaceNetSimManager::handle_modem_receive(
    const goby::acomms::protobuf::ModemTransmission& rx_data)
{
    if (rx_data.GetExtension(goby::acomms::micromodem::protobuf::transmission).receive_stat_size() <
        1)
        return;

    const goby::acomms::micromodem::protobuf::ReceiveStatistics& stats =
        rx_data.GetExtension(goby::acomms::micromodem::protobuf::transmission).receive_stat(0);

    if (!modem_tcp_port_.count(stats.source()))
    {
        glog.is(WARN) && glog << "No tcp port mapping for modem id: " << stats.source()
                              << std::endl;
        return;
    }

    netsim::protobuf::NetSimManagerRequest req;
    req.set_id(request_id_++);
    netsim::protobuf::ReceiveStats& netsim_stats = *req.add_stats();
    netsim_stats.set_modem_tcp_port(cfg_.modem_tcp_port());
    netsim_stats.set_tx_modem_tcp_port(modem_tcp_port_[stats.source()]);
    netsim_stats.set_packet_success(stats.mode() ==
                                    goby::acomms::micromodem::protobuf::RECEIVE_GOOD);
    *netsim_stats.mutable_mm_stats() = stats;

    if (client_->connected())
    {
        glog.is(DEBUG1) && glog << "Sent stats: " << req.ShortDebugString() << std::endl;
        client_->write(req);
    }
}
