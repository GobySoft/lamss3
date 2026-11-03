# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Boost REQUIRED COMPONENTS thread)
find_package(Newmat REQUIRED)
lamss_find_ivp(mbutil)
lamss_add_app(pLamssCBF MOOS
  SOURCES LamssCBF.cpp LamssCBF_fsm.cpp
  PROTOS LamssCBF_config.proto
  LINK_LIBRARIES
    lamss::sonar_protobuf lamss::autonomy_protobuf lamss::core_henrik_util
    Newmat::Newmat IvP::mbutil Boost::thread)
