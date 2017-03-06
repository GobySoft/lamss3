#--------------------------------------------------------
# The CMakeLists.txt for:                  pLamssAEL
# Author(s):                               Henrik Schmidt
#--------------------------------------------------------

set (CMAKE_CXX_FLAGS_RELEASE "-g")

file(GLOB PROTOS *.proto)  
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} ${PROTOS} )

set(SRC LamssAEL.cpp)

ADD_EXECUTABLE(pLamssAEL ${SRC} ${PROTO_HDRS} ${PROTO_SRCS})
      
TARGET_LINK_LIBRARIES(pLamssAEL 
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
