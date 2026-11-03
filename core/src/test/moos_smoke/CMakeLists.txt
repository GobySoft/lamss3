# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_find_moos()
add_executable(lamss_core_moos_probe moos_probe.cpp)
target_link_libraries(lamss_core_moos_probe PRIVATE MOOS::MOOS)
add_dependencies(lamss_core_all lamss_core_moos_probe)

find_program(LAMSS_MOOSDB MOOSDB)
find_program(LAMSS_GOBYD gobyd)
find_program(LAMSS_GOBY goby)
find_program(LAMSS_GOBY_MOOS_GATEWAY goby_moos_gateway)
if(LAMSS_MOOSDB AND LAMSS_GOBYD AND LAMSS_GOBY AND LAMSS_GOBY_MOOS_GATEWAY)
  add_test(NAME lamss_core_moos_smoke
    COMMAND ${CMAKE_COMMAND} -E env
      LAMSS_MOOSDB=${LAMSS_MOOSDB}
      LAMSS_GOBYD=${LAMSS_GOBYD}
      LAMSS_GOBY=${LAMSS_GOBY}
      LAMSS_GOBY_MOOS_GATEWAY=${LAMSS_GOBY_MOOS_GATEWAY}
      LAMSS_GATEWAY_PLUGIN=$<TARGET_FILE:lamss_core_moos_gateway_plugin>
      LAMSS_CORE_PROTOBUF=$<TARGET_FILE:lamss_core_protobuf>
      LAMSS_MOOS_PROBE=$<TARGET_FILE:lamss_core_moos_probe>
      LAMSS_ISERIALNMEA=$<TARGET_FILE:iSerialNMEA>
      LAMSS_PVIEWMESSAGE=$<TARGET_FILE:pViewMessage>
      LAMSS_ILINEBASEDCOMMS=$<TARGET_FILE:iLineBasedComms>
      LAMSS_PMOOSLINK=$<TARGET_FILE:pMOOSLink>
      ${CMAKE_CURRENT_SOURCE_DIR}/moos_smoke.sh)
  set_tests_properties(lamss_core_moos_smoke PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
endif()
