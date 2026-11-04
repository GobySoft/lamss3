// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __CEIGENRAYS_h__
#define __CEIGENRAYS_h__

#include <complex>
#include <fstream>
#include <math.h>
#include <string.h>
#include <string>
#include <vector>

namespace lamss
{
namespace core
{

class CRay
{
  public:
    double amp;
    double phase;
    double tau;
    double dip_source;
    double dip_receiver;
    int surface_bounces;
    int bottom_bounces;
};

class CEigenRay
{
  public:
    void clear();

    double source_depth;
    double receiver_depth;
    double freq;
    double range;
    int num_rays;
    std::vector<CRay> bundle;

  protected:
};

class CEigenRayTable
{
  public:
    bool readEigenRayTable(std::string arr_file_path, int max_num_bounces);
    bool getEigenRays(double req_range, double req_depth, double& act_range, double& act_depth,
                      CEigenRay& eig);
    bool putEigenRay(double sd, double rd, double f, double r, CRay ray);
    int getDepths(std::vector<double>& depths);
    int getRanges(std::vector<double>& ranges);
    std::vector<CEigenRay> getEigenRayTable(std::string arr_file_path, int max_num_bounces);

    void clear();

    double source_depth;
    int num_ranges;
    int num_depths;

  protected:
    std::vector<double> range;
    std::vector<double> depth;
    std::vector<CEigenRay> eigenray_table;
};

} // namespace core
} // namespace lamss

#endif /* __CEIGENRAYS_h__ */
