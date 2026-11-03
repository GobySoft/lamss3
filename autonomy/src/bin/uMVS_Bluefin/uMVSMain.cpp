// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MVSim.h"
#include <MOOS/libMOOS/MOOSLib.h>
#include <string>

int main(int argc, char* argv[])
{
    const char* sMissionFile = "Mission.moos";
    const char* sMOOSName = "uMVS_Bluefin";

    switch (argc)
    {
        case 3: sMOOSName = argv[2]; [[fallthrough]];
        case 2: sMissionFile = argv[1];
    }

    CMVSim TheSim;

    TheSim.Run(sMOOSName, sMissionFile);

    return 0;
}
