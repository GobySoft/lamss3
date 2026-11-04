// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "lamss/core/henrik_util/CBessel.h"

using namespace std;

namespace lamss
{
namespace core
{

double CBesTable::interp(double x)
{
    // *** INTERPOLATES IN A REAL ARRAY value TO DETERMINE VALUE
    // *** FOR ARGUMENT x using 4 pt Lagrange interpolation
    if (x > x_max)
    {
        cout << "interp: Argument overflow";
        return (0.0);
    }
    else
    {
        double rindex = (x - x_min) * onodx;
        int index = int(rindex - 0.5);
        if ((size_t)index > value.size() - 3)
            index = value.size() - 3;
        if (index < 1)
            index = 1;
        double rem = rindex - index;
        double res = -0.1666667 * rem * (rem - 1E0) * (rem - 2E0) * value[index - 1] +
                     0.5 * (rem * rem - 1E0) * (rem - 2E0) * value[index] -
                     0.5 * rem * (rem + 1E0) * (rem - 2E0) * value[index + 1] +
                     0.1666667 * rem * (rem * rem - 1E0) * value[index + 2];
        return (res);
    }
}

double CBessel::bessel_J(int m, double x)
{
    if (x > bes_table[0].x_max)
    {
        // expand tables
        int nx = x * bes_table[0].onodx + 1;
        int nx_old = bes_table[0].value.size();
        for (int ix = nx_old; ix <= nx; ix++)
        {
            vector<double> besfun;
            bessjn(bes_table.size(), ix * bes_table[0].dx, besfun);
            for (int m = 0; m < (int)bes_table.size(); m++)
            {
                bes_table[m].x_max = nx * bes_table[0].dx;
                bes_table[m].value.push_back(besfun[m]);
            }
        }
    }
    return (bes_table[m].interp(x));
}

bool CBessel::initBessel(int max_order, double max_x)
{
    double dx = M_PI / 20.0;
    double onodx = 1.0 / dx;
    int nx = max_x * onodx + 1;
    bes_table.clear();
    for (int m = 0; m <= max_order; m++)
    {
        CBesTable empty;
        empty.dx = dx;
        empty.onodx = onodx;
        empty.x_min = 0;
        empty.x_max = nx * dx;
        for (int ix = 0; ix <= nx; ix++) empty.value.push_back(0.0);
        bes_table.push_back(empty);
    }
    // MOOSTrace("initialized table\n");
    for (int ix = 0; ix <= nx; ix++)
    {
        // MOOSTrace("calling bessjn, x=%f\n",ix*dx);
        vector<double> besfun;
        bessjn(max_order, ix * dx, besfun);
        // MOOSTrace("returning from bessjn,j_0-2=%f,%f,%f\n",besfun[0],besfun[1],besfun[2]);
        for (int m = 0; m <= max_order; m++) bes_table[m].value[ix] = besfun[m];
    }
    return (true);
}

double CBessel::bessj0(double x)
{
    // Calculates the Bessel function J_0(x) using series expansions
    double ax = fabs(x);
    double xx, y, z, ans, ans1, ans2;

    if (ax < 8.0)
    {
        y = x * x;
        ans1 = 57568490574.0e0 +
               y * (-13362590354.0e0 +
                    y * (651619640.7e0 +
                         y * (-11214424.18e0 + y * (77392.33017e0 + y * (-184.9052456e0)))));
        ans2 = 57568490411.0e0 +
               y * (1029532985.0e0 +
                    y * (9494680.718e0 + y * (59272.64853e0 + y * (267.8532712e0 + y * 1.0e0))));
        ans = ans1 / ans2;
    }
    else
    {
        z = 8.0 / ax;
        y = z * z;
        xx = ax - 0.785398164;
        ans1 = 1.0e0 + y * (-0.1098628627e-2 +
                            y * (0.2734510407e-4 + y * (-0.2073370639e-5 + y * 0.2093887211e-6)));
        ans2 = -0.1562499995e-1 +
               y * (0.1430488765e-3 +
                    y * (-0.6911147651e-5 + y * (0.7621095161e-6 - y * 0.934935152e-7)));
        ans = sqrt(0.636619772e0 / ax) * (cos(xx) * ans1 - z * sin(xx) * ans2);
    }
    return (ans);
}

double CBessel::bessj1(double x)
{
    // Calculates the Bessel function J_0(x) using series expansions

    double z, xx, y, ans, ans1, ans2;
    double ax = fabs(x);
    if (ax < 8.0)
    {
        y = x * x;
        ans1 = x * (72362614232.0e0 +
                    y * (-7895059235.0e0 +
                         y * (242396853.1e0 +
                              y * (-2972611.439e0 + y * (15704.48260e0 + y * (-30.16036606e0))))));
        ans2 = 144725228442.0e0 +
               y * (2300535178.0e0 +
                    y * (18583304.74e0 + y * (99447.43394e0 + y * (376.9991397e0 + y * 1.0e0))));
        ans = ans1 / ans2;
    }
    else
    {
        z = 8.0 / ax;
        y = z * z;
        xx = ax - 2.356194491e0;
        ans1 = 1.0e0 + y * (0.183105e-2 +
                            y * (-0.3516396496e-4 + y * (0.2457520174e-5 + y * (-0.240337019e-6))));
        ans2 = 0.04687499995e0 +
               y * (-0.2002690873e-3 +
                    y * (0.8449199096e-5 + y * (-0.88228987e-6 + y * 0.105787412e-6)));
        ans = sqrt(0.636619772e0 / ax) * (cos(xx) * ans1 - z * sin(xx) * ans2);
        if (x < 0.0)
            ans = -ans;
    }
    return (ans);
}

void CBessel::bessjn(int n, double x, vector<double>& bjn)
{
    // Computes bessel function of first kind and order 0,1,..., n
    // using Miller's recursion device for arguments
    // smaller than max order.
    int iacc = 40;
    double bigno = 1.0e30;
    double bigni = 1.0e-30;
    bjn.clear();
    // MOOSTrace("bessjn - n=%d, x=%f\n",n,x);
    if (fabs(x) < 1e-12)
    {
        bjn.push_back(1.0);
        for (int i = 1; i <= n; i++) bjn.push_back(0.0);
    }
    else if (n == 0)
        bjn.push_back(bessj0(x));
    else if (n == 1)
    {
        bjn.push_back(bessj0(x));
        bjn.push_back(bessj1(x));
    }
    else if (x > (double)n)
    {
        double tox = 2.0 / x;
        bjn.push_back(bessj0(x));
        bjn.push_back(bessj1(x));
        for (int i = 1; i < n; i++)
        {
            double rec = i * tox * bjn[i] - bjn[i - 1];
            bjn.push_back(rec);
        }
    }
    else
    {
        // MOOSTrace("bessjn - entering  Miller n=%d, x=%f\n",n,x);
        double tox = 2.0 / x;
        int m = 2 * int(n + sqrt(double(iacc * n)) / 2.0);
        double jsum = 0;
        double sum = 0.0;
        double bjp1 = 0.0;
        double bj = 1.0;
        // MOOSTrace("bessjn - n=%d, loop i\n",n);
        for (int i = 0; i <= n; i++) bjn.push_back(0.0);

        for (unsigned int i = m; i > 0; i--)
        {
            // MOOSTrace("bessjn - loop i=%d\n",i);
            double bjm1 = i * tox * bj - bjp1;
            bjp1 = bj;
            bj = bjm1;
            if (fabs(bj) > bigno)
            {
                // MOOSTrace("bessjn - loop i=%d - scaling\n",i);
                bj *= bigni;
                bjp1 *= bigni;
                for (int j = i + 2; j <= n + 1; j++) bjn[j - 1] *= bigni;
                sum *= bigni;
            }
            if (jsum != 0.0)
                sum += bj;
            jsum = 1 - jsum;
            if (i <= (unsigned int)n)
                bjn[i] = bjp1;
        }
        // MOOSTrace("bessjn - loop i done\n");

        bjn[0] = bj;
        sum = 2.0 * sum - bj;
        for (int j = 0; j <= n; j++)
        {
            // MOOSTrace("bessjn - loop j=%d\n",j);
            bjn[j] /= sum;
        }
    }
    // MOOSTrace("bessjn done\n");

    return;
}

bool CBessel::initSphBes(int max_order, double max_x)
{
    double dx = M_PI / 20.0;
    double onodx = 1.0 / dx;
    int nx = max_x * onodx + 1;
    sphbes_table.clear();
    for (int m = 0; m <= max_order; m++)
    {
        CBesTable empty;
        empty.dx = dx;
        empty.onodx = onodx;
        empty.x_min = 0;
        empty.x_max = nx * dx;
        for (int ix = 0; ix <= nx; ix++) empty.value.push_back(0.0);
        sphbes_table.push_back(empty);
    }
    for (int ix = 0; ix <= nx; ix++)
    {
        vector<double> besfun;
        sphbesjn(max_order, ix * dx, besfun);
        for (int m = 0; m <= max_order; m++) sphbes_table[m].value[ix] = besfun[m];
    }
    return (true);
}

void CBessel::sphbesjn(int n, double x, vector<double>& bjn)
{
    // Computes spherical bessel function of first kind and order 0,1,..., n
    // using Miller's recursion device for arguments
    // smaller than max order.
    int iacc = 40;
    double bigno = 1.0e30;
    double bigni = 1.0e-30;
    bjn.clear();
    if (fabs(x) < 1e-12)
    {
        bjn.push_back(1.0);
        for (int i = 1; i <= n; i++) bjn.push_back(0.0);
    }
    else if (n == 0)
        bjn.push_back(sin(x) / x);
    else if (n == 1)
    {
        bjn.push_back(sin(x) / x);
        bjn.push_back(sin(x) / pow(x, 2) - cos(x) / x);
    }
    else if (x > (double)n)
    {
        double onox = 1.0 / x;
        bjn.push_back(sin(x) / x);
        bjn.push_back(sin(x) / pow(x, 2) - cos(x) / x);
        for (int i = 2; i <= n; i++)
        {
            double rec = (2 * (i - 1) + 1) * onox * bjn[i - 1] - bjn[i - 2];
            bjn.push_back(rec);
        }
    }
    else
    {
        double onox = 1.0 / x;
        int m = 2 * int(n + sqrt(double(iacc * n)) / 2.0);
        double bjp1 = 0.0;
        double bj = 1.0;
        for (int i = 0; i <= n; i++) bjn.push_back(0.0);
        for (unsigned int i = m; i > 0; i--)
        {
            double bjm1 = (2 * i + 1) * onox * bj - bjp1;
            bjp1 = bj;
            bj = bjm1;
            if (fabs(bj) > bigno)
            {
                bj *= bigni;
                bjp1 *= bigni;
                for (int j = i + 2; j <= n + 1; j++) bjn[j - 1] *= bigni;
            }
            if (i <= (unsigned int)n)
                bjn[i] = bjp1;
        }

        bjn[0] = bj;
        double scale = sin(x) / (x * bj);
        for (int j = 0; j <= n; j++) bjn[j] *= scale;
    }
    return;
}

double CBessel::sphbes_j(int m, double x)
{
    if (x > sphbes_table[0].x_max)
    {
        // expand tables
        int nx = x * sphbes_table[0].onodx + 1;
        int nx_old = sphbes_table[0].value.size();
        for (int ix = nx_old; ix <= nx; ix++)
        {
            vector<double> besfun;
            sphbesjn(sphbes_table.size(), ix * sphbes_table[0].dx, besfun);
            for (int m = 0; m <= (int)sphbes_table.size(); m++)
            {
                sphbes_table[m].x_max = nx * sphbes_table[0].dx;
                sphbes_table[m].value.push_back(besfun[m]);
            }
        }
    }
    return (sphbes_table[m].interp(x));
}

} // namespace core
} // namespace lamss
