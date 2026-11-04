// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "lamss/core/henrik_util/CVectorUtils.h"

using namespace std;

namespace lamss
{
namespace core
{

// >>>>>>>>>>>>>>> vector utility methods <<<<<<<<<<<<
int CmyVectorUtils::str2dbl_vec(string msg, vector<double>& values)
{
    string bite;

    // MOOSTrace("string = %s\n",msg.c_str());
    values.clear();
    int nz = 0;
    while (msg.length() > 0)
    {
        bite = MOOSChomp(msg, ",");
        values.push_back(atof(bite.c_str()));
        nz++;
    }
    // MOOSTrace("number of values = %d\n",nz);

    return (nz);
}

bool CmyVectorUtils::dbl_vec2str(vector<double> numbers, string& output)
{
    output = "";
    for (int i = 0; i < (int)numbers.size(); i++)
    {
        if (i > 0)
            output += ",";
        string istr = doubleToString(numbers[i], 4); // 4 is the precision 123.45
        output += istr;
    }
    return (true);
}

vector<double> CmyVectorUtils::zeros(int m, int n)
{
    vector<double> values(m * n, 0);
    /*
  values.clear();
  for (int i=0; i<m; i++)
    
    for (int j=0; j<n; j++)
      values.push_back(0.0);
  */
    return (values);
}

double CmyVectorUtils::mean(const vector<double>& values)
{
    int len = values.size();
    double sum = 0;
    for (int i = 0; i < len; i++) sum += values[i];

    return (sum / len);
}

bool CmyVectorUtils::vshift(vector<double> in, int indx_in, vector<double>& out, int indx_out,
                            int len)
{
    if (indx_in == 0 && indx_out == 0 && len == (int)in.size())
    {
        // simple copy, clear out first
        out.clear();
        for (int i = 0; i < len; i++) out.push_back(in[i]);
    }
    else
    {
        if (indx_in + len > (int)in.size() || indx_out + len > (int)out.size())
            return (false);
        // Make sure it works if in and out same vector (shifting)
        if (indx_in >= indx_out)
        {
            for (int i = 0; i < len; i++) out[indx_out + i] = in[indx_in + i];
        }
        else
        {
            for (int i = len - 1; i >= 0; i--) out[indx_out + i] = in[indx_in + i];
        }
    }
    return (true);
}

bool CmyVectorUtils::cumsum(vector<double> in, vector<double>& out)
{
    double sum = 0.0;
    out.resize(in.size());
    for (int i = 0; i < (int)in.size(); i++)
    {
        sum += in[i];
        out[i] = sum;
    }

    return (true);
}

bool CmyVectorUtils::cumtrapz(vector<double> x, vector<double> y, vector<double>& out)
{
    out.resize(y.size());
    double sum = 0.5 * y[0] * (x[1] - x[0]);
    out[0] = sum;

    int last = y.size() - 1;
    for (int i = 1; i < last - 1; i++)
    {
        sum += y[i] * 0.5 * (x[i + 1] - x[i - 1]);
        out[i] = sum;
    }

    sum += 0.5 * y[last] * (x[last] - x[last - 1]);
    ;
    out[last - 1] = sum;

    return (true);
}

double CmyVectorUtils::sum(const vector<double>& in)
{
    double sum = 0.0;
    for (int i = 0; i < (int)in.size(); i++) { sum += in[i]; }

    return (sum);
}

int CmyVectorUtils::intsum(const vector<int>& in)
{
    int sum = 0;
    for (int i = 0; i < (int)in.size(); i++) { sum += in[i]; }

    return (sum);
}

bool CmyVectorUtils::mmul(vector<double> A, int rows_A, int cols_A, vector<double> B, int cols_B,
                          vector<double>& C)
{
    // performs matrix product C(rows_A,cols_B) = A(rows_A,cols_A) * B(cols_A,cols_B), assuming matrices are stored columnwize, FORTRAN style
    C.resize(rows_A * cols_B);
    for (int j = 0; j < cols_B; j++)
        for (int i = 0; i < rows_A; i++)
        {
            double sum = 0.0;
            for (int k = 0; k < cols_A; k++) sum += A[i + k * rows_A] * B[k + j * cols_A];
            C[i + j * rows_A] = sum;
        }
    return (true);
}

bool CmyVectorUtils::vmul(vector<double> a, vector<double> b, vector<double>& c)
{
    if (a.size() != b.size())
        return (false);

    c.resize(a.size());
    for (int i = 0; i < (int)a.size(); i++) c[i] = a[i] * b[i];

    return (true);
}

bool CmyVectorUtils::vadd(vector<double> a, vector<double> b, vector<double>& c)
{
    if (a.size() != b.size())
        return (false);

    c.resize(a.size());
    for (int i = 0; i < (int)a.size(); i++) c[i] = a[i] + b[i];

    return (true);
}

bool CmyVectorUtils::vsub(vector<double> a, vector<double> b, vector<double>& c)
{
    if (a.size() != b.size())
        return (false);

    c.resize(a.size());
    for (int i = 0; i < (int)a.size(); i++) c[i] = a[i] - b[i];

    return (true);
}

bool CmyVectorUtils::vneg(vector<double> a, vector<double>& c)
{
    c.resize(a.size());
    for (int i = 0; i < (int)a.size(); i++) c[i] = -a[i];

    return (true);
}

bool CmyVectorUtils::vsmul(vector<double> a, double b, vector<double>& c)
{
    c.resize(a.size());

    for (int i = 0; i < (int)a.size(); i++) c[i] = b * a[i];

    return (true);
}

bool CmyVectorUtils::vmov(vector<double> a, vector<double>& c)
{
    c.resize(a.size());
    for (int i = 0; i < (int)a.size(); i++) c[i] = a[i];

    return (true);
}

double CmyVectorUtils::vmax(const vector<double>& in)
{
    double maxval = -1e300;
    for (int i = 0; i < (int)in.size(); i++)
    {
        if (in[i] > maxval)
            maxval = in[i];
    }

    return (maxval);
}

double CmyVectorUtils::vmin(const vector<double>& in)
{
    double minval = 1e300;
    for (int i = 0; i < (int)in.size(); i++)
    {
        if (in[i] < minval)
            minval = in[i];
    }

    return (minval);
}

bool CmyVectorUtils::minv(vector<double> a, int dim, vector<double>& c)
{
    nrows = dim;
    ncols = dim;
    c = zeros(dim, dim);
    for (int i = 1; i <= dim; i++) c[indx(i, i)] = 1;

    for (int k = 1; k <= dim; k++)
    {
        // pivoting
        double mx = fabs(a[indx(k, k)]);
        int imx = k;
        for (int i = k + 1; i <= dim; i++)
            if (fabs(a[indx(i, k)]) > mx)
            {
                mx = fabs(a[indx(i, k)]);
                imx = i;
            }
        if (imx > k)
        {
            for (int j = k; j <= dim; j++)
            {
                double temp = a[indx(k, j)];
                a[indx(k, j)] = a[indx(imx, j)];
                a[indx(imx, j)] = temp;
            }
            for (int j = 1; j <= dim; j++)
            {
                double temp = c[indx(k, j)];
                c[indx(k, j)] = c[indx(imx, j)];
                c[indx(imx, j)] = temp;
            }
        }
        // reduction
        if (a[indx(k, k)] == 0.0)
            return (false);
        a[indx(k, k)] = 1.0 / a[indx(k, k)];
        for (int i = k + 1; i <= dim; i++)
        {
            double fac = -a[indx(i, k)] * a[indx(k, k)];
            a[indx(i, k)] = 0.0;
            for (int j = k + 1; j <= dim; j++) a[indx(i, j)] += a[indx(k, j)] * fac;
            for (int j = 1; j <= dim; j++) c[indx(i, j)] += c[indx(k, j)] * fac;
        }
    }
    // back substitition
    for (int i = dim; i >= 1; i--)
        for (int j = 1; j <= dim; j++)
        {
            double ss = c[indx(i, j)];
            for (int k = i + 1; k <= dim; k++) ss -= c[indx(k, j)] * a[indx(i, k)];
            c[indx(i, j)] = ss * a[indx(i, i)];
        }

    return (true);
}

vector<vector<complex<double>>> CmyVectorUtils::cminv(vector<vector<complex<double>>> A)
{
    vector<vector<complex<double>>> C;
    int dim = A.size();
    if (A.size() < 1 || A[0].size() != A.size())
    {
        cout << "cminv: Dimension mismatch" << std::endl;
        exit(EXIT_FAILURE);
    }

    C.resize(dim);
    for (int i = 0; i < dim; i++)
    {
        C[i].resize(dim, complex<double>(0.0, 0.0));
        C[i][i] = complex<double>(1.0, 0.0);
    }

    for (int k = 0; k < dim; k++)
    {
        // pivoting
        double mx = fabs(A[k][k]);
        int imx = k;
        for (int i = k + 1; i < dim; i++)
            if (fabs(A[i][k]) > mx)
            {
                mx = fabs(A[i][k]);
                imx = i;
            }
        if (imx > k)
        {
            for (int j = k; j < dim; j++)
            {
                complex<double> temp = A[k][j];
                A[k][j] = A[imx][j];
                A[imx][j] = temp;
            }
            for (int j = 0; j < dim; j++)
            {
                complex<double> temp = C[k][j];
                C[k][j] = C[imx][j];
                C[imx][j] = temp;
            }
        }
        // reduction
        if (fabs(A[k][k]) == 0.0)
        {
            cout << "cminv: matrix singular" << std::endl;
            exit(EXIT_FAILURE);
        }
        A[k][k] = 1.0 / A[k][k];
        for (int i = k + 1; i < dim; i++)
        {
            complex<double> fac = -A[i][k] * A[k][k];
            A[i][k] = 0;
            for (int j = k + 1; j < dim; j++) A[i][j] += A[k][j] * fac;
            for (int j = 0; j < dim; j++) C[i][j] += C[k][j] * fac;
        }
    }
    // back substitition
    for (int i = dim - 1; i >= 0; i--)
        for (int j = 0; j < dim; j++)
        {
            complex<double> ss = C[i][j];
            for (int k = i + 1; k < dim; k++) ss -= C[k][j] * A[i][k];
            C[i][j] = ss * A[i][i];
        }

    return (C);
}

bool CmyVectorUtils::bminv(vector<double> a, int dim, int bw, vector<double>& c)
{
    nrows = dim;
    ncols = dim;
    int bw2 = bw;

    c = zeros(dim, dim);
    for (int i = 1; i <= dim; i++) c[indx(i, i)] = 1;

    // equilibration
    for (int i = 1; i <= dim; i++)
    {
        int j1 = max(1, i - bw);
        int j2 = min(dim, i + bw);
        double fac = 0.0;
        for (int j = j1; j <= j2; j++) fac = max(fac, fabs(a[indx(i, j)]));
        fac = 1.0 / fac;
        for (int j = j1; j <= j2; j++) a[indx(i, j)] = a[indx(i, j)] * fac;
        c[indx(i, i)] = fac * c[indx(i, i)];
    }

    for (int k = 1; k <= dim; k++)
    {
        //MOOSTrace("k=%d, pivoting\n",k);
        // pivoting
        double mx = fabs(a[indx(k, k)]);
        int imx = k;
        for (int i = k + 1; i <= min(k + bw, dim); i++)
            if (fabs(a[indx(i, k)]) > mx)
            {
                mx = fabs(a[indx(i, k)]);
                imx = i;
            }
        if (imx > k)
        {
            bw2 = max(bw2, bw + imx - k);
            for (int j = k; j <= min(k + bw2, dim); j++)
            {
                double temp = a[indx(k, j)];
                a[indx(k, j)] = a[indx(imx, j)];
                a[indx(imx, j)] = temp;
            }
            for (int j = 1; j <= dim; j++)
            {
                double temp = c[indx(k, j)];
                c[indx(k, j)] = c[indx(imx, j)];
                c[indx(imx, j)] = temp;
            }
        }
        // MOOSTrace("k=%d, reduction\n",k);
        // reduction
        if (a[indx(k, k)] == 0.0)
            return (false);
        a[indx(k, k)] = 1.0 / a[indx(k, k)];
        for (int i = k + 1; i <= min(k + bw2, dim); i++)
        {
            double fac = -a[indx(i, k)] * a[indx(k, k)];
            a[indx(i, k)] = 0.0;
            for (int j = k + 1; j <= min(dim, k + bw2); j++) a[indx(i, j)] += a[indx(k, j)] * fac;
            for (int j = 1; j <= dim; j++) c[indx(i, j)] += c[indx(k, j)] * fac;
        }
    }
    // back substitition
    for (int i = dim; i >= 1; i--)
    {
        //  MOOSTrace("i=%d, back substitution\n",i);

        int kmax = min(i + bw2, dim);
        for (int j = 1; j <= dim; j++)
        {
            double ss = c[indx(i, j)];
            for (int k = i + 1; k <= kmax; k++) ss -= c[indx(k, j)] * a[indx(i, k)];
            c[indx(i, j)] = ss * a[indx(i, i)];
        }
    }

    //  MOOSTrace("back substitution done\n");
    return (true);
}

double CmyVectorUtils::bsolve(vector<double> a, int dim, int bw, vector<double>& b, int bcols)
{
    nrows = dim;
    ncols = dim;
    int bw2 = bw;

    double cond = 0.0;

    if ((int)b.size() < dim * bcols)
    {
        MOOSTrace("bsolve: dimension problems with RHS\n");
        return (1e100);
    }
    // equilibration
    for (int i = 1; i <= dim; i++)
    {
        int j1 = max(1, i - bw);
        int j2 = min(dim, i + bw);
        double fac = 0.0;
        for (int j = j1; j <= j2; j++) fac = max(fac, fabs(a[indx(i, j)]));
        fac = 1.0 / fac;
        for (int j = j1; j <= j2; j++) a[indx(i, j)] = a[indx(i, j)] * fac;
        for (int j = 1; j <= bcols; j++) b[indx(i, j)] = fac * b[indx(i, j)];
    }

    for (int k = 1; k <= dim; k++)
    {
        //MOOSTrace("k=%d, pivoting\n",k);
        // pivoting
        double mx = fabs(a[indx(k, k)]);
        int imx = k;
        for (int i = k + 1; i <= min(k + bw, dim); i++)
            if (fabs(a[indx(i, k)]) > mx)
            {
                mx = fabs(a[indx(i, k)]);
                imx = i;
            }
        if (imx > k)
        {
            bw2 = max(bw2, bw + imx - k);
            for (int j = k; j <= min(k + bw2, dim); j++)
            {
                double temp = a[indx(k, j)];
                a[indx(k, j)] = a[indx(imx, j)];
                a[indx(imx, j)] = temp;
            }
            for (int j = 1; j <= bcols; j++)
            {
                double temp = b[indx(k, j)];
                b[indx(k, j)] = b[indx(imx, j)];
                b[indx(imx, j)] = temp;
            }
        }
        //MOOSTrace("k=%d, reduction\n",k);
        // reduction
        if (a[indx(k, k)] == 0.0)
            return (1e100);
        a[indx(k, k)] = 1.0 / a[indx(k, k)];
        cond = max(cond, fabs(a[indx(k, k)]));

        for (int i = k + 1; i <= min(k + bw2, dim); i++)
        {
            double fac = -a[indx(i, k)] * a[indx(k, k)];
            a[indx(i, k)] = 0.0;
            for (int j = k + 1; j <= min(dim, k + bw2); j++) a[indx(i, j)] += a[indx(k, j)] * fac;
            for (int j = 1; j <= bcols; j++) b[indx(i, j)] += b[indx(k, j)] * fac;
        }
    }
    //  back substitition
    for (int i = dim; i >= 1; i--)
    {
        //  MOOSTrace("i=%d, back substitution\n",i);

        int kmax = min(i + bw2, dim);
        for (int j = 1; j <= bcols; j++)
        {
            double ss = b[indx(i, j)];
            for (int k = i + 1; k <= kmax; k++) ss -= b[indx(k, j)] * a[indx(i, k)];
            b[indx(i, j)] = ss * a[indx(i, i)];
        }
    }

    //MOOSTrace("back substitution done\n");
    return (cond);
}

int CmyVectorUtils::indx(int i, int j) { return ((i - 1) + nrows * (j - 1)); }

int CmyVectorUtils::indx_tri(int i, int j)
{
    // vector index for packed triangular matrix including diagonal
    return ((i - 1) + (j - 1));
}

bool CmyVectorUtils::interp(const vector<double>& x, const vector<double>& y,
                            const vector<double>& x_int, vector<double>& y_int)
{
    y_int = zeros(x_int.size(), 1);
    for (int i = 0; i < (int)x_int.size(); i++)
    {
        if (x_int[i] <= x[0])
            y_int[i] = y[0];
        else if (x_int[i] >= x[x.size() - 1])
            y_int[i] = y[x.size() - 1];

        //	return(false);
        else
        {
            bool done = false;
            for (int j = 1; j < (int)x.size() && !done; j++)
            {
                if (x_int[i] < x[j])
                {
                    y_int[i] =
                        y[j - 1] + (x_int[i] - x[j - 1]) * (y[j] - y[j - 1]) / (x[j] - x[j - 1]);
                    done = true;
                }
            }
        }
    }
    return (true);
}

void CmyVectorUtils::resample(double dx, const std::vector<double>& y, double dx_new,
                              std::vector<double>& y_new)
{
    int n = y.size();
    int n_new = (int)n * dx / dx_new;
    y_new.resize(n_new);
    double oneodx = 1.0 / dx;
    for (int i = 0; i < n_new; i++)
    {
        double x_new = i * dx_new;
        int j = std::min((int)floor(x_new * oneodx), n - 2);
        y_new[i] = y[j] + (x_new - j * dx) * (y[j + 1] - y[j]) * oneodx;
    }
}

void CmyVectorUtils::vpair_sort(std::vector<double> x, std::vector<double> y,
                                std::vector<double>& x_sort, std::vector<double>& y_sort)

{
    // Note that sorting can be done in-place
    int n = x.size();
    std::vector<double> xs;
    std::vector<double> ys;
    xs.resize(n);
    ys.resize(n);
    std::vector<bool> done;
    done.resize(n);
    for (int i = 0; i < n; i++) done[i] = false;
    for (int i = 0; i < n; i++)
    {
        double minx = 1e200;
        int minj;
        for (int j = 0; j < n; j++)
        {
            if (!done[i] && x[j] < minx)
            {
                minx = x[j];
                minj = j;
            }
        }
        xs[i] = x[minj];
        ys[i] = y[minj];
        done[minj] = true;
    }
    x_sort = xs;
    y_sort = ys;
}

// >>>>>>>> Complex arithmetic <<<<<<<

complex<double> CmyVectorUtils::zmul(complex<double> a, complex<double> b)
{
    double rr = a.real() * b.real() - a.imag() * b.imag();
    double im = a.real() * b.imag() + a.imag() * b.real();
    return (complex<double>(rr, im));
}

complex<double> CmyVectorUtils::conjg(complex<double> a)
{
    double rr = a.real();
    double im = -a.imag();
    return (complex<double>(rr, im));
}

bool CmyVectorUtils::cholesky(vector<complex<double>> a, int dim, vector<complex<double>>& c)
{
    nrows = dim;
    ncols = dim;
    // cholesky returns only lower triangular
    c.resize(dim * dim);
    c = zzeros(dim, dim);
    double arg = a[indx(1, 1)].real();
    c[indx(1, 1)] = sqrt(arg);
    for (int k = 1; k <= nrows; k++)
    {
        complex<double> sum = complex<double>(0, 0);
        for (int l = 1; l <= nrows - 1; l++) sum = sum + zmul(c[indx(k, l)], conjg(c[indx(k, l)]));
        c[indx(k, k)] = sqrt(arg);
        if (c[indx(k, k)].real() == 0.0)
            return (false);
        for (int i = k + 1; i <= nrows; i++)
        {
            complex<double> sum_l = complex<double>(0, 0);
            for (int l = 1; l < k; l++) sum_l += zmul(c[indx(i, l)], conjg(c[indx(k, l)]));
            // Lower triangular
            c[indx(i, k)] = (a[indx(i, k)] - sum_l) / c[indx(k, k)].real();
        }
    }
    return (true);
}

bool CmyVectorUtils::mmul(vector<complex<double>> A, int rows_A, int cols_A,
                          vector<complex<double>> B, int cols_B, vector<complex<double>>& C)
{
    // performs complex matrix product C(rows_A,cols_B) = A(rows_A,cols_A) * B(cols_A,cols_B), assuming matrices are stored columnwize, FORTRAN style
    C.resize(rows_A * cols_B);
    for (int j = 0; j < cols_B; j++)
        for (int i = 0; i < rows_A; i++)
        {
            complex<double> sum = 0.0;
            for (int k = 0; k < cols_A; k++) sum += A[i + k * rows_A] * B[k + j * cols_A];
            C[i + j * rows_A] = sum;
        }
    return (true);
}

vector<double> CmyVectorUtils::conv(const vector<double>& A, const vector<double>& B)
{
    vector<double> C;
    C.resize(A.size() + B.size());
    for (int i = 0; i < (int)C.size(); i++) C[i] = 0.0;
    for (int t = 0; t < (int)A.size(); t++)
        for (int tau = 0; tau < (int)B.size(); tau++) C[t + tau] += A[t] * B[tau];
    return (C);
}

vector<complex<double>> CmyVectorUtils::conv(const vector<complex<double>>& A,
                                             const vector<complex<double>>& B)
{
    vector<complex<double>> C;
    C.resize(A.size() + B.size());
    for (int i = 0; i < (int)C.size(); i++) C[i] = complex<double>(0, 0);

    for (int t = 0; t < (int)A.size(); t++)
        for (int tau = 0; tau < (int)B.size(); tau++) C[t + tau] += A[t] * B[tau];
    return (C);
}

vector<vector<complex<double>>> CmyVectorUtils::cmmul(const vector<vector<complex<double>>>& A,
                                                      const vector<vector<complex<double>>>& B)
{
    vector<vector<complex<double>>> C;
    // Matrix x matrix multiplication using c++ vector<vector> indexing
    int rows_A = A.size();
    int cols_A = B.size();
    int cols_B;
    if (A.size() < 1 || A[0].size() != B.size())
    {
        cout << "cmmul: Dimension mismatch" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (B.size() > 0)
        cols_B = B[0].size();

    C.resize(rows_A);
    for (int i = 0; i < rows_A; i++)
    {
        C[i].resize(cols_B);
        for (int j = 0; j < cols_B; j++)
        {
            complex<double> sum = complex<double>(0.0);
            for (int k = 0; k < cols_A; k++) sum += A[i][k] * B[k][j];
            C[i][j] = sum;
        }
    }
    return (C);
}

vector<complex<double>> CmyVectorUtils::cmvmul(const vector<vector<complex<double>>>& A,
                                               const vector<complex<double>>& B)
{
    vector<complex<double>> C;
    // Matrix x vector multiplication using c++ vector<vector> indexing
    int rows_A = A.size();
    int cols_A = B.size();
    if (A.size() < 1 || A[0].size() != B.size())
    {
        cout << "cmvmul: Dimension mismatch" << std::endl;
        exit(EXIT_FAILURE);
    }
    C.resize(rows_A);
    for (int i = 0; i < rows_A; i++)
    {
        complex<double> sum = complex<double>(0.0);
        for (int k = 0; k < cols_A; k++) sum += A[i][k] * B[k];
        C[i] = sum;
    }
    return (C);
}

vector<vector<complex<double>>> CmyVectorUtils::transp(const vector<vector<complex<double>>>& A)
{
    vector<vector<complex<double>>> C;
    if (A.size() > 0)
    {
        C.resize(A[0].size());
        for (int i = 0; i < (int)C.size(); i++)
        {
            C[i].resize(A.size());
            for (int j = 0; j < (int)C[i].size(); j++) C[i][j] = conjg(A[j][i]);
        }
    }
    return (C);
}

vector<vector<double>> CmyVectorUtils::transp(const vector<vector<double>>& A)
{
    vector<vector<double>> C;
    if (A.size() > 0)
    {
        C.resize(A[0].size());
        for (int i = 0; i < (int)C.size(); i++)
        {
            C[i].resize(A.size());
            for (int j = 0; j < (int)C[i].size(); j++) C[i][j] = A[j][i];
        }
    }
    return (C);
}

vector<complex<double>> CmyVectorUtils::zzeros(int m, int n)
{
    vector<complex<double>> values;
    values.clear();
    for (int i = 0; i < m; i++)
        for (int j = 0; j < n; j++) values.push_back(complex<double>(0, 0));

    return (values);
}

//Added by SDanesh

vector<double> CmyVectorUtils::hann(int N) //creates hanning window of length N
{
    vector<double> window;
    window.clear();
    for (int n = 0; n < N; n++) { window.push_back(pow(sin(PI * n / (N - 1)), 2)); }
    return (window);
}

//Added by Andrew Poulsen
vector<double> CmyVectorUtils::tukey(int N, double r) //creates tukey window of length N
{
    vector<double> window;
    window.clear();
    double dx = 1.0 / (N - 1);
    int k1 = floor(0.5 * r * (N - 1)) + 1;
    int k2 = N - k1 + 1;
    if (r <= 0.0)
    {
        for (int k = 0; k < N; k++) window.push_back(1.0);
    }
    else if (r >= 1.0)
    {
        window = hann(N);
    }
    else
    {
        for (int k = 0; k < N; k++)
        {
            if (k < k1)
                window.push_back(0.5 * (1 + cos((2 * PI / r) * (dx * k - r * 0.5))));
            else if (k < (k2 - 1))
                window.push_back(1.0);
            else
                window.push_back(0.5 * (1 + cos((2 * PI / r) * (dx * k - 1 + r * 0.5))));
        }
    }
    return (window);
}

int CmyVectorUtils::NN(double num, vector<double> myvec) //Find Nearest Neighbor
//myvec must be monotonically INCREASING
{
    double dif1, dif2;
    int index;
    int dflt = myvec.size();
    //Nearester neighbor - resturns index in vector
    for (unsigned int i = 0; i < myvec.size() - 1; i++)
    {
        if ((num > myvec[i]) && (num <= myvec[i + 1]))
        {
            dif1 = abs(num - myvec[i]);
            dif2 = abs(num - myvec[i + 1]);
            index = i;
            break;
        }
    }
    if (dif1 <= dif2)
    {
        return (index);
    }
    if (dif2 < dif1)
    {
        return (index + 1);
    }
    else
        return (dflt);
}

vector<vector<double>> CmyVectorUtils::zero_matrix(int n, int m)
{
    vector<vector<double>> my_matrix;
    vector<double> myvals;
    my_matrix.clear();
    for (int i = 0; i < n; i++)
    {
        myvals.clear();
        for (int j = 0; j < m; j++) { myvals.push_back(0); }
        my_matrix.push_back(myvals);
    }
    return (my_matrix);
}

void CmyVectorUtils::lin_least_square(const vector<double>& x, const vector<double>& y, double x_0,
                                      double& y_0, double& slope, double& rms_error)
{
    int n = x.size();
    vector<double> A, At;
    A.resize(n * 2);
    for (int i = 0; i < n; i++)
    {
        A[i] = 1;
        A[i + n] = (x[i] - x_0);
    }

    At.resize(n * 2);
    for (int i = 0; i < n; i++)
    {
        At[i * 2] = A[i];
        At[i * 2 + 1] = A[i + n];
    }

    vector<double> C, Cinv;
    mmul(At, 2, n, A, 2, C);
    vector<double> B;
    mmul(At, 2, n, y, 1, B);
    minv(C, 2, Cinv);
    mmul(Cinv, 2, 2, B, 1, At);
    y_0 = At[0];
    slope = At[1];
    double sumsq = 0.0;
    for (int i = 0; i < n; i++) sumsq += pow(y_0 * A[i] + slope * A[i + n] - y[i], 2);
    rms_error = 2 * sqrt(sumsq / n) / (x[n - 1] - x[0]);
}

void CmyVectorUtils::prune_timeseries(vector<double> t, vector<double> y, double t_now,
                                      double history, vector<double>& t_new, vector<double>& y_new)
{
    // prune timeseries
    int first = 0;
    int num_t = t.size();
    for (int i = 0; i < num_t; i++)
    {
        if (t_now - t[i] > history)
            first = i + 1;
    }
    if (first > 0)
    {
        num_t -= first;
        t_new.resize(num_t);
        y_new.resize(num_t);
        for (int i = 0; i < num_t; i++)
        {
            t_new[i] = t[i + first];
            y_new[i] = y[i + first];
        }
    }
    else
    {
        t_new = t;
        y_new = y;
    }
}

} // namespace core
} // namespace lamss
