// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __CVECTORUTILS_HEADER
#define __CVECTORUTILS_HEADER

#include <complex>
#include <iostream>
#include <math.h>
#include <string>
#include <vector>

namespace lamss
{
namespace core
{

class CmyVectorUtils
{
  public:
    int str2dbl_vec(std::string, std::vector<double>& values);
    bool dbl_vec2str(std::vector<double> numbers, std::string& str);
    std::vector<double> zeros(int m, int n);
    double mean(const std::vector<double>& val);
    double sum(const std::vector<double>& val);
    int intsum(const std::vector<int>& ival);
    bool vshift(std::vector<double> in, int indx_in, std::vector<double>& out, int indx_out,
                int len);
    bool cumsum(std::vector<double> in, std::vector<double>& out);
    bool cumtrapz(std::vector<double> x, std::vector<double> y, std::vector<double>& out);
    bool interp(const std::vector<double>& x, const std::vector<double>& y,
                const std::vector<double>& xi, std::vector<double>& yi);
    void resample(double dx, const std::vector<double>& y, double dx_new,
                  std::vector<double>& y_new);
    void vpair_sort(std::vector<double> x, std::vector<double> y, std::vector<double>& x_sort,
                    std::vector<double>& y_sort);
    bool mmul(std::vector<double> A, int rows_A, int cols_A, std::vector<double> B, int cols_B,
              std::vector<double>& C);
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
    bool cholesky(std::vector<std::complex<double>> a, int dim,
                  std::vector<std::complex<double>>& c);
    bool mmul(std::vector<std::complex<double>> A, int rows_A, int cols_A,
              std::vector<std::complex<double>> B, int cols_B,
              std::vector<std::complex<double>>& C);
    std::vector<std::vector<std::complex<double>>>
    cmmul(const std::vector<std::vector<std::complex<double>>>& A,
          const std::vector<std::vector<std::complex<double>>>& B);
    std::vector<std::complex<double>>
    cmvmul(const std::vector<std::vector<std::complex<double>>>& A,
           const std::vector<std::complex<double>>& B);
    std::vector<std::vector<std::complex<double>>>
    transp(const std::vector<std::vector<std::complex<double>>>& A);
    std::vector<std::vector<double>> transp(const std::vector<std::vector<double>>& A);
    std::vector<std::complex<double>> conv(const std::vector<std::complex<double>>& A,
                                           const std::vector<std::complex<double>>& B);
    std::vector<double> conv(const std::vector<double>& A, const std::vector<double>& B);
    std::vector<std::vector<std::complex<double>>>
    cminv(std::vector<std::vector<std::complex<double>>> A);
    std::vector<std::complex<double>> zzeros(int m, int n);

    //added by sdanesh
    std::vector<double> hann(int N);

    std::vector<double> tukey(int N, double r);
    int NN(double num, std::vector<double> myvec);
    std::vector<std::vector<double>> zero_matrix(int n, int m);
    void lin_least_square(const std::vector<double>& x, const std::vector<double>& y, double x_0,
                          double& y_0, double& slope, double& rms_error);
    void prune_timeseries(std::vector<double> t, std::vector<double> y, double t_now,
                          double history, std::vector<double>& t_new, std::vector<double>& y_new);

  protected:
    int nrows;
    int ncols;

    int indx(int i, int j);
    int indx_tri(int i, int j);
};

} // namespace core
} // namespace lamss

#endif /* __CVECTORUTILS_HEADER */
