// H. Schmidt, henrik@mit.edu 08.18.10
// Laboratory for Autonomous Marine Sensing Systems (LAMSS)
// Massachusetts Institute of Technology
//
// CEigenRays.cpp
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

#include "CEigenRays.h"

using namespace std;

bool CEigenRayTable::readEigenRayTable(string arr_file_path, int max_num_bounces)
{
    // Returns the number of eigenrays and builds the ray table from the
    // results produced by Bellhop in a .arr file

    eigenray_table.clear();

    std::ifstream arr_file;
    arr_file.open(arr_file_path.c_str());
    num_depths = 0;
    num_ranges = 0;
    if (arr_file.is_open())
    {
        string nDim;
        arr_file >> nDim;

        double freq;
        arr_file >> freq;

        int num_sources;
        arr_file >> num_sources;

        if (num_sources != 1)
        {
            cout << "readEigenRayTable: Error - Only one source  allowed " << std::endl;
            return (false);
        }

        arr_file >> source_depth;

        arr_file >> num_depths;
        if (isnan(num_depths) || num_depths <= 0)
            return (false);
        depth.clear();
        for (int i = 0; i < num_depths; i++)
        {
            double buf;
            arr_file >> buf;
            depth.push_back(buf);
        }

        arr_file >> num_ranges;
        if (isnan(num_ranges) || num_ranges <= 0)
            return (false);
        range.clear();
        for (int j = 0; j < num_ranges; j++)
        {
            double buf;
            arr_file >> buf;
            range.push_back(buf);
        }

        double unused;
        arr_file >> unused;

        for (int i = 0; i < num_depths; i++)
        {
            for (int j = 0; j < num_ranges; j++)
            {
                int num_rays_total = 0;
                CEigenRay eigbuf;
                eigbuf.clear();
                eigbuf.source_depth = source_depth;
                eigbuf.receiver_depth = depth[i];
                eigbuf.freq = freq;
                eigbuf.range = range[j];
                arr_file >> num_rays_total;
                if (isnan(num_rays_total))
                    return (false);
                for (int k = 0; k < num_rays_total; k++)
                {
                    CRay ray;
                    double val;
                    arr_file >> ray.amp;
                    bool valid = !isnan(ray.amp);
                    arr_file >> val;
                    ray.phase = val * M_PI / 180.0;

                    // 2019-09-10 :: Oscar Viquez (oviquezr@mit.edu)
                    // Bellhop 2019 uses a complex delay formulation -- keeping real part only
                    arr_file >> ray.tau;
                    arr_file >> val; // dismiss imaginary part of delay

                    // Continue reading file:
                    arr_file >> val;
                    ray.dip_source = val * M_PI / 180.0;
                    arr_file >> val;
                    ray.dip_receiver = val * M_PI / 180.0;
                    arr_file >> ray.surface_bounces;
                    arr_file >> ray.bottom_bounces;
                    if (valid && ray.surface_bounces + ray.bottom_bounces <= max_num_bounces)
                        eigbuf.bundle.push_back(ray);
                }
                eigbuf.num_rays = eigbuf.bundle.size();
                eigenray_table.push_back(eigbuf);
            }
        }
    }

    return (true);
}

bool CEigenRayTable::getEigenRays(double req_range, double req_depth, double& act_range,
                                  double& act_depth, CEigenRay& eig)
{
    // find closest depth and range in table

    double dist = 1e20;
    int i_close = 0;
    int j_close = 0;
    eig.clear();

    for (int i = 0; i < num_depths; i++)
    {
        for (int j = 0; j < num_ranges; j++)
        {
            double d = sqrt(pow(req_range - range[j], 2) + pow(req_depth - depth[i], 2));
            if (d < dist)
            {
                dist = d;
                i_close = i;
                j_close = j;
            }
        }
    }
    act_range = range[j_close];
    act_depth = depth[i_close];
    eig = eigenray_table[i_close * num_ranges + j_close];
    return (true);
}

bool CEigenRayTable::putEigenRay(double sd, double rd, double f, double r, CRay ray)
{
    // Populates the ray table with single ray, Used for RAP mode.
    CEigenRay eigbuf;

    eigbuf.source_depth = sd;

    eigbuf.bundle.clear();
    eigbuf.receiver_depth = rd;
    eigbuf.freq = f;
    eigbuf.range = r;
    eigbuf.bundle.push_back(ray);
    eigbuf.num_rays = 1;

    clear();
    source_depth = sd;
    num_ranges = 1;
    num_depths = 1;
    range.push_back(r);
    depth.push_back(rd);
    eigenray_table.push_back(eigbuf);

    return (true);
}

void CEigenRayTable::clear()
{
    // Clear eigenray table
    num_ranges = 0;
    num_depths = 0;
    range.clear();
    depth.clear();

    eigenray_table.clear();
}

int CEigenRayTable::getRanges(vector<double>& ranges)
{
    ranges = range;
    return (range.size());
}

int CEigenRayTable::getDepths(vector<double>& depths)
{
    depths = depth;
    return (depth.size());
}

void CEigenRay::clear()
{
    // Clear eigenrays

    num_rays = 0;
    bundle.clear();
}

std::vector<CEigenRay> CEigenRayTable::getEigenRayTable(string arr_file_path, int max_num_bounces)
{
    // Returns the number of eigenrays and builds the ray table from the
    // results produced by Bellhop in a .arr file

    eigenray_table.clear();

    std::ifstream arr_file;
    arr_file.open(arr_file_path.c_str());

    if (arr_file.is_open())
    {
        string nDim;
        arr_file >> nDim;

        double freq;
        arr_file >> freq;

        int num_sources;
        arr_file >> num_sources;

        if (num_sources != 1)
        {
            MOOSTrace("readEigenRayTable: Error - Only one source  allowed \n");
        }

        arr_file >> source_depth;

        arr_file >> num_depths;
        depth.clear();
        for (int i = 0; i < num_depths; i++)
        {
            double buf;
            arr_file >> buf;
            depth.push_back(buf);
        }

        arr_file >> num_ranges;
        range.clear();
        for (int j = 0; j < num_ranges; j++)
        {
            double buf;
            arr_file >> buf;
            range.push_back(buf);
        }

        double unused;
        arr_file >> unused;

        for (int i = 0; i < num_depths; i++)
        {
            for (int j = 0; j < num_ranges; j++)
            {
                int num_rays_total;
                CEigenRay eigbuf;
                eigbuf.clear();
                eigbuf.source_depth = source_depth;
                eigbuf.receiver_depth = depth[i];
                eigbuf.freq = freq;
                eigbuf.range = range[j];

                arr_file >> num_rays_total;
                for (int k = 0; k < num_rays_total; k++)
                {
                    CRay ray;
                    double val;
                    arr_file >> ray.amp;
                    bool valid = !isnan(ray.amp);
                    arr_file >> val;
                    ray.phase = val * M_PI / 180.0;

                    // 2019-09-10 :: Oscar Viquez (oviquezr@mit.edu)
                    // Bellhop 2019 uses a complex delay formulation -- keeping real part only
                    arr_file >> ray.tau;
                    arr_file >> val; // dismiss imaginary part of delay

                    // Continue reading file:
                    arr_file >> val;
                    ray.dip_source = val * M_PI / 180.0;
                    arr_file >> val;
                    ray.dip_receiver = val * M_PI / 180.0;
                    arr_file >> ray.surface_bounces;
                    arr_file >> ray.bottom_bounces;
                    if (valid && ray.surface_bounces + ray.bottom_bounces <= max_num_bounces)
                        eigbuf.bundle.push_back(ray);
                }
                eigbuf.num_rays = eigbuf.bundle.size();
                eigenray_table.push_back(eigbuf);
            }
        }
        arr_file.close();
    }

    return (eigenray_table);
}
