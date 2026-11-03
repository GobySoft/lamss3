// Copyright 2013-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef pMOOSLinkH
#define pMOOSLinkH

#include "MOOS/libMOOS/MOOSLib.h"

bool on_connect_to_server(void* mooslink);

class CpMOOSLink : public CMOOSApp
{
  public:
    CpMOOSLink() : config_read_(false), local_client_(m_Comms) {}
    ~CpMOOSLink() {}

  private:
    bool OnNewMail(MOOSMSG_LIST& NewMail);
    bool Iterate();
    bool OnConnectToServer();
    bool OnRemoteConnectToServer();

    void RegisterVariables();
    bool ReadConfiguration();

    typedef std::string MOOSVar;
    void parse_request(const std::string& value, std::pair<MOOSVar, MOOSVar>* request);

    enum Direction
    {
        IN,
        OUT
    };

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
