// t. schneider tes@mit.edu 11.18.13
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// copyright 2013 MIT & t. schneider
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

#include "pMOOSLink.h"


bool on_connect_to_server(void* mooslink)
{ return reinterpret_cast<CpMOOSLink*>(mooslink)->OnRemoteConnectToServer(); }

bool CpMOOSLink::OnNewMail(MOOSMSG_LIST &local_mail)
{
    repost(local_mail, OUT);
    return true;
}

bool CpMOOSLink::OnRemoteConnectToServer()
{
    std::cout << "=== Remote connect ===" << std::endl;
    for(std::multimap<MOOSVar, MOOSVar>::const_iterator it = in_map_.begin(),
            end = in_map_.end(); it != end; ++it)
    {
        remote_client_.Register(it->first, 0);
    }
    return true;
}

bool CpMOOSLink::OnConnectToServer()
{
    if (!config_read_)
    {
      if(!ReadConfiguration())
          return false;
      config_read_ = true;
    }
 
    RegisterVariables();   

    return true;
}
 
bool CpMOOSLink::Iterate()
{
    MOOSMSG_LIST remote_mail;
    remote_client_.Fetch(remote_mail);    
    repost(remote_mail, IN);
    return true;
    
}

void CpMOOSLink::RegisterVariables()
{
    for(std::multimap<MOOSVar, MOOSVar>::const_iterator it = out_map_.begin(),
            end = out_map_.end(); it != end; ++it)
    {
        local_client_.Register(it->first, 0);
    }
    

}

bool CpMOOSLink::ReadConfiguration()
{
    std::string remote_server;
    int remote_port;
    std::string community;
    

    if(!m_MissionReader.GetConfigurationParam("remote_server", remote_server))
    {
        std::cerr << "Must specify remote_server" << std::endl;
        exit(EXIT_FAILURE);
    }
    if(!m_MissionReader.GetConfigurationParam("remote_port", remote_port))
    {
        std::cerr << "Must specify remote_port" << std::endl;
        exit(EXIT_FAILURE);
    }    

    if(!m_MissionReader.GetValue("community", community))
    {
        std::cerr << "Must specify community" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    std::string app_name = std::string(GetAppName()+"@"+community);

    std::list<std::string> params;
    if(m_MissionReader.GetConfiguration(GetAppName(), params))
    {
        for(std::list<std::string>::reverse_iterator it = params.rbegin(),
                end = params.rend(); it != end; ++it)
        {
            const std::string& line  = *it;

            std::string::size_type equal_pos = line.find("=");
            if(equal_pos != std::string::npos)
            {
                std::string key = line.substr(0, equal_pos);
                std::string value = line.substr(equal_pos+1);
                if(MOOSStrCmp(key, "in"))
                {
                    std::pair<MOOSVar, MOOSVar> request;
                    parse_request(value, &request);

                    std::cout << "Mapping pull (in) request: remote: " << request.first << ", local: " << request.second << std::endl;
		    
                    in_map_.insert(request);
                }
                else if(MOOSStrCmp(key, "out"))
                {
                    std::pair<MOOSVar, MOOSVar> request;
                    parse_request(value, &request);

                    std::cout << "Mapping push (out) request: local: " << request.first << ", remote: " << request.second << std::endl;
                    
                    out_map_.insert(request);
                }
            }
        }
    }
    remote_client_.SetOnConnectCallBack(on_connect_to_server, this);
    remote_client_.Run(remote_server.c_str(), remote_port, app_name.c_str() , m_nCommsFreq);

    std::cout << "Started remote comms client ... " << std::endl;
    while(!remote_client_.IsConnected())
    {
        std::cout << "Waiting for connection to " << remote_server << ":" << remote_port << " ... " << std::endl;
        sleep(1);
    }
    std::cout << "Connected!" << std::endl;
   

    return true;
}



void CpMOOSLink::parse_request(const std::string& value, std::pair<MOOSVar, MOOSVar>* request)
{
    std::string::size_type colon_pos = value.find(":");
    if(colon_pos != std::string::npos)
    {
        request->first = value.substr(0, colon_pos);
        request->second = value.substr(colon_pos+1);
    }
    else
    {
        request->first = value;
        request->second = value;
    }
                    
}

void CpMOOSLink::repost(MOOSMSG_LIST& mail, Direction dir)
{
    std::string source_string = (dir == IN) ? "Remote" : "Local";
    std::string dest_string = (dir == IN) ? "Local" : "Remote";
    CMOOSCommClient& dest_client = (dir == IN) ? local_client_ : remote_client_;
    const std::multimap<MOOSVar, MOOSVar>& map = (dir == IN) ? in_map_ : out_map_;
    
    for(MOOSMSG_LIST::iterator it = mail.begin(),
            end = mail.end(); it != end; it++)
    {
        CMOOSMsg& msg = *it;

        std::cout << source_string << " [" << msg.GetKey() << "]: " << msg.GetAsString() << std::endl;

        typedef std::multimap<MOOSVar, MOOSVar>::const_iterator It;
        std::pair<It, It> repost = map.equal_range(msg.GetKey());
        for(It it = repost.first; it != repost.second; ++it)
        {
            CMOOSMsg repost_msg = msg;
            repost_msg.m_sKey = it->second;
            std::cout << "\t-> " << dest_string << " [" << it->second << "]" << std::endl;
            dest_client.Post(repost_msg);
        }
    }  
}
