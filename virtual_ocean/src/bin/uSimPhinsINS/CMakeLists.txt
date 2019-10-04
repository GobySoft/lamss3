#--------------------------------------------------------
# The CMakeLists.txt for:                       uSimPhinsINS
# Author(s):                              Supun Randeni
#--------------------------------------------------------

SET(SRC
  SimPhinsINS.cpp
  SimPhinsINS_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(uSimPhinsINS ${SRC})

TARGET_LINK_LIBRARIES(uSimPhinsINS
   ${MOOS_LIBRARIES}
   ${GOBY_LIBRARIES}
   ${LAMSS_PROTOBUF_LIBRARY}
   boost_system
   mbutil
   m
   pthread)

