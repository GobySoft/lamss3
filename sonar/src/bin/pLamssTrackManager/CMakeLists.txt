#--------------------------------------------------------
# The CMakeLists.txt for:                  pLamssTrackManager
# Author(s):                               Henrik Schmidt
#--------------------------------------------------------

set (CMAKE_CXX_FLAGS_RELEASE "-g")

file(GLOB PROTOS *.proto)  
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} ${PROTOS} )

set(SRC LamssTrackManager.cpp  LamssTrackManager_fsm.cpp)

ADD_EXECUTABLE(pLamssTrackManager ${SRC} ${PROTO_HDRS} ${PROTO_SRCS})
      
TARGET_LINK_LIBRARIES(pLamssTrackManager 
  ${Boost_LIBRARIES}
  ${MOOS_LIBRARIES}
  ${GOBY_LIBRARIES}
  ${LAMSS_PROTOBUF_LIBRARY}
  ${PROTOBUF_LIBRARIES}
  ${IVP_MBUTIL_LIBRARY}
  ${LAMSS_HENRIK_UTIL_LIBRARY}
  )
