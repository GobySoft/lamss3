// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BHV_MIN_ALTITUDE_HEADER
#define BHV_MIN_ALTITUDE_HEADER

#include "IvPBehavior.h"
#include <string>

class IvPDomain;
namespace lamss
{
namespace autonomy
{

class BHV_MinAltitude : public IvPBehavior
{
  public:
    BHV_MinAltitude(IvPDomain);
    ~BHV_MinAltitude(){};

    bool setParam(std::string, std::string);
    IvPFunction* onRunState();
    void onIdleState();

  protected:
  private: // Behavior Parameters
    double max_depth_local;
    double min_altitude;
    bool m_inverted_dvl;
};

} // namespace autonomy
} // namespace lamss

#endif
