# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_find_moos()
lamss_find_ivp(behaviors ivpbuild logic ivpcore bhvutil mbutil geometry)
# netsim's package config names Boost::serialization without finding it
find_package(Boost QUIET COMPONENTS serialization)
if(TARGET Boost::serialization)
  find_package(NETSIM QUIET)
endif()

set(LAMSS_AUTONOMY_BEHAVIORS
  BHV_ConstantAltitude
  BHV_ConstantBuoyancy
  BHV_MinAltitude
  BHV_SmartYoyo
  BHV_RaceTrack
  BHV_MaintainBearing
  BHV_WiggleTail
  BHV_LamssZigZag)
set(_sources
  create_behavior.cpp
  depth/ConstantAltitude/BHV_ConstantAltitude.cpp
  depth/ConstantBuoyancy/BHV_ConstantBuoyancy.cpp
  depth/MinAltitude/BHV_MinAltitude.cpp
  depth/SmartYoyo/BHV_SmartYoyo.cpp
  survey/RaceTrack/BHV_RaceTrack.cpp
  tracking/MaintainBearing/BHV_MaintainBearing.cpp
  tracking/WiggleTail/AOF_WiggleTail.cpp
  tracking/WiggleTail/BHV_WiggleTail.cpp
  tracking/ZigZag/BHV_ZigZag.cpp)
set(_netsim_libs)
if(NETSIM_FOUND)
  list(APPEND LAMSS_AUTONOMY_BEHAVIORS BHV_OptAcommsDepth)
  list(APPEND _sources
    depth/OptAcommsDepth/BHV_OptAcommsDepth.cpp
    depth/OptAcommsDepth/ZAIC_Vector.cpp)
  set(_netsim_libs netsim_acousticstoolbox)
else()
  message(STATUS "netsim not found: building lamss_autonomy_behaviors without BHV_OptAcommsDepth")
endif()
set(LAMSS_AUTONOMY_BEHAVIORS ${LAMSS_AUTONOMY_BEHAVIORS} PARENT_SCOPE)

lamss_add_library(behaviors NO_EXPORT
  SOURCES ${_sources}
  LINK_LIBRARIES
    goby_moos
    lamss::core_henrik_util
    ${_netsim_libs}
    IvP::behaviors
    IvP::ivpbuild
    IvP::logic
    IvP::ivpcore
    IvP::bhvutil
    IvP::mbutil
    IvP::geometry)
target_include_directories(lamss_autonomy_behaviors PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(NETSIM_FOUND)
  target_compile_definitions(lamss_autonomy_behaviors PRIVATE LAMSS_AUTONOMY_HAVE_NETSIM)
endif()
# loaded through the links below, and the loader takes $ORIGIN from the link's directory
set_target_properties(lamss_autonomy_behaviors PROPERTIES
  BUILD_RPATH_USE_ORIGIN OFF
  INSTALL_RPATH "$ORIGIN/../..")

# pHelmIvP (ivp_behavior_dir or IVP_BEHAVIOR_DIRS) and IvPico load each libBHV_<name>.so in a directory
set(LAMSS_AUTONOMY_BEHAVIOR_DIR "${CMAKE_CURRENT_BINARY_DIR}/ivp_behaviors" PARENT_SCOPE)
install(DIRECTORY DESTINATION ${CMAKE_INSTALL_LIBDIR}/lamss/ivp_behaviors)
set(_links)
foreach(_bhv IN LISTS LAMSS_AUTONOMY_BEHAVIORS)
  list(APPEND _links COMMAND ${CMAKE_COMMAND} -E create_symlink
    "../$<TARGET_SONAME_FILE_NAME:lamss_autonomy_behaviors>"
    "${CMAKE_CURRENT_BINARY_DIR}/ivp_behaviors/lib${_bhv}.so")
  install(CODE "file(CREATE_LINK \"../../$<TARGET_SONAME_FILE_NAME:lamss_autonomy_behaviors>\"
    \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/lamss/ivp_behaviors/lib${_bhv}.so\"
    SYMBOLIC)")
endforeach()
add_custom_command(TARGET lamss_autonomy_behaviors POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/ivp_behaviors"
  ${_links}
  VERBATIM)
