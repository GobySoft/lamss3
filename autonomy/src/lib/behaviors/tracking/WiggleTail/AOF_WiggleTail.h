// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef AOF_WIGGLETAIL_HEADER
#define AOF_WIGGLETAIL_HEADER

#include "AOF.h"

#define LR 1
#define OPT 2

class IvPDomain;
namespace lamss
{
namespace autonomy
{

class AOF_WiggleTail : public AOF
{
  public:
    AOF_WiggleTail(IvPDomain, double, double);
    ~AOF_WiggleTail(){};

  public:
    double evalBox(const IvPBox*) const; // virtual defined
    double lrmetric(double) const;

  protected:
    double leftabs, rightabs, hwidth, osCourse;

    double crsBase;
    double crsDelta;
};

} // namespace autonomy
} // namespace lamss

#endif
