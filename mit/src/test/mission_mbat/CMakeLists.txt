# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

if(IS_DIRECTORY "${LAMSS_CMAKE_DIR}/../core/src/python/lamss/mission")
  get_filename_component(_lamss_python "${LAMSS_CMAKE_DIR}/../core/src/python" ABSOLUTE)
else()
  set(_lamss_python "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/python3/dist-packages")
endif()
lamss_add_pytest(lamss_mit_mission_mbat_generate ${CMAKE_CURRENT_SOURCE_DIR}/test_mbat_generate.py
  PYTHONPATH ${_lamss_python})

option(LAMSS_MIT_MBAT_REALTIME_TEST "Also run the mbat mission test at warp 1 (about 10 minutes)" OFF)

find_program(LAMSS_SUPERVISORD supervisord)
find_program(LAMSS_GOBYD gobyd)
find_program(LAMSS_MOOSDB MOOSDB)
find_program(LAMSS_GOBY_LOG_TOOL goby_log_tool)
if(LAMSS_SUPERVISORD AND LAMSS_GOBYD AND LAMSS_MOOSDB)
  set(_test ${PROJECT_SOURCE_DIR}/missions/cruise/mbat/test/mbat.yml)
  lamss_add_mission_test(NAME lamss_mit_mission_mbat_smoke CRUISE mbat VARIANT smoke TIMEOUT 600 TEST ${_test})
  if(LAMSS_GOBY_LOG_TOOL)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    set_tests_properties(lamss_mit_mission_mbat_smoke PROPERTIES FIXTURES_SETUP mbat_smoke_logs)
    add_test(NAME lamss_mit_mission_mbat_smoke_bearing
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check_track_bearing.py
        ${PROJECT_SOURCE_DIR}/missions/log/lamss_mit_mission_mbat_smoke --tolerance 20
        -l ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/liblamss_core_protobuf.so)
    set_tests_properties(lamss_mit_mission_mbat_smoke_bearing PROPERTIES FIXTURES_REQUIRED mbat_smoke_logs)
  endif()
  if(LAMSS_MIT_MBAT_REALTIME_TEST)
    lamss_add_mission_test(NAME lamss_mit_mission_mbat CRUISE mbat TIMEOUT 1200 TEST ${_test})
  endif()
else()
  message(STATUS "lamss_mit_mission_mbat_smoke: not registered, needs supervisord, gobyd and MOOSDB")
endif()
