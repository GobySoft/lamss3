# Copyright 2015-2026 Massachusetts Institute of Technology
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Boost REQUIRED COMPONENTS thread)
lamss_find_ivp(contacts geometry mbutil)
lamss_add_app(pLamssMissionManager MOOS
  SOURCES LamssMissionManager.cpp LamssMissionManager_fsm.cpp
  PROTOS LamssMissionManager_config.proto
  LINK_LIBRARIES
    lamss::autonomy_protobuf
    lamss::core_protobuf
    lamss::core_henrik_util
    goby_frontseat_bluefin
    Boost::thread
    IvP::contacts
    IvP::geometry
    IvP::mbutil)
