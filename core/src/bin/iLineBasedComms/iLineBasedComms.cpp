// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iLineBasedComms.h"

#include <boost/algorithm/string.hpp>

#include "goby/util/debug_logger.h"
using namespace goby::util::logger;

#include "goby/moos/moos_serializer.h"

using goby::glog;

lamss::core::iLineBasedCommsConfig CiLineBasedComms::cfg_;
CiLineBasedComms* CiLineBasedComms::inst_ = 0;

CiLineBasedComms* CiLineBasedComms::get_instance()
{
    if (!inst_)
        inst_ = new CiLineBasedComms();
    return inst_;
}

// Construction / Destruction
CiLineBasedComms::CiLineBasedComms() : goby::moos::GobyMOOSApp(&cfg_), modem_(0)
{
    if (cfg_.connection_type() == lamss::core::iLineBasedCommsConfig::CONNECTION_SERIAL)
    {
        glog << "opening serial port " << cfg_.serial_port() << " @ " << cfg_.serial_baud()
             << std::endl;

        modem_ = new goby::util::SerialClient(cfg_.serial_port(), cfg_.serial_baud());
    }
    else if (cfg_.connection_type() == lamss::core::iLineBasedCommsConfig::CONNECTION_TCP_AS_CLIENT)
    {
        glog << "opening tcp client: " << cfg_.tcp_server() << ":" << cfg_.tcp_port() << std::endl;
        modem_ = new goby::util::TCPClient(cfg_.tcp_server(), cfg_.tcp_port());
    }
    else if (cfg_.connection_type() == lamss::core::iLineBasedCommsConfig::CONNECTION_TCP_AS_SERVER)
    {
        glog << "opening tcp server on port" << cfg_.tcp_port() << std::endl;
        modem_ = new goby::util::TCPServer(cfg_.tcp_port());
    }
    else
    {
        glog.is(DIE) && glog << "Unsupported connection type" << std::endl;
    }
    modem_->start();

    for (int i = 0, n = cfg_.bridge_size(); i < n; ++i)
    {
        if (bridge_.count(cfg_.bridge(i).local_var()) == 0)
            subscribe(cfg_.bridge(i).local_var(), &CiLineBasedComms::handle_bridge, this);

        std::string remote_var = cfg_.bridge(i).has_remote_var() ? cfg_.bridge(i).remote_var()
                                                                 : cfg_.bridge(i).local_var();

        glog << "Bridging: " << cfg_.bridge(i).local_var() << " to " << remote_var << std::endl;

        bridge_.insert(std::make_pair(cfg_.bridge(i).local_var(), remote_var));
    }

    if (cfg_.has_read_from_var())
        subscribe(cfg_.read_from_var(), &CiLineBasedComms::handle_out, this);
}

void CiLineBasedComms::loop()
{
    std::string in;
    while ((modem_->active() || cfg_.connection_type() ==
                                    lamss::core::iLineBasedCommsConfig::CONNECTION_TCP_AS_SERVER) &&
           modem_->readline(&in))
    {
        boost::trim(in);
        glog.is(DEBUG2) && glog << ">> " << in << std::endl;

        if (cfg_.has_write_to_var())
        {
            publish(cfg_.write_to_var(), in);
        }
        else
        {
            CMOOSMsg msg;
            goby::moos::MOOSSerializer::parse(&msg, goby::util::hex_decode(in));
            glog.is(DEBUG1) && glog << "Published to " << msg.GetKey() << std::endl;
            publish(msg);
        }
    }
}

void CiLineBasedComms::handle_out(const CMOOSMsg& msg)
{
    glog << "<< " << msg.GetString() << std::endl;
    modem_->write(std::string(msg.GetString() + "\r\n"));
}

void CiLineBasedComms::handle_bridge(const CMOOSMsg& const_msg)
{
    if (modem_->active() ||
        cfg_.connection_type() == lamss::core::iLineBasedCommsConfig::CONNECTION_TCP_AS_SERVER)
    {
        typedef std::multimap<LocalVar, RemoteVar>::iterator It;
        std::pair<It, It> range = bridge_.equal_range(const_msg.GetKey());
        for (It it = range.first, end = range.second; it != end; ++it)
        {
            CMOOSMsg msg = const_msg;
            msg.m_sKey = it->second;
            std::string out;
            goby::moos::MOOSSerializer::serialize(msg, &out);
            out = goby::util::hex_encode(out);

            glog.is(DEBUG2) && glog << "<< " << out << std::endl;
            glog.is(DEBUG1) && glog << "Sent " << const_msg.GetKey() << " as " << msg.GetKey()
                                    << std::endl;
            modem_->write(std::string(out + "\r\n"));
        }
    }
    else
    {
        glog.is(WARN) && glog << "Connection not active" << std::endl;
    }
}
