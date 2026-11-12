# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(GSL REQUIRED)
lamss_find_ivp(geometry)
lamss_add_app(uSimPassiveSonar MOOS
  SOURCES SimPassiveSonar.cpp
  PROTOS SimPassiveSonar_config.proto
  LINK_LIBRARIES lamss::virtual_ocean_henrik_util lamss::virtual_ocean_protobuf lamss::core_henrik_util
    IvP::mbutil IvP::geometry GSL::gsl)
if(NETSIM_FOUND)
  target_link_libraries(uSimPassiveSonar PRIVATE netsim_acousticstoolbox)
  target_compile_definitions(uSimPassiveSonar PRIVATE LAMSS_VIRTUAL_OCEAN_HAVE_NETSIM)
endif()
