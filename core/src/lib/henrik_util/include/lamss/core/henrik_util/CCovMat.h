// H. Schmidt, henrik@mit.edu 01.20.10
// Laboratory for Autonomous Marine Sensing Systems (LAMSS)
// Massachusetts Institute of Technology 
// 
// CCovMat.h  
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


#ifndef __CCOVMAT_HEADER__
#define __CCOVMAT_HEADER__

//#include "MOOS/libMOOS/MOOSLib.h"
//#include "MBUtils.h"
//#include "MOOS/libMOOS/Utils/MOOSUtils.h"

#include <math.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <complex>

#include "lamss/lib_henrik_util/Random.h"
 
class CovMat
{
 public:

  bool putCovMat(int nrcv, double frq, std::vector<std::complex<double> > newcov);
  bool getCovMat(int nrcv, double& frq, std::vector<std::complex<double> >& cov);
  bool getCovLow(int nrcv, double& frq, std::vector<std::complex<double> >& cov);
  bool getCholLow(int nrcv, double& frq, std::vector<std::complex<double> >& c);
  bool putCovLow(int nrcv, double frq, std::vector<std::complex<double> > cov);
  bool putCholLow(int nrcv, double frq, std::vector<std::complex<double> > c);
  std::complex<double> Covar(int i, int j);
  void realization(std::vector<std::complex<double> >& out);
  bool scaleCovMat(double fac);
  bool wnChol();
  double normCholTrace();
 protected:
  int indx( int i, int j);
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
  std::vector<std::complex<double> > cov;
  // c is the cholesky factorized lower triangular
  std::vector<std::complex<double> > c;
};

#endif /* __CCOVMAT_HEADER__ */
