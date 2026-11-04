// Copyright 2019-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __CNOISECOVARIANCE_HEADER__
#define __CNOISECOVARIANCE_HEADER__

#include "MBUtils.h"
#include "MOOS/libMOOS/MOOSLib.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"

#include <complex>
#include <fstream>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>
#include <vector>

#include "lamss/core/henrik_util/CBessel.h"

namespace lamss
{
namespace core
{

class ComputeCovariance
{
  public:
    bool initialize(double freq, double sound_speed, std::vector<double> theta,
                    std::vector<double> Sin);
    bool hydCovariance(double delta_R, double delta_z, std::complex<double>& cov);
    bool vsaCovariance(double delta_R, double delta_z, std::vector<std::complex<double>>& cov);
    bool vsaIsotropicCovariance(double delta_R, double delta_x, double delta_y, double delta_z,
                                std::vector<std::complex<double>>& cov);

    bool hydIsotropicCovariance(double delta_R, std::complex<double>& cov);

    CBessel besgen;

    CBessel spherical_besgen;

  protected:
    std::complex<double> filon(std::vector<double> x, std::vector<std::complex<double>> cfun,
                               std::vector<std::complex<double>> cex, std::vector<double> arg);

    std::complex<double> conjg(std::complex<double>);
    int indx(int i, int j);
    int n_th;
    double omega;
    double rho;
    double c;
    double k;
    std::vector<double> k_z;
    std::vector<double> k_r;
    std::vector<double> S;
};

} // namespace core
} // namespace lamss

#endif /* __CNOISECOVARIANCE_HEADER__ */
