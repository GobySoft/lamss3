# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_find_moos()
add_executable(lamss_autonomy_moos_probe moos_probe.cpp)
target_link_libraries(lamss_autonomy_moos_probe PRIVATE MOOS::MOOS)
add_dependencies(lamss_autonomy_all lamss_autonomy_moos_probe)

find_program(LAMSS_MOOSDB MOOSDB)
find_program(LAMSS_PHELMIVP pHelmIvP)
if(LAMSS_MOOSDB)
  add_test(NAME lamss_autonomy_moos_smoke
    COMMAND ${CMAKE_COMMAND} -E env
      LAMSS_MOOSDB=${LAMSS_MOOSDB}
      LAMSS_MOOS_PROBE=$<TARGET_FILE:lamss_autonomy_moos_probe>
      LAMSS_PLAMSSMISSIONMANAGER=$<TARGET_FILE:pLamssMissionManager>
      LAMSS_PTETRAHOMING=$<TARGET_FILE:pTetraHoming>
      LAMSS_PGATEWAY=$<TARGET_FILE:pGateway>
      LAMSS_UMVS_BLUEFIN=$<TARGET_FILE:uMVS_Bluefin>
      ${CMAKE_CURRENT_SOURCE_DIR}/moos_smoke.sh)
  set_tests_properties(lamss_autonomy_moos_smoke PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
endif()
if(LAMSS_MOOSDB AND LAMSS_PHELMIVP)
  add_test(NAME lamss_autonomy_behaviors_load
    COMMAND ${CMAKE_COMMAND} -E env
      LAMSS_MOOSDB=${LAMSS_MOOSDB}
      LAMSS_PHELMIVP=${LAMSS_PHELMIVP}
      LAMSS_MOOS_PROBE=$<TARGET_FILE:lamss_autonomy_moos_probe>
      LAMSS_BEHAVIOR_DIR=${LAMSS_AUTONOMY_BEHAVIOR_DIR}
      "LAMSS_BEHAVIORS=${LAMSS_AUTONOMY_BEHAVIORS}"
      ${CMAKE_CURRENT_SOURCE_DIR}/behaviors_load.sh ${CMAKE_CURRENT_SOURCE_DIR}/smoke.bhv)
  set_tests_properties(lamss_autonomy_behaviors_load PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
endif()
