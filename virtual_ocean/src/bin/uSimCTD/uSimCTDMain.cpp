// *****************************************************************************
// Author: Raymond Lum
// Date:   Aug 3, 2011
// *****************************************************************************

#include "uSimCTD.h"

int main(int argc, char* argv[])
{
    const char* sMissionFile = "uSimCTD.moos";

    if (argc > 1)
    {
        sMissionFile = argv[1];
    }

    CuSimCTD MSim;

    MSim.Run("uSimCTD", sMissionFile);

    return 0;
}
