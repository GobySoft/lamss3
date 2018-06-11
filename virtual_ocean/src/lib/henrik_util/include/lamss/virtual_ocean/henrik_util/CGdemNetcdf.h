// Copyright 2018 Henrik Schmidt
//                Laboratory for Autonoimous marine Sensing Systems
//                Massachusetts Institute of Technology
//
// Created: June 2018

#ifndef CGDEMNETCDF
#define CGDEMNETCDF

#include "goby/util/sci.h"
#include <cmath>
#include <ctime>
#include <limits>
#include "lamss/lib_henrik_util/CVectorUtils.h"
#include "MOOS/libMOOSGeodesy/MOOSGeodesy.h"
#include <string>
#include <iostream>
#include <netcdf.h>

class GdemNetCdf
{
 public:

  // GDEM specific functions and variables
  bool open_nc(std::string);
  void init_geodesy(double lat_origin, double lon_origin);
  std::vector<double> get_depth_profile(double lat_, double lon_,
					std::vector<double> dep_,
					int month);
  double interp_ssp(double x,double y,
		    const std::vector<double>& grid_x,
		    const std::vector<double>& grid_y,
		    const std::vector<double>& grid_z);
  
  std::vector<double> lat_vec_;
  std::vector<double> lon_vec_;
  std::vector<double> dep_vec_;
  std::vector<float> time_vec_;
  int ncid_;
  bool nc_is_open_;
  int latid_;
  int lonid_;
  int depid_;
  int timeid_;
  int sspid_;
  int lat_varid_;
  int lon_varid_;
  int dep_varid_;
  int time_varid_;
  int ssp_varid_;

  CMOOSGeodesy geodesy_;
  CmyVectorUtils vecUtils;    

  ///////
};

#endif 
