#--------------------------------------------------------
# The CMakeLists.txt for:                 iLineBasedComms
# Author(s):                                 t. schneider
#--------------------------------------------------------

file(GLOB PROTOS_CONFIGURED ${CMAKE_CURRENT_BINARY_DIR}/*.proto)  
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} ${PROTOS} ${PROTOS_CONFIGURED})

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(iLineBasedComms ${SRC} ${PROTO_HDRS} ${PROTO_SRCS})
      
TARGET_LINK_LIBRARIES(iLineBasedComms
  ${MOOS_LIBRARIES}
  ${GOBY_LIBRARIES}
  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
)
