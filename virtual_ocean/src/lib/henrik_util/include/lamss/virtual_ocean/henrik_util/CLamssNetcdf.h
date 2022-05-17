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
#include "goby/moos/moos_geodesy.h"
#include <string>
#include <sstream>
#include <iostream>
#include <netcdf.h>
#include <vector>

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

  // Generic arrays and variables
  bool open_nc_file(std::string);
  bool close_nc_file();
  bool get_lat_lon_dimensions();
  void get_depth_dimension();
  void get_time_dimension();
  void get_lat_lon();
  void get_depths();
  void get_times();
  bool closest_lat_lon(double lat_, double lon_,
		       std::vector<double>& min_lat_val,
		       std::vector<double>& min_lon_val,
		       std::vector<double>& min_lat_m,
		       std::vector<double>& min_lon_m,
		       std::vector<int>& min_ilat, 
		       std::vector<int>& min_ilon);
  void init_geodesy(double lat_origin, double lon_origin);
  void init_timebase(double moos_start_time, double utc_start, double utc_incr, bool fixed_start);
  double interp_triangle(double x,double y,
			 const std::vector<double>& grid_x,
			 const std::vector<double>& grid_y,
			 const std::vector<double>& grid_z);
  double datenum(const std::vector<int>& date_vec);
  bool extract_date(const std::string& in, std::vector<int>& date,
		    double& units_secs);
  std::vector<int> date_vec_;
  double datenum_start;
  
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
  std::vector<double> weights_vec_;
  std::vector<double> baseline_weights_vec_;

  
  int ncid_;
  bool nc_is_open_;
  int latid_;
  int lonid_;
  int depid_;
  int timeid_;
  int sspid_;
  int bathyid_;
  int eofid_;
    
  int lat_varid_;
  int lon_varid_;
  int dep_varid_;
  int time_varid_;
  int ssp_varid_;
  int temp_varid_;
  int salt_varid_;
  int bathy_varid_;

  std::string time_units_;
  double nc_time_unit;
  
  bool scewed_grid;
  
  int status;  // error status 

  // Netcdf variable names
  std::string lon_ncname;
  std::string lat_ncname;
  std::string dep_ncname;
  std::string time_ncname;
  std::string tsc_ncname;
  std::string temp_ncname;
  std::string salt_ncname;

  std::string nc_vars;
  
  // Dimension lengths  
  size_t lat_len;
  size_t lon_len;
  size_t dep_len;
  size_t time_len;
  size_t eof_len;

  goby::moos::CMOOSGeodesy geodesy_;
  CmyVectorUtils vecUtils;    

  ///////
  // GDEM specific functions and variables
  void initialize_gdem_nc(std::string nc_file,
			  double lat_origin, double lon_origin,
			  double moos_start, bool fixed_start);
  bool open_gdem_nc(std::string);
  std::vector<double> get_gdem_profile(double lat_, double lon_,
				       std::vector<double> dep_,
				       int month);
  void get_gdem_varnames();
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
  void get_mseas_varnames_ids();
  void get_mseas_lat_lon();
  double get_mseas_bathy(double lat, double lon);
  void get_mseas_profile( double lat_, double lon_,
			  const std::vector<double>& dep_,
			  double curr_time,
			  std::vector<double>& temp,
			  std::vector<double>& salt);
  int tgrid2id_;
  int tgrid3id_;
  std::vector<std::vector<double> > tgrid_lat;
  std::vector<std::vector<double> > tgrid_lon;
  double moos_start_time;
  
  // HYCOM specific functions and variables
  void initialize_hycom_nc(std::string nc_file,
			   std::string nc_type,
			   double lat_origin, double lon_origin,
			   double moos_start, bool fixed_start);
  bool open_hycom_nc(std::string);
  void get_hycom_varnames();
  void get_hycom_varids_att();
  void get_hycom_profile(double lat_, double lon_,
			 std::vector<double>& dep_,
			 double utc_time,
			 std::vector<double>& temp,
			 std::vector<double>& salt);

  float temp_offset;
  float temp_scaling;
  float temp_missing;
  float salt_offset;
  float salt_scaling;
  float salt_missing;

  //EOF specific functions and variables
  void initialize_eof_nc(std::string nc_file);
  bool open_eof_nc(std::string);
  void get_eof_varnames();
  void get_eof_dimension();
  void get_eof_depth_dimension();
  void get_eof_depths();
  void get_eof_weights();
  void get_eof_basevals();
  void get_num_eofs();
  void get_num_depth();
  void get_eofs();
  std::vector<double> get_eof_profile(double lat_, double lon_,
                                       std::vector<double> dep);
  bool handle_eof_weight_update_message(int index, double weight_update);
  std::vector<double> current_eof_weights();
  void internal_wave_eof_weight(int index,
				double x, double y,
				double t,
				double period,
				double wavelength,
				double heading,
				double amplitude_factor);
  
  std::string eof_num_depth;
  std::string eof_num_eofs;
  std::string eof_baseval;
  std::string eof_eofs;
  std::string eof_weights;
  std::string eof_depths;

  std::string eof_dim_eofs_ncname;
  std::string eof_dim_depth_ncname;
  int eof_weights_varid_;
  int eof_baseval_varid_;
  int eof_num_eofs_varid_;
  int eof_num_depth_varid_;
  int eof_eofs_varid_;
  std::vector<double> eof_baseval_values;
  double eof_num_eofs_value;
  double eof_num_depth_value;
  std::vector<std::vector<double> > eof_eofs_values;
  std::vector<std::vector<double> > baseval_weighted_eofs;

 private:
  std::vector<std::string> files;
  bool isLonNeg = false;

};

#endif

