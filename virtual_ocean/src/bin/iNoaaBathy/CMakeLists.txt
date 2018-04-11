#--------------------------------------------------------
# The CMakeLists.txt for:                 iNoaaBathy
# Author(s):                              Henrik Schmidt
#--------------------------------------------------------

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(iNoaaBathy ${SRC})
      
TARGET_LINK_LIBRARIES(iNoaaBathy
  ${MOOS_LIBRARIES}
  ${IVP_MBUTIL_LIBRARY}
  ${GOBY_LIBRARIES}
  ${Boost_LIBRARIES}
  bellhop_tools
  netcdf
)
