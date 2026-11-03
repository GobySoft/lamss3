// H. Schmidt, henrik@mit.edu 01.20.10
// Laboratory for Autonomous Marine Sensing Systems (LAMSS)
// Massachusetts Institute of Technology
//
// CNoiseCovariance.h
//
// see the readme file within this directory for information
// pertaining to usage and purpose of this script.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.

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

#include "lamss/lib_henrik_util/CBessel.h"

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

#endif /* __CNOISECOVARIANCE_HEADER__ */
