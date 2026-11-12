// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MOOS/libMOOS/MOOSLib.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"
#include "SimGPS.h"

int main(int argc, char* argv[])
{
    const char* sMissionFile = "uSimGPS.moos";

    if (argc > 1)
    {
        sMissionFile = argv[1];
    }

    CSimGPS SimGPS;

    SimGPS.Run("uSimGPS", sMissionFile);

    return 0;
}
