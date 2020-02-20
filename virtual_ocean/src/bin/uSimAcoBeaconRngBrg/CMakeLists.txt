#--------------------------------------------------------
# The CMakeLists.txt for:             uSimAcoBeaconRngBrg
# Author(s):                                 Nick Rypkema
# Changes by                               Henrik Schmidt
# 2/20/2020
# Moved to lamss-shared from -internal
# renamed from pAcoBearingRngBrgSim to uSimAcoBearingRngBrg
# Getting true nav from IFS_DATA_IN instead of NAV_
#--------------------------------------------------------

# FILE(GLOB SRC *.cpp)

SET(SRC
  SimAcoBeaconRngBrg.cpp
  SimAcoBeaconRngBrg_Info.cpp
  main.cpp
)



ADD_EXECUTABLE(uSimAcoBeaconRngBrg ${SRC})

TARGET_LINK_LIBRARIES(uSimAcoBeaconRngBrg
   ${MOOS_LIBRARIES}
   ${GOBY_LIBRARIES}
   ${IVP_GEOMETRY_LIBRARY}
   ${IVP_MBUTIL_LIBRARY}
   ${IVP_CONTACTS_LIBRARY}
   ${IVP_APPUTIL_LIBRARY}
   m
   pthread
   goby_frontseat_bluefin)

