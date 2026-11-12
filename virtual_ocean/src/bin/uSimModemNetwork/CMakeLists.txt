# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(GSL REQUIRED)
lamss_find_ivp(geometry)
lamss_add_app(uSimModemNetwork MOOS
  SOURCES SimModemNetwork.cpp
  PROTOS SimModemNetwork_config.proto
  LINK_LIBRARIES lamss::virtual_ocean_henrik_util lamss::virtual_ocean_protobuf lamss::core_henrik_util
    IvP::mbutil IvP::geometry GSL::gsl netsim_acousticstoolbox netsim_messages)
