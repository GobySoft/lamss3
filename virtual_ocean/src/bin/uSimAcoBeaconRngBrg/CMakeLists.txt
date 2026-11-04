# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_find_ivp(geometry contacts apputil)
lamss_add_app(uSimAcoBeaconRngBrg MOOS
  SOURCES SimAcoBeaconRngBrg.cpp SimAcoBeaconRngBrg_Info.cpp main.cpp
  LINK_LIBRARIES IvP::geometry IvP::contacts IvP::apputil IvP::mbutil goby_frontseat_bluefin)
