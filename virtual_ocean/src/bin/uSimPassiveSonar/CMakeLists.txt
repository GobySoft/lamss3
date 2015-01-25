#--------------------------------------------------------
# The CMakeLists.txt for:                  pMultiAcousticSim
# Author(s):                               H. Schmidt
#--------------------------------------------------------

  FILE(GLOB SRC *.cpp)
  
  ADD_EXECUTABLE(uSimPassiveSonar ${SRC})
  
  TARGET_LINK_LIBRARIES(uSimPassiveSonar
  ${MOOS_LIBRARIES}
  ${LAMSS_HENRIK_UTIL_LIBRARY}
  ${IVP_MBUTIL_LIBRARY}
  ${IVP_GEOMETRY_LIBRARY} 
  ${LAMSS_BELLHOP_TOOLS_LIBRARY}
  ${PROTOBUF_LIBRARIES}
    gsl
    gslcblas
    )

