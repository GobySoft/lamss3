// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __str_num_h__
#define __str_num_h__

#include "MBUtils.h"
#include "MOOS/libMOOS/MOOSLib.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"

#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

namespace lamss
{
namespace core
{
bool int_vec2str(std::vector<int>, std::string&);
int str2int_vec(std::string, std::vector<int>&);
int str2dbl_vec(std::string, std::vector<double>& values);
bool dbl_vec2str(std::vector<double> numbers, std::string& str);

} // namespace core
} // namespace lamss

#endif /* __str_num_h__ */
