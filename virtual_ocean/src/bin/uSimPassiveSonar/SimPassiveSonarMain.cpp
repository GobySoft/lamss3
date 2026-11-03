#include "MOOS/libMOOS/MOOSLib.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"
#include "SimPassiveSonar.h"

int main(int argc, char* argv[])
{
    const char* sMissionFile = "uSimPassiveSonar.moos";

    if (argc > 1)
    {
        sMissionFile = argv[1];
    }

    CSimPassiveSonar uSimPassiveSonar;

    uSimPassiveSonar.Run("uSimPassiveSonar", sMissionFile);

    return 0;
}
