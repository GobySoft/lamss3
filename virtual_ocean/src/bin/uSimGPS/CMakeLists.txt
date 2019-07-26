#--------------------------------------------------------
# The CMakeLists.txt for:                         uSimGPS
# Author(s):                               Henrik Schmidt
#--------------------------------------------------------

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(uSimGPS ${SRC})

TARGET_LINK_LIBRARIES(uSimGPS
  ${MOOS_LIBRARIES}
    goby_moos
)


