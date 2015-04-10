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

#ifndef pMOOSLinkH
#define pMOOSLinkH

#include "MOOS/libMOOS/MOOSLib.h"

bool on_connect_to_server(void* mooslink);

class CpMOOSLink : public CMOOSApp  
{
  public:
 CpMOOSLink(): config_read_(false), local_client_(m_Comms) { }    
    ~CpMOOSLink() { }
    

  private:
    bool OnNewMail(MOOSMSG_LIST &NewMail);
    bool Iterate();
    bool OnConnectToServer();
    bool OnRemoteConnectToServer();

    void RegisterVariables();
    bool ReadConfiguration();

    typedef std::string MOOSVar;
    void parse_request(const std::string& value, std::pair<MOOSVar, MOOSVar>* request);

    enum Direction { IN, OUT };
        
    void repost(MOOSMSG_LIST& mail, Direction dir);

    friend bool on_connect_to_server(void* mooslink);

  private:
    bool config_read_;

    CMOOSCommClient remote_client_;
    CMOOSCommClient& local_client_;

    std::multimap<MOOSVar, MOOSVar> out_map_;
    std::multimap<MOOSVar, MOOSVar> in_map_;
    
};

#endif 
