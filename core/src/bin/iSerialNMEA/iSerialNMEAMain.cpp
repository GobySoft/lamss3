// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iSerialNMEA.h"
#include <string>

int main(int argc, char* argv[]) { return goby::moos::run<CiSerialNMEA>(argc, argv); }
