#--------------------------------------------------------
# The CMakeLists.txt for:              iNetSimManager
# Author(s):                                 T. Schneider
#--------------------------------------------------------
set(PROTOS config.proto)  
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} ${PROTOS})

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(iNetSimManager interface.cpp ${PROTO_HDRS} ${PROTO_SRCS})

TARGET_LINK_LIBRARIES(iNetSimManager
  ${MOOS_LIBRARIES}
  ${GOBY_LIBRARIES}
  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${LAMSS_PROTOBUF_LIBRARY}
  ${CMAKE_DL_LIBS} 
  ${LAMSS_NETSIM_LIBRARY}
  netsim_tcp
  netsim_messages
  )

if(CMAKE_VERSION VERSION_LESS 3.1)
  set_target_properties(iNetSimManager PROPERTIES COMPILE_FLAGS "-std=c++11")
else()
  set_property(TARGET iNetSimManager PROPERTY CXX_STANDARD 11)
endif()
