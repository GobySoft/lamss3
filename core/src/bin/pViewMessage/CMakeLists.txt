#--------------------------------------------------------
# The CMakeLists.txt for:                    pViewMessage
# Author(s):                                   H. Schmidt
#--------------------------------------------------------
file(GLOB PROTOS *.proto)  
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} ${PROTOS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(pViewMessage ${SRC} ${PROTO_HDRS} ${PROTO_SRCS})
   
TARGET_LINK_LIBRARIES(pViewMessage
  ${MOOS_LIBRARIES}
  ${IVP_GEOMETRY_LIBRARY}
  ${IVP_MBUTIL_LIBRARY}
  ${GOBY_MOOSUTIL_LIBRARY}
)

