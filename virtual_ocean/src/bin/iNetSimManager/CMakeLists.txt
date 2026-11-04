# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_add_app(iNetSimManager MOOS
  SOURCES interface.cpp
  PROTOS config.proto
  LINK_LIBRARIES netsim_tcp netsim_messages ${CMAKE_DL_LIBS})
