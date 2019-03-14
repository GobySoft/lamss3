#--------------------------------------------------------
# The CMakeLists.txt for:                  pLamssMissionManager
# Author(s):                               Henrik Schmidt
#                                          Toby Schneider 
#--------------------------------------------------------

#set (CMAKE_CXX_FLAGS_RELEASE "-g")

file(GLOB PROTOS *.proto)  
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} ${PROTOS} )

set(SRC LamssMissionManager.cpp  LamssMissionManager_fsm.cpp)

ADD_EXECUTABLE(pLamssMissionManager ${SRC} ${PROTO_HDRS} ${PROTO_SRCS})
      
TARGET_LINK_LIBRARIES(pLamssMissionManager 
  ${Boost_LIBRARIES}
  ${MOOS_LIBRARIES}
  ${GOBY_LIBRARIES}
  ${LAMSS_PROTOBUF_LIBRARY}
  ${PROTOBUF_LIBRARIES}
  ${LAMSS_HENRIK_UTIL_LIBRARY} 
  ${LAMSS_PROTOBUF_LIBRARY}
  ${IVP_CONTACTS_LIBRARY}
  ${IVP_GEOMETRY_LIBRARY}
  ${IVP_MBUTIL_LIBRARY}
 )
