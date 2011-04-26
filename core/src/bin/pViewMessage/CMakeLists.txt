#--------------------------------------------------------
# The CMakeLists.txt for:                    pViewMessage
# Author(s):                                   H. Schmidt
#--------------------------------------------------------

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(pViewMessage ${SRC})
   
TARGET_LINK_LIBRARIES(pViewMessage
   MOOS
   MOOSGen
   MOOSUtility
   mbutil
   m
   pthread)

