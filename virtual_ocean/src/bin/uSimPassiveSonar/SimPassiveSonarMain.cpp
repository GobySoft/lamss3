#include "MOOSLIB/MOOSLib.h"
#include "MOOSGenLib/MOOSGenLib.h"
#include "SimPassiveSonar.h"

int main(int argc , char * argv[])
{
  const char * sMissionFile = "uSimPassiveSonar.moos";

  if(argc > 1) {
    sMissionFile = argv[1];
  }
  
  CSimPassiveSonar uSimPassiveSonar;
  
  uSimPassiveSonar.Run("uSimPassiveSonar", sMissionFile);
  
  return 0;
}
