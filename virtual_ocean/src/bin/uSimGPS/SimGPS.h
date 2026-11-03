#include "MOOS/libMOOS/MOOSLib.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"
#include "goby/moos/moos_geodesy.h"
#include <string>

#ifndef SIMGPS_HEADER
#define SIMGPS_HEADER

class CSimGPS : public CMOOSApp
{
  public:
    CSimGPS();
    virtual ~CSimGPS();

    bool Iterate();
    bool OnNewMail(MOOSMSG_LIST& NewMail);
    bool OnConnectToServer();
    bool OnStartUp();

  protected:
    goby::moos::CMOOSGeodesy m_Geodesy;
    bool on_surface;
    double min_gps_time;
    double mark_gps_time;
    double gps_depth;
    double gps_lat, gps_lon;
    int gps_interval;
    double nav_x, nav_y, nav_depth;

    void gps_update();
};

#endif
