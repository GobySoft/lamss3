#--------------------------------------------------------
# The CMakeLists.txt for:                        uSimCDT
# Author(s):                                 Raymond Lum
#--------------------------------------------------------

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(uSimCTD ${SRC})

TARGET_LINK_LIBRARIES(uSimCTD
${MOOS_LIBRARIES})
