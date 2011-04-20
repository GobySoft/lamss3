#--------------------------------------------------------
# The CMakeLists.txt for:                     iSerialNMEA
# Author(s):                                 t. schneider
#--------------------------------------------------------

file(GLOB PROTOS *.proto)  
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(iSerialNMEA ${SRC} ${PROTO_HDRS} ${PROTO_SRCS})
      
TARGET_LINK_LIBRARIES(iSerialNMEA
  ${MOOS_LIBRARIES}
  ${IVP_MBUTIL_LIBRARY}
  m
  pthread  
  ${GOBY_LIBRARIES}
)
