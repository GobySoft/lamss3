# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

foreach(_test registry params variants resolve cli)
  lamss_add_pytest(lamss_core_mission_${_test} ${CMAKE_CURRENT_SOURCE_DIR}/test_${_test}.py
    PYTHONPATH ${PROJECT_SOURCE_DIR}/src/python)
endforeach()
