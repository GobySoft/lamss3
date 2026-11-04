// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BHV_CONSTANT_BUOYANCY_HEADER
#define BHV_CONSTANT_BUOYANCY_HEADER

#include "IvPBehavior.h"
#include <string>

class IvPDomain;
namespace lamss
{
namespace autonomy
{

class BHV_ConstantBuoyancy : public IvPBehavior
{
  public:
    BHV_ConstantBuoyancy(IvPDomain);
    ~BHV_ConstantBuoyancy(){};

    bool setParam(std::string, std::string);
    IvPFunction* onRunState();
    void onIdleState();

  protected:
  private: // Behavior Parameters
    double buoyancy;
};

} // namespace autonomy
} // namespace lamss

#endif
