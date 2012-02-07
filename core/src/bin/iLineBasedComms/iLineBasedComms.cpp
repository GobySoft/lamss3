// t. schneider tes@mit.edu 08.12.10
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is iLineBasedComms.cpp 
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

#include "iLineBasedComms.h"

#include <boost/algorithm/string.hpp>

#if GOBY_VERSION_MAJOR >= 2 
#include "goby/common/logger.h"
using namespace goby::common::logger;
#else
#include "goby/util/logger.h"
#endif

using goby::util::glogger;

iLineBasedCommsConfig CiLineBasedComms::cfg_;
CiLineBasedComms* CiLineBasedComms::inst_ = 0;

CiLineBasedComms* CiLineBasedComms::get_instance()
{
    if(!inst_)
        inst_ = new CiLineBasedComms();
    return inst_;
}

// Construction / Destruction
CiLineBasedComms::CiLineBasedComms()
    : GobyMOOSApp(&cfg_),
      modem_(0)
{
    if(cfg_.connection_type() == iLineBasedCommsConfig::CONNECTION_SERIAL)
    {
        glogger() << "opening serial port " << cfg_.serial_port() << " @ " << cfg_.serial_baud() << std::endl;
            
        modem_ = new goby::util::SerialClient(cfg_.serial_port(), cfg_.serial_baud());
    }
    else if(cfg_.connection_type() == iLineBasedCommsConfig::CONNECTION_TCP_AS_CLIENT)
    {
        glogger() << "opening tcp client: " << cfg_.tcp_server() << ":" << cfg_.tcp_port() << std::endl;
        modem_ = new goby::util::TCPClient(cfg_.tcp_server(), cfg_.tcp_port());
        
    }
    else if(cfg_.connection_type() == iLineBasedCommsConfig::CONNECTION_TCP_AS_CLIENT)
    {
        glogger() << "opening tcp server on port" << cfg_.tcp_port() << std::endl;
        modem_ = new goby::util::TCPServer(cfg_.tcp_port());
    }

    modem_->start();    

    subscribe(cfg_.read_from_var(), &CiLineBasedComms::handle_out, this);
}

void CiLineBasedComms::loop()
{
    std::string in;
    while(modem_->readline(&in))
    {
        boost::trim(in);
        glogger() << ">> " << in << std::endl;
        publish(cfg_.write_to_var(), in);
    }
}

void CiLineBasedComms::handle_out(const CMOOSMsg& msg)    
{
    glogger() << "<< "  << msg.GetString() << std::endl;
    modem_->write(std::string(msg.GetString() + "\r\n"));
}
