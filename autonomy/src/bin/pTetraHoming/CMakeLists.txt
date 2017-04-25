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
   geometry
   mbutil
   m
   pthread)

