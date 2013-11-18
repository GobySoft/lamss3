#--------------------------------------------------------
# The CMakeLists.txt for:                 pMOOSLink
# Author(s):                                 t. schneider
#--------------------------------------------------------

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(pMOOSLink ${SRC})
      
TARGET_LINK_LIBRARIES(pMOOSLink
  ${MOOS_LIBRARIES})
