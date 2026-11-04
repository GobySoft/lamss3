# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Boost REQUIRED COMPONENTS thread)
lamss_find_ivp(mbutil)
lamss_add_app(pLamssTrackManager MOOS
  SOURCES LamssTrackManager.cpp LamssTrackManager_fsm.cpp
  PROTOS LamssTrackManager_config.proto
  LINK_LIBRARIES
    lamss::sonar_protobuf lamss::sonar_henrik_util lamss::autonomy_protobuf
    lamss::core_protobuf lamss::core_henrik_util IvP::mbutil Boost::thread)
