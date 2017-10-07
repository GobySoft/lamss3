#--------------------------------------------------------
# The CMakeLists.txt for:                  uSimTargets
# Author(s):                               Henrik Schmidt
#--------------------------------------------------------

set (CMAKE_CXX_FLAGS_RELEASE "-g")

file(GLOB PROTOS *.proto)  
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} ${PROTOS} )

set(SRC SimTargets.cpp)

ADD_EXECUTABLE(uSimTargets ${SRC} ${PROTO_HDRS} ${PROTO_SRCS})
      
TARGET_LINK_LIBRARIES(uSimTargets 
  ${Boost_LIBRARIES}
  ${MOOS_LIBRARIES}
  ${GOBY_LIBRARIES}
  ${LAMSS_PROTOBUF_LIBRARY}
  ${PROTOBUF_LIBRARIES}
  ${IVP_MBUTIL_LIBRARY}
  ${LAMSS_HENRIK_UTIL_LIBRARY}
  ${LAMSS_ANRP_UTIL_LIBRARY}
  ${LAMSS_NEWMAT_LIBRARY}
  )
