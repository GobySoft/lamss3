#--------------------------------------------------------
# The CMakeLists.txt for:                  uSimModemNetwork
# Author(s):                               H. Schmidt
#--------------------------------------------------------

#set (CMAKE_CXX_FLAGS_RELEASE "-g")

file(GLOB PROTOS *.proto)  
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} ${PROTOS} )

set(SRC SimModemNetwork.cpp)

ADD_EXECUTABLE(uSimModemNetwork ${SRC} ${PROTO_HDRS} ${PROTO_SRCS})
      
  TARGET_LINK_LIBRARIES(uSimModemNetwork
  ${GOBY_LIBRARIES}
  ${MOOS_LIBRARIES}
  ${LAMSS_HENRIK_UTIL_LIBRARY}
  ${IVP_MBUTIL_LIBRARY}
  ${IVP_GEOMETRY_LIBRARY} 
  ${LAMSS_BELLHOP_TOOLS_LIBRARY}
  ${LAMSS_PROTOBUF_LIBRARY}
  ${PROTOBUF_LIBRARIES}
  ${Boost_LIBRARIES}
    gsl
    gslcblas
    )

