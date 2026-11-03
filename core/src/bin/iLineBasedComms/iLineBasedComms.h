// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef iLineBasedCommsH
#define iLineBasedCommsH

#include <string>

#include "goby/version.h"
#if GOBY_VERSION_MAJOR >= 2
#include "goby/moos/goby_moos_app.h"
#else
#include "goby/moos/libmoos_util/tes_moos_app.h"
#endif

#include "goby/util/linebasedcomms.h"

#include "iLineBasedComms_config.pb.h"

class CiLineBasedComms :
#if GOBY_VERSION_MAJOR < 2
    public TesMoosApp
{
    typedef TesMoosApp goby::moos::GobyMOOSApp;
#else
    public goby::moos::GobyMOOSApp
{
#endif
  public:
    static CiLineBasedComms* get_instance();

  private:
    CiLineBasedComms();
    ~CiLineBasedComms() {}

    void loop();
    void handle_out(const CMOOSMsg& msg);
    void handle_bridge(const CMOOSMsg& msg);

    goby::util::LineBasedInterface* modem_;

    typedef std::string LocalVar;
    typedef std::string RemoteVar;
    std::multimap<LocalVar, RemoteVar> bridge_;

    static lamss::core::iLineBasedCommsConfig cfg_;
    static CiLineBasedComms* inst_;
};

#endif
