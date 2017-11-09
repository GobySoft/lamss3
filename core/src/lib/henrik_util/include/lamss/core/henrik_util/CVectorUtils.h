// H. Schmidt, henrik@mit.edu 01.20.10
// Laboratory for Autonomous Marine Sensing Systems (LAMSS)
// Massachusetts Institute of Technology 
// 
// CTrack.h  
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

#ifndef __CVECTORUTILS_HEADER
#define __CVECTORUTILS_HEADER

#include "MOOS/libMOOS/MOOSLib.h"
#include "MBUtils.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"

#include <iostream>
#include <string>
#include <vector>
#include <math.h>
#include <complex>

class CmyVectorUtils  
{
 public:
  int str2dbl_vec(std::string, std::vector<double>& values);
  bool dbl_vec2str(std::vector<double> numbers, std::string& str);
  std::vector<double> zeros(int m, int n);
  double mean(const std::vector<double>& val);
  double sum(const std::vector<double>& val);
  int intsum(const std::vector<int>& ival);
  bool vshift(std::vector<double> in, int indx_in,std::vector<double>& out, int indx_out, int len);
  bool cumsum(std::vector<double> in, std::vector<double>& out);
  bool cumtrapz(std::vector<double> x, std::vector<double> y, std::vector<double>& out);
  bool interp(const std::vector<double>& x, const std::vector<double>& y, 
	      const std::vector<double>& xi, std::vector<double>& yi);
  void vpair_sort(std::vector<double> x, 
		  std::vector<double> y,
		  std::vector<double> x_sort,
		  std::vector<double> y_sort);
  bool mmul(std::vector<double> A, int rows_A, int cols_A, std::vector<double> B, int cols_B, std::vector<double>& C);
  bool vadd(std::vector<double> a, std::vector<double> b, std::vector<double>& c);
  bool vmul(std::vector<double> a, std::vector<double> b, std::vector<double>& c);
  bool vsub(std::vector<double> a, std::vector<double> b, std::vector<double>& c);
  bool vneg(std::vector<double> a, std::vector<double>& c);
  bool vsmul(std::vector<double> a, double b, std::vector<double>& c);
  bool vmov(std::vector<double> a, std::vector<double>& c);
  double vmax(const std::vector<double>& val);
  double vmin(const std::vector<double>& val);
  bool minv(std::vector<double> a, int dim, std::vector<double>& a_inv);
  bool bminv(std::vector<double> a, int dim, int bw, std::vector<double>& a_inv);
  double bsolve(std::vector<double> a, int dim, int bw, std::vector<double>& b, int ncols);
  // Complex vector and matrix functions
  std::complex<double> zmul(std::complex<double> a, std::complex<double> b);
  std::complex<double> conjg(std::complex<double> a);
  bool cholesky(std::vector<std::complex<double> > a, 
		 int dim, std::vector<std::complex<double> >& c);
  bool mmul(std::vector<std::complex<double> > A, int rows_A, int cols_A,
			     std::vector<std::complex<double> > B, int cols_B, 
			     std::vector<std::complex<double> >& C);
  std::vector<std::complex<double> > zzeros(int m, int n);

  //added by sdanesh
  std:: vector<double> hann(int N);

  std:: vector<double> tukey(int N,double r);
  int NN(double num, std::vector<double> myvec);
  std::vector<std::vector<double> > zero_matrix(int n, int m);
  void lin_least_square(const std::vector<double>& x, 
			const std::vector<double>& y,
			double x_0,
			double& y_0, 
			double& slope,
			double& rms_error);
  void prune_timeseries(std::vector<double> t, 
			std::vector<double> y,
			double t_now,
			double history, 
			std::vector<double>& t_new,
			std::vector<double>& y_new);


 protected:
  int nrows;
  int ncols;

  int indx(int i, int j);
  int indx_tri(int i, int j);


};

#endif /* __CVECTORUTILS_HEADER */
