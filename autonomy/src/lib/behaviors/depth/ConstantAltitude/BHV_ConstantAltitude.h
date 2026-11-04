// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BHV_CONSTANT_ALTITUDE_HEADER
#define BHV_CONSTANT_ALTITUDE_HEADER

#include "IvPBehavior.h"
#include <string>

class IvPDomain;
namespace lamss
{
namespace autonomy
{

class BHV_ConstantAltitude : public IvPBehavior
{
  public:
    BHV_ConstantAltitude(IvPDomain);
    ~BHV_ConstantAltitude(){};

    bool setParam(std::string, std::string);
    IvPFunction* onRunState();
    void onIdleState();

  protected:
  private: // Behavior Parameters
    double depth_local;
    double altitude;
    bool m_missing_altitude_critical;
    bool m_inverted_dvl;
};

} // namespace autonomy
} // namespace lamss

#endif
