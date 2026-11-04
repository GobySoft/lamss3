# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_find_moos()
add_executable(lamss_sonar_moos_probe moos_probe.cpp)
target_link_libraries(lamss_sonar_moos_probe PRIVATE MOOS::MOOS)
add_executable(lamss_sonar_array_data array_data.cpp)
add_dependencies(lamss_sonar_all lamss_sonar_moos_probe lamss_sonar_array_data)

find_program(LAMSS_MOOSDB MOOSDB)
if(LAMSS_MOOSDB)
  add_test(NAME lamss_sonar_moos_smoke
    COMMAND ${CMAKE_COMMAND} -E env
      LAMSS_MOOSDB=${LAMSS_MOOSDB}
      LAMSS_MOOS_PROBE=$<TARGET_FILE:lamss_sonar_moos_probe>
      LAMSS_ARRAY_DATA=$<TARGET_FILE:lamss_sonar_array_data>
      LAMSS_SONAR_BIN=${PROJECT_SOURCE_DIR}/src/bin
      LAMSS_PLAMSSCBF=$<TARGET_FILE:pLamssCBF>
      LAMSS_PLAMSSAEL=$<TARGET_FILE:pLamssAEL>
      LAMSS_PLAMSSBEARINGTRACKER=$<TARGET_FILE:pLamssBearingTracker>
      LAMSS_PGEOTRACKER=$<TARGET_FILE:pGeoTracker>
      LAMSS_PLAMSSTRACKMANAGER=$<TARGET_FILE:pLamssTrackManager>
      ${CMAKE_CURRENT_SOURCE_DIR}/moos_smoke.sh)
  set_tests_properties(lamss_sonar_moos_smoke PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
endif()
