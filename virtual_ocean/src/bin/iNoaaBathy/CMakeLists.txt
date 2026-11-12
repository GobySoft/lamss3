# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_add_app(iNoaaBathy MOOS
  SOURCES iNoaaBathy.cpp iNoaaBathyMain.cpp
  LINK_LIBRARIES lamss::virtual_ocean_henrik_util IvP::mbutil)
if(NETSIM_FOUND)
  target_link_libraries(iNoaaBathy PRIVATE netsim_acousticstoolbox)
  target_compile_definitions(iNoaaBathy PRIVATE LAMSS_VIRTUAL_OCEAN_HAVE_NETSIM)
endif()
