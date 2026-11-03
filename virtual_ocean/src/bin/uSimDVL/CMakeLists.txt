#--------------------------------------------------------
# The CMakeLists.txt for:                       uSimDVL
# Author(s):                              Supun Randeni
#--------------------------------------------------------

SET(SRC
  SimDVL.cpp
  SimDVL_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(uSimDVL ${SRC})

TARGET_LINK_LIBRARIES(uSimDVL
   ${MOOS_LIBRARIES}
   mbutil
   m
   pthread)

