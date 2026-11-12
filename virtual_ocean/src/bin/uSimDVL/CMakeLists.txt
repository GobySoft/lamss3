# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_add_app(uSimDVL MOOS NO_GOBY
  SOURCES SimDVL.cpp SimDVL_Info.cpp main.cpp
  LINK_LIBRARIES IvP::mbutil)
