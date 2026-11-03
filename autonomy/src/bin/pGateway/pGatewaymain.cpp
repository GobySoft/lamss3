
#include "Gateway.h"
int main(int argc , char * argv[])
{
  const char * sMissionFile = "Mission.moos";

  if(argc > 1) {
    sMissionFile = argv[1];
  }
  
  CGateway MSim;
  
  MSim.Run("pGateway", sMissionFile);
  
  return 0;
}
