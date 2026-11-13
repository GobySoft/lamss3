# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

find_program(LAMSS_SUPERVISORD supervisord)
find_program(LAMSS_GOBYD gobyd)
find_program(LAMSS_MOOSDB MOOSDB)
if(LAMSS_SUPERVISORD AND LAMSS_GOBYD AND LAMSS_MOOSDB)
  lamss_add_mission_test(NAME lamss_core_mission_example CRUISE example
    TEST ${PROJECT_SOURCE_DIR}/missions/cruise/example/test/example.yml)
else()
  message(STATUS "lamss_core_mission_example: not registered, needs supervisord, gobyd and MOOSDB")
endif()
