#--------------------------------------------------------
# The CMakeLists.txt for:                    pTetraHoming
# Author(s):                              Nick Rypkema
# Changes by                              Henrik Schmidt
# 2/20/2020
# Moved to lamss-shared from lamss-internal
# Added publication of HOMING_LOCK
# Only re-initializes beacon location estimate when homing
# lock is stale for more than 120 seconds
#--------------------------------------------------------

# FILE(GLOB SRC *.cpp)

SET(SRC
  TetraHoming.cpp
  TetraHoming_Info.cpp
  main.cpp
)



ADD_EXECUTABLE(pTetraHoming ${SRC})

TARGET_LINK_LIBRARIES(pTetraHoming
   ${MOOS_LIBRARIES}
   ${IVP_GEOMETRY_LIBRARY}
   ${IVP_MBUTIL_LIBRARY}
   m
   pthread)

