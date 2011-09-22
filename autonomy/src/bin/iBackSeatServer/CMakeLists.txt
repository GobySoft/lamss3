#--------------------------------------------------------
# The CMakeLists.txt for:               iBackSeatServer 
# Author(s):                            Arjuna Balasuriya
#--------------------------------------------------------

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(iBackSeatServer ${SRC})

TARGET_LINK_LIBRARIES(iBackSeatServer 
   ${MOOS_LIBRARIES}
   anrp_util
   ${GOBY_LIBRARIES}
   ${Boost_LIBRARIES}
   ${PROTOBUF_LIBRARIES}
#   m
#   pthread
#   goby_linebasedcomms
)

