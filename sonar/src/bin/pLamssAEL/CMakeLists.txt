# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_find_ivp(mbutil)
lamss_add_app(pLamssAEL MOOS
  SOURCES LamssAEL.cpp
  PROTOS LamssAEL_config.proto
  LINK_LIBRARIES lamss::sonar_protobuf lamss::core_henrik_util IvP::mbutil)
