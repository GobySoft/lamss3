# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_add_app(uSimTowedArray MOOS
  SOURCES SimTowedArray.cpp SimTowedArrayMain.cpp
  LINK_LIBRARIES lamss::core_henrik_util IvP::mbutil)
