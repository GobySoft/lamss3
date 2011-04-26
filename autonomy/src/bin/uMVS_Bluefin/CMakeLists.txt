#--------------------------------------------------------
# The CMakeLists.txt for:                    uMVS_Bluefin
# Author(s):                                 David Battle
#--------------------------------------------------------

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(uMVS_Bluefin ${SRC})

TARGET_LINK_LIBRARIES(uMVS_Bluefin
   MOOS
   MOOSGen
   MOOSUtility
   newmat10D
   m
   pthread)


