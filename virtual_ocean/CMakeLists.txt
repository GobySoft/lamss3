# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.22...4.2)
project(lamss_virtual_ocean CXX)
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/LamssToolbox.cmake OPTIONAL RESULT_VARIABLE _in_tree)
if(NOT _in_tree)
  find_package(lamss_core REQUIRED)
  include(LamssToolbox)
endif()
lamss_toolbox(NAME virtual_ocean REQUIRES core)

# lamss::core_henrik_util links these by name; in a joint build core found them in its own scope
lamss_find_moos()
lamss_find_ivp(mbutil)

find_package(Boost REQUIRED COMPONENTS filesystem)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/LamssFindNetCDF.cmake)
install(FILES cmake/LamssFindNetCDF.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lamss_virtual_ocean)
lamss_export_code(
  "find_package(Boost REQUIRED COMPONENTS filesystem)"
  "include(\"\${CMAKE_CURRENT_LIST_DIR}/LamssFindNetCDF.cmake\")")

# The apps that need netsim are built only where it is installed.
# netsim's package refers to Boost::serialization without finding it.
find_package(Boost QUIET COMPONENTS serialization)
if(TARGET Boost::serialization)
  find_package(NETSIM CONFIG QUIET)
endif()
if(NETSIM_FOUND)
  message(STATUS "virtual_ocean: netsim found: building uSimModemNetwork, iNetSimManager, "
    "uSimPassiveSonar's Bellhop requests and iNoaaBathy's BATHY_REQUEST")
else()
  message(STATUS "virtual_ocean: netsim not found: not building uSimModemNetwork, iNetSimManager, "
    "uSimPassiveSonar's Bellhop requests or iNoaaBathy's BATHY_REQUEST")
endif()

add_subdirectory(src/lib/protobuf)
add_subdirectory(src/lib/henrik_util)
add_subdirectory(src/bin/iNoaaBathy)
add_subdirectory(src/bin/uSimAcoBeaconRngBrg)
add_subdirectory(src/bin/uSimCTD)
add_subdirectory(src/bin/uSimDVL)
add_subdirectory(src/bin/uSimGPS)
add_subdirectory(src/bin/uSimNoiseCovariance)
add_subdirectory(src/bin/uSimPassiveSonar)
add_subdirectory(src/bin/uSimPhinsINS)
add_subdirectory(src/bin/uSimTargets)
add_subdirectory(src/bin/uSimTowedArray)
if(NETSIM_FOUND)
  add_subdirectory(src/bin/iNetSimManager)
  add_subdirectory(src/bin/uSimModemNetwork)
endif()
if(BUILD_TESTING)
  add_subdirectory(src/test)
endif()

lamss_toolbox_export()
