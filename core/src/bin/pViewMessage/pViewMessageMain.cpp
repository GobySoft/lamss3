
#include "ViewMessage.h"
int main(int argc , char * argv[])
{
  const char * sMissionFile = "Mission.moos";

  if(argc > 1) {
    sMissionFile = argv[1];
  }
  
  CViewMessage MSim;
  
  MSim.Run("pViewMessage", sMissionFile);
  
  return 0;
}
