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

// Date number of Netcdf time origin
#define MSEAS_TIME_ORIGIN 730486  // datenum 2000-1-1-00.00.00
#define MSEAS_TIME_UNIT 1    // secs. MSEAS times are in seconds from origin
#define HYCOM_TIME_ORIGIN 730486  // datenum 2000-1-1-00.00.00
#define HYCOM_TIME_UNIT  3600    // secs. HYCOM times are in hours from origin
#define GDEM_TIME_ORIGIN 0        // datenum 0000-1-1-00.00.00
#define GDEM_TIME_UNIT  86400    // secs. GDEM times are in days from origin
#define UTC_ORIGIN 719529 // datenum of 1970-1-1-00:00:00
class LamssNetCdf
{
 public:

  // GDEM specific functions and variables
  void initialize_gdem_nc(std::string nc_file,
			  double lat_origin, double lon_origin,
			  double moos_start, bool fixed_start);
  bool open_gdem_nc(std::string);
  std::vector<double> get_gdem_profile(double lat_, double lon_,
				       std::vector<double> dep_,
				       int month);
  // NOAA specific functions and variables
  void initialize_noaa_nc(std::string nc_file,
			  double lat_origin, double lon_origin,
			  double moos_start, bool fixed_start);
  bool open_noaa_nc(std::string);
  double get_noaa_bathy(double lat, double lon);

  // MSEAS specific functions and variables 
  void initialize_mseas_nc(std::string nc_file,
			   double lat_origin, double lon_origin,
			   double moos_start, bool fixed_start);
  bool open_mseas_nc(std::string);
  double get_mseas_bathy(double lat, double lon);
  void get_mseas_profile( double lat_, double lon_,
			  const std::vector<double>& dep_,
			  double curr_time,
			  std::vector<double>& temp,
			  std::vector<double>& salt);
  int tgrid2id_;
  int tgrid3id_;
  double moos_start_time;
  
  // HYCOM specific functions and variables
  void initialize_hycom_nc(std::string nc_file,
			   double lat_origin, double lon_origin,
			   double moos_start, bool fixed_start);
  bool open_hycom_nc(std::string);
  void get_hycom_profile(double lat_, double lon_,
			 const std::vector<double>& dep_,
			 double utc_time,
			 std::vector<double>& temp,
			 std::vector<double>& salt);

  float temp_offset;
  float temp_scaling;
  float temp_missing;
  float salt_offset;
  float salt_scaling;
  float salt_missing;
  
  // Generic arrays and variables
  bool open_nc_file(std::string);
  void init_geodesy(double lat_origin, double lon_origin);
  void init_timebase(double moos_start_time, double utc_start, double utc_incr, bool fixed_start);
  double interp_triangle(double x,double y,
			 const std::vector<double>& grid_x,
			 const std::vector<double>& grid_y,
			 const std::vector<double>& grid_z);
  bool geodesy_initialized;
  double nc_utc_start;
  double nc_utc_delta;
  double nc_utc_duration;
  bool fixed_model_start;
  std::vector<double> lat_vec_;
  std::vector<double> lon_vec_;
  std::vector<double> dep_vec_;
  std::vector<float> time_vec_;
  std::vector<float> utc_vec_;
  int ncid_;
  bool nc_is_open_;
  int latid_;
  int lonid_;
  int depid_;
  int timeid_;
  int sspid_;
  int bathyid_;
    
  int lat_varid_;
  int lon_varid_;
  int dep_varid_;
  int time_varid_;
  int ssp_varid_;
  int temp_varid_;
  int salt_varid_;
  int bathy_varid_;
  
  CMOOSGeodesy geodesy_;
  CmyVectorUtils vecUtils;    

  ///////
};

#endif 
