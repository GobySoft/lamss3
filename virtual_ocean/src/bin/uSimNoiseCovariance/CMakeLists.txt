#--------------------------------------------------------
# The CMakeLists.txt for:                  uSimNoiseCovariance
# Author(s):                               H. Schmidt
#--------------------------------------------------------

  FILE(GLOB SRC *.cpp)
  
  ADD_EXECUTABLE(uSimNoiseCovariance ${SRC})
  
  TARGET_LINK_LIBRARIES(uSimNoiseCovariance
  ${MOOS_LIBRARIES}
  ${LAMSS_HENRIK_UTIL_LIBRARY}
  ${IVP_MBUTIL_LIBRARY}
  ${IVP_GEOMETRY_LIBRARY} 
    gsl
    gslcblas
    )

