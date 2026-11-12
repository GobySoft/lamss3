// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MOOS/libMOOS/MOOSLib.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"
#include "SimTowedArray.h"

int main(int argc, char* argv[])
{
    const char* sMissionFile = "uSimTowedArray.moos";

    if (argc > 1)
    {
        sMissionFile = argv[1];
    }

    CSimTowedArray uSimTowedArray;

    uSimTowedArray.Run("uSimTowedArray", sMissionFile);

    return 0;
}
