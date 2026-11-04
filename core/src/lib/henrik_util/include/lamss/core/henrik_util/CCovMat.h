// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __CCOVMAT_HEADER__
#define __CCOVMAT_HEADER__

//#include "MOOS/libMOOS/MOOSLib.h"
//#include "MBUtils.h"
//#include "MOOS/libMOOS/Utils/MOOSUtils.h"

#include <complex>
#include <fstream>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>
#include <vector>

#include "lamss/core/henrik_util/Random.h"

namespace lamss
{
namespace core
{

class CovMat
{
  public:
    bool putCovMat(int nrcv, double frq, std::vector<std::complex<double>> newcov);
    bool getCovMat(int nrcv, double& frq, std::vector<std::complex<double>>& cov);
    bool getCovLow(int nrcv, double& frq, std::vector<std::complex<double>>& cov);
    bool getCholLow(int nrcv, double& frq, std::vector<std::complex<double>>& c);
    bool putCovLow(int nrcv, double frq, std::vector<std::complex<double>> cov);
    bool putCholLow(int nrcv, double frq, std::vector<std::complex<double>> c);
    // The following uses C++ indexing
    std::complex<double> getCCovar(int i, int j);
    std::vector<std::vector<std::complex<double>>> getCCovMat();
    void realization(std::vector<std::complex<double>>& out);
    void wn_realization(std::vector<std::complex<double>>& out);
    bool scaleCovMat(double fac);
    bool wnChol();
    double normCholTrace();

  protected:
    int indx(int i, int j);
    int indx_tri(int i, int j);
    std::complex<double> conjg(std::complex<double>);
    bool cholesky();
    void addWhiteNoise(double wn);
    double gasdev();

    CRandom ran;

    int num_rcv;
    double freq;
    int iset;
    double gset;

    bool c_done;
    // cov is the lower triangular of the covariance matrix
    std::vector<std::complex<double>> cov;
    // c is the cholesky factorized lower triangular
    std::vector<std::complex<double>> c;
};

} // namespace core
} // namespace lamss

#endif /* __CCOVMAT_HEADER__ */
