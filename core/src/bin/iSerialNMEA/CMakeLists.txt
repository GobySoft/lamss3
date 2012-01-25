#--------------------------------------------------------
# The CMakeLists.txt for:                     iSerialNMEA
# Author(s):                                 t. schneider
#--------------------------------------------------------

file(GLOB PROTOS_CONFIGURED ${CMAKE_CURRENT_BINARY_DIR}/*.proto)  
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} ${PROTOS} ${PROTOS_CONFIGURED})

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(iSerialNMEA ${SRC} ${PROTO_HDRS} ${PROTO_SRCS})
      
TARGET_LINK_LIBRARIES(iSerialNMEA
  ${MOOS_LIBRARIES}
  ${IVP_MBUTIL_LIBRARY}
  ${GOBY_LIBRARIES}
  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
)
