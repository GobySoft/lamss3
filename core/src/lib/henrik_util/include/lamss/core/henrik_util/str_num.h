#ifndef __str_num_h__
#define __str_num_h__

#include "MOOS/libMOOS/MOOSLib.h"
#include "MBUtils.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"

#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>
  bool int_vec2str(std::vector<int>,std::string&);
  int str2int_vec(std::string, std::vector<int>&);
  int str2dbl_vec(std::string, std::vector<double>& values);
  bool dbl_vec2str(std::vector<double> numbers, std::string& str);

#endif /* __str_num_h__ */

