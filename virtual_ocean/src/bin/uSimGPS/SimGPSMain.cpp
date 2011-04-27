#include "MOOSLIB/MOOSLib.h"
#include "MOOSGenLib/MOOSGenLib.h"
#include "SimGPS.h"

int main(int argc , char * argv[])
{
  const char * sMissionFile = "uSimGPS.moos";

  if(argc > 1) {
    sMissionFile = argv[1];
  }
  
  CSimGPS SimGPS;
  
  SimGPS.Run("uSimGPS", sMissionFile);
  
  return 0;
}
