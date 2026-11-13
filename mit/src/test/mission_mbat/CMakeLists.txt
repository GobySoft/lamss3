# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

find_program(LAMSS_SUPERVISORD supervisord)
find_program(LAMSS_GOBYD gobyd)
find_program(LAMSS_MOOSDB MOOSDB)
if(LAMSS_SUPERVISORD AND LAMSS_GOBYD AND LAMSS_MOOSDB)
  lamss_add_mission_test(NAME lamss_mit_mission_mbat CRUISE mbat
    TEST ${PROJECT_SOURCE_DIR}/missions/cruise/mbat/test/mbat.yml)
else()
  message(STATUS "lamss_mit_mission_mbat: not registered, needs supervisord, gobyd and MOOSDB")
endif()
