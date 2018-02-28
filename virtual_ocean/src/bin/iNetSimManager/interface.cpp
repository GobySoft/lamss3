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

#include "goby/moos/goby_moos_app.h"
#include "goby/moos/protobuf/frontseat.pb.h"
#include "goby/moos/moos_string.h"

#include "lamss/lib_lamss_protobuf/modem_sim.pb.h"
#include "lamss/lib_netsim/tcp_client.h"

#include "config.pb.h"

using goby::glog;
using goby::common::goby_time;
using namespace goby::common::logger;


boost::shared_ptr<InterfaceNetSimManagerConfig> master_config;

 
class InterfaceNetSimManager : public GobyMOOSApp
{       
  public:
    static InterfaceNetSimManager* get_instance();
    static void delete_instance();

  private:    
    InterfaceNetSimManager(InterfaceNetSimManagerConfig& cfg);
    ~InterfaceNetSimManager() { }
    
    void loop();
    void check_connected();
    void handle_frontseat_data(const goby::moos::protobuf::FrontSeatInterfaceData& data);
    void handle_node_report(const CMOOSMsg& msg);
    
private:
    const InterfaceNetSimManagerConfig& cfg_;
    static InterfaceNetSimManager* inst_;

    boost::asio::io_service io_;
    std::shared_ptr<netsim::tcp_client> client_;

    int request_id_ {0};

    struct NodeReport
    {
        double time;
        double lat;
        double lon;
        double depth;  
        double speed;
        double heading;
    };

};

InterfaceNetSimManager* InterfaceNetSimManager::inst_ = 0;

InterfaceNetSimManager* InterfaceNetSimManager::get_instance()
{
    if(!inst_)
    {
        master_config.reset(new InterfaceNetSimManagerConfig);
        inst_ = new InterfaceNetSimManager(*master_config);
    }
    
    return inst_;
}

void InterfaceNetSimManager::delete_instance()
{
    if(inst_) delete inst_;
    inst_ = 0;
}

int main(int argc, char* argv[])
{
    return goby::moos::run<InterfaceNetSimManager>(argc, argv);
}


InterfaceNetSimManager::InterfaceNetSimManager(InterfaceNetSimManagerConfig& cfg) :
    GobyMOOSApp(&cfg),
    cfg_(cfg),
    client_(netsim::tcp_client::create(io_))
{
    client_->connect(cfg_.netsim_manager_ip_addr(), cfg_.netsim_manager_tcp_port());
    client_->read_callback<NetSimManagerResponse>(
        [](const NetSimManagerResponse& t, const boost::asio::ip::tcp::endpoint& ep)
        {
            glog.is(DEBUG1) && glog << "Received response from [" << ep << "]: " << t.ShortDebugString() << std::endl;
        });

    register_timer(cfg_.reconnect_interval(), &InterfaceNetSimManager::check_connected, this);

    switch(cfg_.nav_source())
    {
        case InterfaceNetSimManagerConfig::IFRONTSEAT:
            subscribe_pb("IFS_DATA_IN", &InterfaceNetSimManager::handle_frontseat_data, this);
            break;
        case InterfaceNetSimManagerConfig::NODE_REPORT:
            subscribe(cfg_.node_report_var(), &InterfaceNetSimManager::handle_node_report, this);
            break;
    }
    
}


void InterfaceNetSimManager::loop()
{
    try
    {    
        io_.poll();
    }
    catch(const boost::system::error_code& e)
    {
        glog.is(WARN) && glog << "Error: " << e.message() << std::endl;
    }
    
    
}

void InterfaceNetSimManager::check_connected()
{
    if(!client_->connected())
    {
        glog.is(WARN) && glog << "Client is not connected to remote netsim_manager server" << std::endl;
        
        client_->connect(cfg_.netsim_manager_ip_addr(), cfg_.netsim_manager_tcp_port());
    }
}

void InterfaceNetSimManager::handle_node_report(const CMOOSMsg& msg)
{
    using goby::moos::val_from_string;
    const std::string& s = msg.GetString();

    std::string name;
    val_from_string(name, s, "NAME");
    if(boost::iequals(name, cfg_.common().community()))
    {
        NodeReport nr;
        if(!val_from_string(nr.lat, s, "LAT"))
        {
            glog.is(WARN) && glog << "Node report missing LAT: " << s << std::endl;
            return;    
        }        
        if(!val_from_string(nr.lon, s, "LON"))
        {
            glog.is(WARN) && glog << "Node report missing LON: " << s << std::endl;
            return;    
        }
        if(!val_from_string(nr.depth, s, "DEPTH"))
        {
            glog.is(WARN) && glog << "Node report missing DEPTH: " << s << std::endl;
            return;    
        }
        if(!val_from_string(nr.speed, s, "SPD"))
        {
            glog.is(WARN) && glog << "Node report missing SPD: " << s << std::endl;
            return;    
        }
        if(!val_from_string(nr.heading, s, "HDG"))
        {
            glog.is(WARN) && glog << "Node report missing HDG: " << s << std::endl;
            return;    
        }
        if(!val_from_string(nr.time, s, "UTC_TIME"))
        {
            glog.is(WARN) && glog << "Node report missing UTC_TIME: " << s << std::endl;
            return;    
        }

        NetSimManagerRequest req;
        req.set_id(request_id_++);
        auto& nav = *req.add_nav();
        nav.set_modem_tcp_port(cfg_.modem_tcp_port());
        nav.set_time(nr.time);
        nav.set_lat(nr.lat);
        nav.set_lon(nr.lon);
        nav.set_depth(nr.depth);
        nav.set_speed(nr.speed);
        nav.set_heading(nr.heading);
        
        if(client_->connected())
        {
            glog.is(DEBUG1) && glog << "Sent request (from " << cfg_.node_report_var() << "): " << req.ShortDebugString() << std::endl;
            client_->write(req);
        }        
    }
    
    
}



void InterfaceNetSimManager::handle_frontseat_data(const goby::moos::protobuf::FrontSeatInterfaceData& data)
{
    if(data.has_node_status())
    {
        NetSimManagerRequest req;
        req.set_id(request_id_++);
        auto& nav = *req.add_nav();
        nav.set_modem_tcp_port(cfg_.modem_tcp_port());
        nav.set_time(data.node_status().time());
        nav.set_lat(data.node_status().global_fix().lat());
        nav.set_lon(data.node_status().global_fix().lon());
        nav.set_depth(data.node_status().global_fix().depth());
        nav.set_speed(data.node_status().speed());
        nav.set_heading(data.node_status().pose().heading());
        
        if(client_->connected())
        {
            glog.is(DEBUG1) && glog << "Sent request (from IFS_DATA_IN): " << req.ShortDebugString() << std::endl;
            client_->write(req);
        }
    }
}
