// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __CBESSEL_h__
#define __CBESSEL_h__

#include "MOOS/libMOOS/MOOSLib.h"
#include <math.h>
#include <vector>

namespace lamss
{
namespace core
{

class CBesTable
{
  public:
    double interp(double x);
    double dx;
    double onodx;
    double x_min;
    double x_max;
    std::vector<double> value;

  protected:
};

class CBessel
{
  public:
    bool initBessel(int max_order, double max_x);
    bool initSphBes(int max_order, double max_x);
    double bessel_J(int m, double x);
    double sphbes_j(int m, double x);

  protected:
    double bessj0(double x);
    double bessj1(double x);

    void bessjn(int n, double x, std::vector<double>& bjn);
    void sphbesjn(int n, double x, std::vector<double>& bjn);

    std::vector<CBesTable> bes_table;
    std::vector<CBesTable> sphbes_table;
};

} // namespace core
} // namespace lamss

#endif /* __CBESSEL_h__ */
