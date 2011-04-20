#--------------------------------------------------------
# The CMakeLists.txt for:                 iLineBasedComms
# Author(s):                                 t. schneider
#--------------------------------------------------------

file(GLOB PROTOS *.proto)  
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(iLineBasedComms ${SRC} ${PROTO_HDRS} ${PROTO_SRCS})
      
TARGET_LINK_LIBRARIES(iLineBasedComms
  ${MOOS_LIBRARIES}
  ${GOBY_LIBRARIES}
)
