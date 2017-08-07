#--------------------------------------------------------
# The CMakeLists.txt for:                       pTetraHoming
# Author(s):                              Nick Rypkema
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

