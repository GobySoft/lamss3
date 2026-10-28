// Copyright 2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LAMSS_TEST_LAMSS_TOOL_CHECK_H
#define LAMSS_TEST_LAMSS_TOOL_CHECK_H

#include <cstdlib>
#include <iostream>

// assert() is compiled out in RelWithDebInfo builds
#define CHECK(condition)                                                              \
    do {                                                                              \
        if (!(condition))                                                             \
        {                                                                             \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #condition \
                      << std::endl;                                                   \
            std::exit(EXIT_FAILURE);                                                  \
        }                                                                             \
    } while (false)

#endif
