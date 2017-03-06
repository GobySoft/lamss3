#include "MOOS/libMOOS/MOOSLib.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"
#include "SimNoiseCovariance.h"

int main(int argc , char * argv[])
{
  const char * sMissionFile = "uSimNoiseCovariance.moos";

  if(argc > 1) {
    sMissionFile = argv[1];
  }
  
  CSimNoiseCovariance uSimNoiseCovariance;
  
  uSimNoiseCovariance.Run("uSimNoiseCovariance", sMissionFile);
  
  return 0;
}
