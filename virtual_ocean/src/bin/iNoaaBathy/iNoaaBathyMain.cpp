// Copyright 2018-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iNoaaBathy.h"

int main(int argc, char* argv[])
{
    // default parameters file
    const char* sMissionFile = "iNoaaBathy.moos";

    // under what name should the application register with the MOOSDB?
    const char* sMOOSName = "iNoaaBathy";

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
    CiNoaaBathy iNoaaBathyApp;

    // run it
    iNoaaBathyApp.Run(sMOOSName, sMissionFile);

    return 0;
}
