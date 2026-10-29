# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_add_app(iSerialNMEA MOOS
  SOURCES iSerialNMEAMain.cpp iSerialNMEA.cpp
  PROTOS iSerialNMEA_config.proto
  LINK_LIBRARIES lamss::core_henrik_util)
