// Copyright 2013-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pMOOSLink.h"

int main(int argc, char* argv[])
{
    // default parameters file
    const char* sMissionFile = "pMOOSLink.moos";

    // under what name should the application register with the MOOSDB?
    const char* sMOOSName = "pMOOSLink";

    switch (argc)
    {
        case 3:
            // command line says don't register with default name
            sMOOSName = argv[2];
            [[fallthrough]];
        case 2:
            // command line says don't use default config file
            sMissionFile = argv[1];
    }

    // make an application
    CpMOOSLink pMOOSLinkApp;

    // run it
    pMOOSLinkApp.Run(sMOOSName, sMissionFile);

    return 0;
}
