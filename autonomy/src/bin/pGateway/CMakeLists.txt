#--------------------------------------------------------
# The CMakeLists.txt for:                     pMessageSim
# Author(s):                                Don Eickstedt
#--------------------------------------------------------

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(pGateway ${SRC})
   
TARGET_LINK_LIBRARIES(pGateway
  ${MOOS_LIBRARIES}
  ${IVP_MBUTIL_LIBRARY}
  goby_moos
)
