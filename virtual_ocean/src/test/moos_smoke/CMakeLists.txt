# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

add_executable(lamss_virtual_ocean_moos_probe moos_probe.cpp)
target_link_libraries(lamss_virtual_ocean_moos_probe PRIVATE MOOS::MOOS)
add_executable(lamss_virtual_ocean_make_flat_bathy make_flat_bathy.cpp)
target_link_libraries(lamss_virtual_ocean_make_flat_bathy PRIVATE netCDF::netcdf)
add_dependencies(lamss_virtual_ocean_all
  lamss_virtual_ocean_moos_probe lamss_virtual_ocean_make_flat_bathy)

set(_apps iNoaaBathy uSimCTD uSimGPS uSimDVL uSimPhinsINS uSimAcoBeaconRngBrg uSimNoiseCovariance
  uSimTowedArray uSimPassiveSonar uSimTargets)
set(_netsim_apps)
if(NETSIM_FOUND)
  set(_netsim_apps uSimModemNetwork iNetSimManager)
endif()
string(REPLACE ";" " " _netsim_apps_str "${_netsim_apps}")
set(_app_env)
foreach(_app IN LISTS _apps _netsim_apps)
  string(TOUPPER ${_app} _upper)
  list(APPEND _app_env LAMSS_${_upper}=$<TARGET_FILE:${_app}>)
endforeach()

find_program(LAMSS_MOOSDB MOOSDB)
if(LAMSS_MOOSDB)
  add_test(NAME lamss_virtual_ocean_moos_smoke
    COMMAND ${CMAKE_COMMAND} -E env
      LAMSS_MOOSDB=${LAMSS_MOOSDB}
      LAMSS_MOOS_PROBE=$<TARGET_FILE:lamss_virtual_ocean_moos_probe>
      LAMSS_MAKE_FLAT_BATHY=$<TARGET_FILE:lamss_virtual_ocean_make_flat_bathy>
      "LAMSS_VO_NETSIM_APPS=${_netsim_apps_str}"
      ${_app_env}
      ${CMAKE_CURRENT_SOURCE_DIR}/moos_smoke.sh)
  set_tests_properties(lamss_virtual_ocean_moos_smoke PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
endif()
