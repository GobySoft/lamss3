# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Protobuf REQUIRED)
lamss_add_test(lamss_core_moos_gateway_plugin_load
  SOURCES load.cpp LINK_LIBRARIES protobuf::libprotobuf ${CMAKE_DL_LIBS})
target_compile_definitions(lamss_core_moos_gateway_plugin_load
  PRIVATE LAMSS_MOOS_GATEWAY_PLUGIN="$<TARGET_FILE:lamss_core_moos_gateway_plugin>")
add_dependencies(lamss_core_moos_gateway_plugin_load lamss_core_moos_gateway_plugin)

find_program(LAMSS_GOBY_MOOS_GATEWAY goby_moos_gateway)
if(LAMSS_GOBY_MOOS_GATEWAY)
  add_test(NAME lamss_core_moos_gateway_plugin_example_config
    COMMAND ${CMAKE_COMMAND} -E env
      GOBY_MOOS_GATEWAY_PLUGINS=$<TARGET_FILE:lamss_core_moos_gateway_plugin>
      ${LAMSS_GOBY_MOOS_GATEWAY} --example_config)
  set_tests_properties(lamss_core_moos_gateway_plugin_example_config PROPERTIES
    PASS_REGULAR_EXPRESSION "\\[lamss\\.core\\.goby_gateway_config\\]")
endif()
