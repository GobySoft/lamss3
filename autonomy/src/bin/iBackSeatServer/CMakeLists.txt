#--------------------------------------------------------
# The CMakeLists.txt for:               iBackSeatServer 
# Author(s):                            Arjuna Balasuriya
#--------------------------------------------------------

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(iBackSeatServer ${SRC})

TARGET_LINK_LIBRARIES(iBackSeatServer 
   ${MOOS_LIBRARIES}
   anrp_util
   ${GOBY_LINEBASEDCOMMS_LIBRARY}
#   m
#   pthread
#   goby_linebasedcomms
)

