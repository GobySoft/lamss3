#--------------------------------------------------------
# The CMakeLists.txt for:                 pSimTargets
# Author(s):                              Henrik Schmidt
#--------------------------------------------------------

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(uSimTargets ${SRC})
   
TARGET_LINK_LIBRARIES(uSimTargets
  ${MOOS_LIBRARIES}
  ${LAMSS_HENRIK_UTIL_LIBRARY}
  ${IVP_MBUTIL_LIBRARY}
  ${IVP_GEOMETRY_LIBRARY} 
)
