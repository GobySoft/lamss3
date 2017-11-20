#--------------------------------------------------------
# The CMakeLists.txt for:                  pLamssBearingTracker
# Author(s):                               Henrik Schmidt
#--------------------------------------------------------

set (CMAKE_CXX_FLAGS_RELEASE "-g")

file(GLOB PROTOS *.proto)  
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} ${PROTOS} )

set(SRC LamssBearingTracker.cpp)

ADD_EXECUTABLE(pLamssBearingTracker ${SRC} ${PROTO_HDRS} ${PROTO_SRCS})
      
TARGET_LINK_LIBRARIES(pLamssBearingTracker 
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
