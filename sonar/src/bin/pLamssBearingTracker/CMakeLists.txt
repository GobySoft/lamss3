# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Newmat REQUIRED)
lamss_find_ivp(mbutil)
lamss_add_app(pLamssBearingTracker MOOS
  SOURCES LamssBearingTracker.cpp
  PROTOS LamssBearingTracker_config.proto
  LINK_LIBRARIES lamss::sonar_protobuf lamss::sonar_henrik_util Newmat::Newmat IvP::mbutil)
