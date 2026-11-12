// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Gateway.h"
int main(int argc, char* argv[])
{
    const char* sMissionFile = "Mission.moos";

    if (argc > 1)
    {
        sMissionFile = argv[1];
    }

    CGateway MSim;

    MSim.Run("pGateway", sMissionFile);

    return 0;
}
