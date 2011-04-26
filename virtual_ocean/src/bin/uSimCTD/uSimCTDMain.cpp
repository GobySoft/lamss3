// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// Author: Raymond Lum
// Renamed from uCtdSim2 by H. Schmidt
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#include "uSimCTD.h"

int main(int argc , char * argv[])
{
  const char * sMissionFile = "Mission.moos";

  if(argc > 1) {
    sMissionFile = argv[1];
  }
  
  CuSimCTD MSim;
  
  MSim.Run("uSimCTD", sMissionFile);
  
  return 0;
}
