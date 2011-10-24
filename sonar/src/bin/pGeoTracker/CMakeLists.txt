#--------------------------------------------------------
# The CMakeLists.txt for:                      pgeoTracker
# Author(s):                               Henrik Schmidt
#--------------------------------------------------------

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(pGeoTracker ${SRC})

TARGET_LINK_LIBRARIES(pGeoTracker
   ${MOOS_LIBRARIES}
   ${IVP_MBUTIL_LIBRARY}
   ${LAMSS_HENRIK_UTIL_LIBRARY}
   ${IVP_GEOMETRY_LIBRARY}
   opt
)


