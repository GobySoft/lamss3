// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

// Usage: make_flat_bathy <file.nc> <lat_origin> <lon_origin> <depth_m>
//
// Writes a NOAA-style bathymetry grid (lat, lon, short z(lat, lon), negative down) of constant depth
// covering +/- 0.1 degrees around the origin, the layout iNoaaBathy reads.

#include <cstdlib>
#include <iostream>
#include <vector>

#include <netcdf.h>

namespace
{
void check(int status)
{
    if (status != NC_NOERR)
    {
        std::cerr << "make_flat_bathy: " << nc_strerror(status) << std::endl;
        std::exit(1);
    }
}
} // namespace

int main(int argc, char** argv)
{
    if (argc != 5)
    {
        std::cerr << "usage: make_flat_bathy <file.nc> <lat_origin> <lon_origin> <depth_m>"
                  << std::endl;
        return 2;
    }
    const double lat0 = std::atof(argv[2]);
    const double lon0 = std::atof(argv[3]);
    const short z = static_cast<short>(-std::atof(argv[4]));
    const int n = 21;
    const double step = 0.01;

    std::vector<double> lat(n), lon(n);
    for (int i = 0; i < n; ++i)
    {
        lat[i] = lat0 + (i - n / 2) * step;
        lon[i] = lon0 + (i - n / 2) * step;
    }
    std::vector<short> depth(n * n, z);

    int ncid, lat_dim, lon_dim, lat_var, lon_var, z_var;
    check(nc_create(argv[1], NC_CLOBBER, &ncid));
    check(nc_def_dim(ncid, "lat", n, &lat_dim));
    check(nc_def_dim(ncid, "lon", n, &lon_dim));
    check(nc_def_var(ncid, "lat", NC_DOUBLE, 1, &lat_dim, &lat_var));
    check(nc_def_var(ncid, "lon", NC_DOUBLE, 1, &lon_dim, &lon_var));
    const int dims[] = {lat_dim, lon_dim};
    check(nc_def_var(ncid, "z", NC_SHORT, 2, dims, &z_var));
    check(nc_enddef(ncid));
    check(nc_put_var_double(ncid, lat_var, lat.data()));
    check(nc_put_var_double(ncid, lon_var, lon.data()));
    check(nc_put_var_short(ncid, z_var, depth.data()));
    check(nc_close(ncid));
    return 0;
}
