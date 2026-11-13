// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MOOS/libMOOS/MOOSLib.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"
#include "iBackSeatServer.h"

int main(int argc, char* argv[])
{
    const char* sMissionFile = "iBackSeatServer.moos";

    if (argc > 1)
    {
        sMissionFile = argv[1];
    }

    CiBackSeatServer iBackSeatServer;

    iBackSeatServer.Run("iBackSeatServer", sMissionFile);

    return 0;
}
