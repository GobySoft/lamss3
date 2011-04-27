#--------------------------------------------------------
# The CMakeLists.txt for:                         pArraySim
# Author(s):                               Henrik Schmidt
#--------------------------------------------------------

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(uSimTowedArray ${SRC})

TARGET_LINK_LIBRARIES(uSimTowedArray
  ${MOOS_LIBRARIES}
  ${LAMSS_HENRIK_UTIL_LIBRARY}
  ${IVP_MBUTIL_LIBRARY}
)


