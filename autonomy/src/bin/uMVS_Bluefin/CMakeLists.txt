# Copyright 2011-2026 Massachusetts Institute of Technology
# SPDX-License-Identifier: GPL-3.0-or-later

find_path(LAMSS_NEWMAT_INCLUDE_DIR newmat/newmatap.h REQUIRED)
find_library(LAMSS_NEWMAT_LIBRARY newmat REQUIRED)
lamss_find_ivp(mbutil)
lamss_add_app(uMVS_Bluefin MOOS
  SOURCES
    AcousticBeacon.cpp
    AcousticIntersection.cpp
    AcousticNode.cpp
    AcousticResponder.cpp
    AcousticSignal.cpp
    AcousticTransceiver.cpp
    MOOSTerrain.cpp
    MVSim.cpp
    SimBase.cpp
    SimEntity.cpp
    SimEnvironment.cpp
    SimGlobalHelper.cpp
    SimLoggable.cpp
    SimParams.cpp
    SimVehicle.cpp
    SixDOFAUV.cpp
    uMVSMain.cpp
  LINK_LIBRARIES IvP::mbutil ${LAMSS_NEWMAT_LIBRARY})
target_include_directories(uMVS_Bluefin SYSTEM PRIVATE ${LAMSS_NEWMAT_INCLUDE_DIR})
