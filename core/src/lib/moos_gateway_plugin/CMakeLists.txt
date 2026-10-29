# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_find_moos()
lamss_find_ivp(geometry mbutil)

lamss_add_library(moos_gateway_plugin
  NO_EXPORT
  SOURCES moos_gateway_plugin.cpp
  PROTOS moos_gateway_plugin_config.proto
  LINK_LIBRARIES
    goby
    goby_zeromq
    goby_moos
    lamss::core_protobuf
    IvP::geometry
    IvP::mbutil)

file(RELATIVE_PATH LAMSS_BIN_TO_LIB_DIR
  "${CMAKE_INSTALL_FULL_BINDIR}" "${CMAKE_INSTALL_FULL_LIBDIR}")
set(LAMSS_GATEWAY_PLUGIN_SONAME "$<TARGET_SONAME_FILE_NAME:lamss_core_moos_gateway_plugin>")
# file(GENERATE) cannot see this directory's imported targets
get_target_property(_coroner_configs goby_coroner_moos_gateway_plugin IMPORTED_CONFIGURATIONS)
list(GET _coroner_configs 0 _coroner_config)
get_target_property(LAMSS_CORONER_PLUGIN_SONAME
  goby_coroner_moos_gateway_plugin IMPORTED_SONAME_${_coroner_config})
configure_file(lamss_goby_moos_gateway.in lamss_goby_moos_gateway.configured @ONLY)
file(GENERATE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/lamss_goby_moos_gateway"
  INPUT "${CMAKE_CURRENT_BINARY_DIR}/lamss_goby_moos_gateway.configured")
install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/lamss_goby_moos_gateway"
  DESTINATION ${CMAKE_INSTALL_BINDIR})
