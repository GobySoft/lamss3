// Copyright 2019-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "SimPhinsINS_Info.h"
#include "ColorParse.h"
#include "ReleaseInfo.h"
#include <cstdlib>
#include <iostream>

using namespace std;

//----------------------------------------------------------------
// Procedure: showSynopsis

void showSynopsis()
{
    blk("SYNOPSIS:                                                       ");
    blk("------------------------------------                            ");
    blk("  The uSimPhinsINS application is used for               ");
    blk("                                                                ");
    blk("                                                                ");
    blk("                                                                ");
    blk("                                                                ");
}

//----------------------------------------------------------------
// Procedure: showHelpAndExit

void showHelpAndExit()
{
    blk("                                                                ");
    blu("=============================================================== ");
    blu("Usage: uSimPhinsINS file.moos [OPTIONS]                   ");
    blu("=============================================================== ");
    blk("                                                                ");
    showSynopsis();
    blk("                                                                ");
    blk("Options:                                                        ");
    mag("  --alias", "=<ProcessName>                                      ");
    blk("      Launch uSimPhinsINS with the given process name         ");
    blk("      rather than uSimPhinsINS.                           ");
    mag("  --example, -e                                                 ");
    blk("      Display example MOOS configuration block.                 ");
    mag("  --help, -h                                                    ");
    blk("      Display this help message.                                ");
    mag("  --interface, -i                                               ");
    blk("      Display MOOS publications and subscriptions.              ");
    mag("  --version,-v                                                  ");
    blk("      Display the release version of uSimPhinsINS.        ");
    blk("                                                                ");
    blk("Note: If argv[2] does not otherwise match a known option,       ");
    blk("      then it will be interpreted as a run alias. This is       ");
    blk("      to support pAntler launching conventions.                 ");
    blk("                                                                ");
    exit(0);
}

//----------------------------------------------------------------
// Procedure: showExampleConfigAndExit

void showExampleConfigAndExit()
{
    blk("                                                                ");
    blu("=============================================================== ");
    blu("uSimPhinsINS Example MOOS Configuration                   ");
    blu("=============================================================== ");
    blk("                                                                ");
    blk("ProcessConfig = uSimPhinsINS                              ");
    blk("{                                                               ");
    blk("  AppTick   = 4                                                 ");
    blk("  CommsTick = 4                                                 ");
    blk("                                                                ");
    blk("}                                                               ");
    blk("                                                                ");
    exit(0);
}

//----------------------------------------------------------------
// Procedure: showInterfaceAndExit

void showInterfaceAndExit()
{
    blk("                                                                ");
    blu("=============================================================== ");
    blu("uSimPhinsINS INTERFACE                                    ");
    blu("=============================================================== ");
    blk("                                                                ");
    showSynopsis();
    blk("                                                                ");
    blk("SUBSCRIPTIONS:                                                  ");
    blk("------------------------------------                            ");
    blk("  NODE_MESSAGE = src_node=alpha,dest_node=bravo,var_name=FOO,   ");
    blk("                 string_val=BAR                                 ");
    blk("                                                                ");
    blk("PUBLICATIONS:                                                   ");
    blk("------------------------------------                            ");
    blk("  Publications are determined by the node message content.      ");
    blk("                                                                ");
    exit(0);
}

//----------------------------------------------------------------
// Procedure: showReleaseInfoAndExit

void showReleaseInfoAndExit()
{
    showReleaseInfo("uSimPhinsINS", "gpl");
    exit(0);
}
