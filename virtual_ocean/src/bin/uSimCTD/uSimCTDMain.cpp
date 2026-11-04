// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

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
