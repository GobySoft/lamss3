// Copyright 2012-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "lamss/core/henrik_util/CMeasAvg.h"

using namespace std;

namespace lamss
{
namespace core
{

//****************************************************************************
//*  MeasAvg::set_RawVal()                                                   *
//*                                                                          *
//*  This method calculates the moving average of a sequence of measurements *
//*                                                                          *
//****************************************************************************

void CMeasAvg::configAvg(int length)
{
    len = length;
    num_measurements = 0;
}

void CMeasAvg::setVal(double val)
{
    if (num_measurements < len)
    {
        vals.push_back(val);
        num_measurements += 1;
    }
    else
    {
        vals.push_back(val);
        vals.pop_front();
        num_measurements = len;
    }

    double sum = 0.0;

    for (int kk = 0; kk < num_measurements; kk++) { sum += vals[kk]; }

    avg = sum / num_measurements;
}

//***************************************************************************
//*  MeasAvg::getAvg()                                                       *
//*                                                                         *
//*  This method returns the value of the moving average                    *
//*                                                                         *
//***************************************************************************
double CMeasAvg::getAvg()
{
    if (num_measurements > 0)
        return (avg);
    else
        return (0.0);
}

//***************************************************************************
//*  CAngleAvg::setVal()                                                    *
//*                                                                         *
//*  This method calculates the moving average of a sequence of angles in   *
//*  degrees                                                                *
//***************************************************************************

void CAngleAvg::configAvg(int length)
{
    len = length;
    num_measurements = 0;
}

void CAngleAvg::setVal(double val)
{
    double csum, ssum, s, c;

    if (num_measurements < len)
    {
        s = sin(val * M_PI / 180.0);
        c = cos(val * M_PI / 180.0);
        svals.push_back(s);
        cvals.push_back(c);
        num_measurements += 1;
    }
    else
    {
        s = sin(val * M_PI / 180);
        c = cos(val * M_PI / 180.0);
        svals.push_back(s);
        cvals.push_back(c);
        svals.pop_front();
        cvals.pop_front();
        num_measurements = len;
    }

    csum = 0.0;
    ssum = 0.0;

    for (int kk = 0; kk < num_measurements; kk++)
    {
        csum += cvals[kk];
        ssum += svals[kk];
    }

    avg = atan2(ssum / num_measurements, csum / num_measurements) * 180.0 / M_PI;
    //MOOSTrace("ssum = %f, csum = %f, average angle = %f\n",ssum,csum,avg);
}

//***************************************************************************
//*  CAngleAvg::get_FVal()                                                       *
//*                                                                         *
//*  This method returns the value of the moving average                    *
//*                                                                         *
//***************************************************************************
double CAngleAvg::getAvg()
{
    if (num_measurements > 0)
        return (avg);
    else
        return (0.0);
}

//***************************************************************************
//*  CRadianAvg::setVal()                                                    *
//*                                                                         *
//*  This method calculates the moving average of a sequence of angles in   *
//*  radians                                                                *
//***************************************************************************

void CRadianAvg::configAvg(int length)
{
    len = length;
    num_measurements = 0;
}

void CRadianAvg::setVal(double val)
{
    double csum, ssum, s, c;

    if (num_measurements < len)
    {
        s = sin(val);
        c = cos(val);
        svals.push_back(s);
        cvals.push_back(c);
        num_measurements += 1;
    }
    else
    {
        s = sin(val);
        c = cos(val);
        svals.push_back(s);
        cvals.push_back(c);
        svals.pop_front();
        cvals.pop_front();
        num_measurements = len;
    }

    csum = 0.0;
    ssum = 0.0;

    for (int kk = 0; kk < num_measurements; kk++)
    {
        csum += cvals[kk];
        ssum += svals[kk];
    }

    avg = atan2(ssum / num_measurements, csum / num_measurements);
}

//***************************************************************************
//*  CRadianAvg::get_FVal()                                                       *
//*                                                                         *
//*  This method returns the value of the moving average                    *
//*                                                                         *
//***************************************************************************
double CRadianAvg::getAvg()
{
    if (num_measurements > 0)
        return (avg);
    else
        return (0.0);
}

} // namespace core
} // namespace lamss
