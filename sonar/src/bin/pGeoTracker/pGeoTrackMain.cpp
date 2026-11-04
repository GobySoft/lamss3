// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "GeoTracker.h"
#include <MOOS/libMOOS/MOOSLib.h>

int main(int argc, char* argv[])
{
    const char* sMissionFile = "Mission.moos";
    const char* sMOOSName = "pGeoTracker";

    switch (argc)
    {
        case 3: sMOOSName = argv[2]; [[fallthrough]];
        case 2: sMissionFile = argv[1];
    }

    CGeoTracker Tracker;

    Tracker.Run(sMOOSName, sMissionFile);

    return 0;
}
