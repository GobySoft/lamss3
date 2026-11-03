#include "MOOS/libMOOS/MOOSLib.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"
#include "SimModemNetwork.h"

int main(int argc , char * argv[])
{
  const char * sMissionFile = "uSimModemNetwork.moos";

  if(argc > 1) {
    sMissionFile = argv[1];
  }
  
  CSimModemNetwork uSimModemNetwork;
  
  uSimModemNetwork.Run("uSimModemNetwork", sMissionFile);
  
  return 0;
}
