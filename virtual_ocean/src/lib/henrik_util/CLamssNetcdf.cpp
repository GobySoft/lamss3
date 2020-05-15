// Copyright 2018 Henrik Schmidt
//                Laboratory for Autonoimous marine Sensing Systems
//                Massachusetts Institute of Technology
//
// Created: June 2018
// Opens and reads selected values of temperature, salinity or sound speed from
// NetCdf files extracted from the GDEM database.

#include "CLamssNetcdf.h"

bool LamssNetCdf::open_nc_file(std::string nc_file_)
{
  //load files here!...after reading moos block
  int status;  // error status 

  status = nc_open (nc_file_.c_str(), 0, &ncid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - nc_file did not open!"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  else 
    {
      nc_is_open_ = true;
      std::cout<<"netcdf file < "<<nc_file_<<" > opened successfully!"<<std::endl;
    }
  return(true);
}

void LamssNetCdf::initialize_gdem_nc(std::string nc_file,
				     double lat_origin,
				     double lon_origin,
				     double moos_start,
				     bool fixed_start)
{
  nc_is_open_  = false;
  if (open_gdem_nc(nc_file))
    {
      scewed_grid = false;
      geodesy_.Initialise(lat_origin,lon_origin);

      // conversion of nc times to utc
      double utc_start;
      double utc_incr;
      double nc_unit;
      if (datenum_start >= 0)
	{
	  utc_start =  (datenum_start-UTC_ORIGIN)*24.0*3600.0 + time_vec_[0]*nc_time_unit;
	  nc_unit = nc_time_unit;
	}
      else
	{
	  utc_start =  (GDEM_TIME_ORIGIN-UTC_ORIGIN)*24.0*3600.0 + time_vec_[0]*
	    GDEM_TIME_UNIT;
	  nc_unit = GDEM_TIME_UNIT;
	}

      if (time_vec_.size() > 1)
	utc_incr = ((time_vec_.back() - time_vec_[0])/(time_vec_.size()-1))*nc_unit;
      else
	utc_incr = nc_unit;
      
      init_timebase(moos_start,utc_start,utc_incr,fixed_start);
      
    }
}

void LamssNetCdf::initialize_noaa_nc(std::string nc_file,
				     double lat_origin,
				     double lon_origin,
				     double moos_start,
				     bool fixed_start)
{
  nc_is_open_  = false;
  if (open_noaa_nc(nc_file))
    {
      scewed_grid = false;
      geodesy_.Initialise(lat_origin,lon_origin);
    }
}

void LamssNetCdf::initialize_mseas_nc(std::string nc_file,
				      double lat_origin,
				      double lon_origin,
				      double moos_start,
				      bool fixed_start)
{
  nc_is_open_  = false;
  if (open_mseas_nc(nc_file))
    {
      scewed_grid = true;
      geodesy_.Initialise(lat_origin,lon_origin);

      // conversion of nc times to utc
      double utc_start;
      double utc_incr;
      double nc_unit;
      if (datenum_start >= 0)
	{
	  utc_start =  (datenum_start-UTC_ORIGIN)*24.0*3600.0 + time_vec_[0]*nc_time_unit;
	  nc_unit = nc_time_unit;
	}
      else
	{
	  utc_start =  (MSEAS_TIME_ORIGIN-UTC_ORIGIN)*24.0*3600.0 + time_vec_[0]*
	    MSEAS_TIME_UNIT;
	  nc_unit = MSEAS_TIME_UNIT;
	}

      if (time_vec_.size() > 1)
	utc_incr = ((time_vec_.back() - time_vec_[0])/(time_vec_.size()-1))*nc_unit;
      else
	utc_incr = nc_unit;
      
      init_timebase(moos_start,utc_start,utc_incr,fixed_start);
     }
}

void LamssNetCdf::initialize_hycom_nc(std::string nc_file,
				      double lat_origin,
				      double lon_origin,
				      double moos_start,
				      bool fixed_start)
{
  nc_is_open_  = false;
  
  if (open_hycom_nc(nc_file))
    {
      scewed_grid - false;
      geodesy_.Initialise(lat_origin,lon_origin);

      // conversion of nc times to utc
      double utc_start;
      double utc_incr;
      double nc_unit;
      if (datenum_start >= 0)
	{
	  utc_start =  (datenum_start-UTC_ORIGIN)*24.0*3600.0 + time_vec_[0]*nc_time_unit;
	  nc_unit = nc_time_unit;
	}
      else
	{
	  utc_start =  (HYCOM_TIME_ORIGIN-UTC_ORIGIN)*24.0*3600.0 + time_vec_[0]*
	    HYCOM_TIME_UNIT;
	  nc_unit = HYCOM_TIME_UNIT;
	}

      if (time_vec_.size() > 1)
	utc_incr = ((time_vec_.back() - time_vec_[0])/(time_vec_.size()-1))*nc_unit;
      else
	utc_incr = nc_unit;
      
      init_timebase(moos_start,utc_start,utc_incr,fixed_start);
    }
}

void LamssNetCdf::get_lat_lon_dimensions()
{
  // >>>>>>>>>>>>>>>>>
  //get id's and lengths of lat/lon/depth/time dimensions

  // >>>>>>>>>> Longitude
  
  status = nc_inq_dimid(ncid_, lon_ncname.c_str(), &lonid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get dimension id for lon."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "lonid_=" << lonid_ << std::endl;

  status = nc_inq_dimlen(ncid_, lonid_, &lon_len); /* get lon length */
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get lon_length."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "lon_len=" << lon_len << std::endl;

  // >>>>>>>>>> Latitude	

  status = nc_inq_dimid(ncid_, lat_ncname.c_str(), &latid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get dimension id for lat."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "latid_=" << latid_ << std::endl;

  status = nc_inq_dimlen(ncid_, latid_, &lat_len); /* get lat length */
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get lat_length."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "lat_len=" << lat_len << std::endl;
}

void LamssNetCdf::get_depth_dimension()
{

  // >>>>>>>>>> Depth	

  status = nc_inq_dimid(ncid_, dep_ncname.c_str(), &depid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get dimension id for dep."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "depid_=" << depid_ << std::endl;

  status = nc_inq_dimlen (ncid_, depid_, &dep_len);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get dep_length."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }

  std::cout << "dep_len=" << dep_len << std::endl;

}

void LamssNetCdf::get_time_dimension()
{
  // >>>>>>>>>> Time	

  status = nc_inq_dimid(ncid_, time_ncname.c_str(), &timeid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get dimension id for time."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "timeid_=" << timeid_ << std::endl;

  status = nc_inq_dimlen (ncid_, timeid_, &time_len);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get time_len."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }

  std::cout << "time_len=" << time_len << std::endl;

}

void LamssNetCdf::get_gdem_varnames()
{
  // >>>>>>>>>> Longitude	

  char ch[NC_MAX_NAME+1] ;
  status = nc_inq_varname(ncid_, 0 , ch);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable name for lon."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  lon_ncname = ch;
  std::cout << "lon_ncname_=" << lon_ncname << std::endl;


  // >>>>>>>>>>>> Latitude

  status = nc_inq_varname(ncid_, 1 , ch);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable name for lat."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  lat_ncname = ch;
  std::cout << "lat_ncname_=" << lat_ncname << std::endl;

  
  // >>>>>>>>>> Depth	

  status = nc_inq_varname(ncid_, 2 , ch);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable name for depth."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  dep_ncname = ch;
  std::cout << "dep_ncname=" << dep_ncname << std::endl;

  // >>>>>>>>>> month	

  status = nc_inq_varname(ncid_, 4 , ch);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable name for varid=4."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  time_ncname = ch;
  std::cout << "time_ncname=" << time_ncname << std::endl;

  // >>>>>>>>>> Sound Speed, temperature or salinity	

  status = nc_inq_varname(ncid_, 6 , ch);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable name for varid=6."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  tsc_ncname = ch;
  std::cout << "tsc_ncname=" << tsc_ncname << std::endl;

  status = nc_inq_varid(ncid_, tsc_ncname.c_str(), &ssp_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for "
			      << tsc_ncname <<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "ssp_varid_=" << ssp_varid_ << std::endl;

}

void LamssNetCdf::get_hycom_varnames()
{
  // >>>>>>>>>> HYCOM variable names	

  char ch[NC_MAX_NAME+1] ;

  // >>>>>>>>>> Varid 5 is temperature or u-vel	 - NOT ALWAYS
  //    status = nc_inq_varname(ncid_, 5 , ch);

  temp_ncname = "water_temp";
  status = nc_inq_varid(ncid_, temp_ncname.c_str(), &temp_varid_);
  if (status != NC_NOERR)
    {
      temp_ncname = "water_u";
    }

  salt_ncname = "salinity";
  status = nc_inq_varid(ncid_, salt_ncname.c_str(), &salt_varid_);
  if (status != NC_NOERR)
    {
      salt_ncname = "water_v";
    }
}

void LamssNetCdf::get_lat_lon()
{
  status = nc_inq_varid(ncid_, lon_ncname.c_str(), &lon_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for lon."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "lon_varid_=" << lon_varid_ << std::endl;

  status = nc_inq_varid(ncid_, lat_ncname.c_str(), &lat_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for lat."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "lat_varid_=" << lat_varid_ << std::endl;

// read lat, lon and dep vectors
  lat_vec_.resize(lat_len);
  status = nc_get_var_double (ncid_, lat_varid_, &lat_vec_[0]);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get latitude vector"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "latitudes: " << lat_vec_[0] << " , " << lat_vec_[lat_vec_.size()-1]
       << std::endl;

  lon_vec_.resize(lon_len);
  status = nc_get_var_double (ncid_, lon_varid_, &lon_vec_[0]);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get longitude vector"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "longitudes: " << lon_vec_[0] << " , " << lon_vec_[lon_vec_.size()-1]
       << std::endl;

}

void LamssNetCdf::get_depths()
{
  status = nc_inq_varid(ncid_, dep_ncname.c_str(), &dep_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for depth"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "dep_varid_=" << dep_varid_ << std::endl;

  dep_vec_.resize(dep_len);
  status = nc_get_var_double (ncid_, dep_varid_, &dep_vec_[0]);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get depth vector"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "depths: " << dep_vec_[0] << " , " << dep_vec_[dep_vec_.size()-1]
       << std::endl;
}

void LamssNetCdf::get_times()
{
  status = nc_inq_varid(ncid_, time_ncname.c_str(), &time_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for month"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "time_varid_=" << time_varid_ << std::endl;

  time_vec_.resize(time_len);
  status = nc_get_var_float (ncid_, time_varid_, &time_vec_[0]);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get time vector"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "times: " << time_vec_[0] << " , "
			  << time_vec_[time_vec_.size()-1]
			  << std::endl;
  // Gewt time units
  size_t t_len;
  size_t vr_len;
  //status = nc_inq_attlen (ncid_, time_varid_, "valid_range", &vr_len);
  //if (status != NC_NOERR)       std::cout<<nc_strerror(status)<<std::endl;
  status = nc_inq_attlen (ncid_, time_varid_, "units", &t_len);
  if (status != NC_NOERR)       std::cout<<nc_strerror(status)<<std::endl;

  char * ch ;
  ch = (char *) malloc(t_len + 1);

  status = nc_get_att_text(ncid_, time_varid_, "units" , ch);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get time units."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  
  ch[t_len] = '\0';
  time_units_ = ch;
  free(ch);
  
  std::cout << "time units: " << time_units_ << std::endl;
  date_vec_.clear();
  // extract time origin

  if (extract_date(time_units_,date_vec_, nc_time_unit))
    {
      datenum_start = datenum(date_vec_);
      std::cout << "datenum: " << datenum_start 
		<< ", nc_time unit: " << nc_time_unit << std::endl;
    }
  else
    datenum_start = -1000; // Invalid model start
}

bool LamssNetCdf::open_gdem_nc(std::string nc_file_)
{

  if(nc_is_open_) return(true);

  open_nc_file(nc_file_);

  // GDEM names for lat/lion are not fixed
  get_gdem_varnames();
  
  // Get dimensions of lat, lon, depth and time
  get_lat_lon_dimensions();
  get_depth_dimension();
  get_time_dimension();
  
  get_lat_lon();
  get_depths();    
  get_times();

  return(true);
}
  
bool LamssNetCdf::open_mseas_nc(std::string nc_file_)
{

  // First open NetCdf file
  
  if(nc_is_open_) return(true);

  open_nc_file(nc_file_);

  lon_ncname = "tlon";
  lat_ncname = "tlat";
  dep_ncname = "level";
  time_ncname = "time";

  get_mseas_varnames_ids();
  
  get_lat_lon_dimensions();
  get_depth_dimension();
  get_time_dimension();

  // Lat,lon and depth on scewed grid defined by tgrid2,3
  // Read by get_mseas_profile
  get_mseas_lat_lon();
  get_times();
  
  return true;
}

void LamssNetCdf::get_mseas_varnames_ids()
{
  // Variable IDs
  // >>>>>>>>>> Bathymetry
  status = nc_inq_varid (ncid_, "tbath", &bathyid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for tbath."<< std::endl;
      std::cout<<nc_strerror(status)<< std::endl;
      exit(EXIT_FAILURE);
    }
    std::cout << "bathyid_=" << bathyid_ << std::endl;

  // >>>>>>>>>> Lat/Long Grid 
  status = nc_inq_varid (ncid_, "tgrid2", &tgrid2id_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for tgrid2."<< std::endl;
      std::cout<<nc_strerror(status)<< std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "tgrid2id_=" << tgrid2id_ << std::endl;

  // >>>>>>>>>> Lat/Long/depth Grid 
  status = nc_inq_varid (ncid_, "tgrid3", &tgrid3id_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for tgrid3."<< std::endl;
      std::cout<<nc_strerror(status)<< std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "tgrid3id_=" << tgrid3id_ << std::endl;

  // >>>>>>>>>> read time vector time	

  status = nc_inq_varid(ncid_, "time", &time_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for time"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "time_varid_=" << time_varid_ << std::endl;

  // >>>>>>>>>> temperature 	

  status = nc_inq_varid(ncid_, "temp", &temp_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for temperature"
	       <<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "temp_varid_=" << temp_varid_ << std::endl;

  // >>>>>>>>>> salinity 	

  status = nc_inq_varid(ncid_, "salt", &salt_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for salinity"
	       <<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "salt_varid_=" << salt_varid_ << std::endl;
}

bool LamssNetCdf::open_noaa_nc(std::string nc_file_)
{

  // First open NetCdf file
  
  if(nc_is_open_) return(true);
  
  std::cout << "Opening NOAA bathy nc file: " << nc_file_ << std::endl;
  open_nc_file(nc_file_);

  lon_ncname = "lon";
  lat_ncname = "lat";

  get_lat_lon_dimensions();

  get_lat_lon();
  
  status = nc_inq_varid (ncid_, "Band1", &bathy_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for tbath."<< std::endl;
      std::cout<<nc_strerror(status)<< std::endl;
      exit(EXIT_FAILURE);
    }

  std::cout << "bathy_varid_=" << bathy_varid_ << std::endl;

  return true;
}

bool LamssNetCdf::open_hycom_nc(std::string nc_file_)
{

  if(nc_is_open_) return(true);
  std::cout << "Opening hycom nc file " << nc_file_ << std::endl;
  open_nc_file(nc_file_);

  get_hycom_varnames();
  
  lon_ncname = "lon";
  lat_ncname = "lat";
  dep_ncname = "depth";
  time_ncname = "time";

  // Get dimensions
  get_lat_lon_dimensions();
  get_depth_dimension();
  get_time_dimension();
  // Get variable IDs and attributes
  get_hycom_varids_att();
  // get nc coordinates  
  get_lat_lon();
  get_depths();    
  get_times();

  return(true);
}
  
void LamssNetCdf::get_hycom_varids_att()
{
  // >>>>>>>>>> temperature 	

  status = nc_inq_varid(ncid_, temp_ncname.c_str(), &temp_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for " << temp_ncname
	       <<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << temp_ncname << "_varid_=" << temp_varid_ << std::endl;

  // Temperature or U velocity Attributes

  status = nc_get_att_float(ncid_,temp_varid_, "scale_factor", &temp_scaling);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get scaling attribute for " << temp_ncname
	       <<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << temp_ncname << " scaling: " << temp_scaling << std::endl;
  
  
  status = nc_get_att_float(ncid_,temp_varid_, "add_offset", &temp_offset);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get offset attribute for " << temp_ncname
	       <<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << temp_ncname << " offset: " << temp_offset << std::endl;
  
  status = nc_get_att_float(ncid_,temp_varid_, "missing_value", &temp_missing);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get missing value attribute for " << temp_ncname
	       <<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << temp_ncname << " missing: " << temp_missing << std::endl;
  
  // >>>>>>>>>> salinityor or V velocity 	

  status = nc_inq_varid(ncid_, salt_ncname.c_str(), &salt_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for " << salt_ncname
	       <<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << salt_ncname << "_varid_=" << salt_varid_ << std::endl;

  // Salinity Attributes

  status = nc_get_att_float(ncid_,salt_varid_, "scale_factor", &salt_scaling);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get scaling attribute for " << salt_ncname
	       <<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << salt_ncname << " scaling: " << salt_scaling << std::endl;
  
  status = nc_get_att_float(ncid_,salt_varid_, "add_offset", &salt_offset);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get offset attribute for " << salt_ncname
	       <<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << salt_ncname << " offset: " << salt_offset << std::endl;
  
  status = nc_get_att_float(ncid_,salt_varid_, "missing_value", &salt_missing);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get missing value attribute for " << salt_ncname
	       <<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << salt_ncname << " missing: " << salt_missing << std::endl;
  
}

void LamssNetCdf::init_timebase(double m_start_time,
				double utc_start,
				double utc_incr,
				bool fixed_start)
{
  fixed_model_start = fixed_start;
  moos_start_time = m_start_time;
  // For fixed_model_start, nc file start reset to mission start
  if (fixed_model_start)
    nc_utc_start = moos_start_time;
  else
    nc_utc_start = utc_start;
  nc_utc_delta = utc_incr;
  nc_utc_duration = utc_incr * time_vec_.size();
  utc_vec_.resize(time_vec_.size());

  for (int i=0, n=time_vec_.size(); i<n ; i++)
    utc_vec_[i] = nc_utc_start +(i-1)*nc_utc_delta;
      
  std::cout << "init_timebase: " << std::endl;
  std::cout << "moos_start_time: " << moos_start_time << ", "
	    << "nc_utc_start: " << nc_utc_start << ", "
	    << "nc_utc_delta: " << nc_utc_delta << ", "
	    << "nc_utc_duration: " << nc_utc_duration << ", "
	    << std::endl;
  
}

std::vector<double> LamssNetCdf:: get_gdem_profile( double lat_, double lon_,
					      std::vector<double> dep_,
					      int month)
{
  int status;

  double lat_m;
  double lon_m;
  std::vector<int> min_ilat,min_ilon;
  std::vector<double> min_lat_val;
  std::vector<double> min_lon_val;
  std::vector<double> x,y,z;

  bool debug = false;
  
  z.resize(3);
  std::vector<double> svp_out;
  std::vector<double> svp;
  svp.resize(dep_vec_.size());

  double default_ssp = std::numeric_limits<double>::quiet_NaN() ; // NaN
  for (int i=0; i<svp.size(); i++)
    svp[i]=default_ssp;

  //convert lat/lon position to local UTM


  if(!geodesy_.LatLong2LocalUTM(lat_,lon_,lat_m,lon_m)) 
  {
    std::cout << "ERROR: Failed to calculate UTM position. " << std::endl;
    exit(EXIT_FAILURE);
  }

  //get sound speed of closest 3 grid points
  // Note that here x is latitude and y longitude!

  // is closest point to auv overa grid size away?
  // if not, get me the profile
  
  if (!closest_lat_lon(lat_,lon_,
		       min_lat_val,min_lon_val,x,y,min_ilat,min_ilon))
    {
      svp_out.resize(dep_.size());
      for (int i=0; i<svp_out.size(); i++)
	svp_out[i]=default_ssp;
      return(svp_out);      
    }
  else
    {
      double min_depth = vecUtils.vmin(dep_);
      double max_depth = vecUtils.vmax(dep_);
      
      int dep_indx_1 = dep_vec_.size()-1;
      for  (int i=0; i<dep_vec_.size(); i++)
	if (dep_vec_[i] <= min_depth)
	  dep_indx_1 = i;
      int dep_indx_2 = dep_indx_1;
      for  (int i=dep_vec_.size()-1; i>=dep_indx_1; i--)
	if (dep_vec_[i] >= max_depth)
	  dep_indx_2 = i;
      
      svp_out.resize(dep_.size());
      
      
      //get lat/lon/bathy of closest grid points
      for (int i_dep=dep_indx_1; i_dep<=dep_indx_2; i_dep++)
	{
	  for(int n=0;n<3;n++)
	    {
	      if (month > 0 )       // Selected month
		{
		  int m_indx = month-1;
		  m_indx = std::max(0,std::min(11,m_indx));
		  size_t ssp_index[] = {static_cast<size_t>(m_indx),static_cast<size_t>(i_dep),static_cast<size_t>(min_ilat[n]),static_cast<size_t>(min_ilon[n])};
		  float val;
		  status = nc_get_var1_float(ncid_, ssp_varid_, ssp_index, &val);
		  if (status != NC_NOERR)
		    {
		      std::cout <<"ERROR - failed to get ssp value for "
				<< std::setprecision(std::numeric_limits<double>::digits10)
				<< min_lat_val[n]<<"N x "<< min_lon_val[n]<<"E x "
				<< dep_vec_[i_dep] << " m."<< std::endl;
		      std::cout << nc_strerror(status)<<std::endl;
		      z[n] = default_ssp;
		    }
		  else 
		    {
		      z[n] = val;
		    }
		}
	      else                  // Interpolate for current utc
		{
		  // time indexing. For GDEM current month
		  double curr_time = moos_start_time;
		  double mission_time = curr_time - moos_start_time;
		  double utc_ref;
		  if (fixed_model_start)        // Mission start at file start
		    {
		      utc_ref = moos_start_time;
		    }
		  else                          // Absolute time with rewind
		    {
		      utc_ref = nc_utc_start ;
		    }
		  double utc_rel = curr_time - utc_ref;
		  double utc_corr = floor(utc_rel/nc_utc_duration)*
		    nc_utc_duration;
		  double utc_rdcd = utc_rel - utc_corr;
		  
		  size_t indx_1 = (int) floor(utc_rdcd/nc_utc_delta);
		  size_t indx_2 = indx_1+1;
		  if (indx_2 >= utc_vec_.size())
		    indx_2 = 0;
		  if (debug)
		    std::cout << "indx_1,2 = " << indx_1 << ", " << indx_2 <<std::endl;
		  size_t ssp_index_1[] = {static_cast<size_t>(indx_1),static_cast<size_t>(i_dep),static_cast<size_t>(min_ilat[n]),static_cast<size_t>(min_ilon[n])};
		  float val_1;
		  int status_1 = nc_get_var1_float(ncid_, ssp_varid_, ssp_index_1, &val_1);
		  double dval_1 = double(val_1);
		  size_t ssp_index_2[] = {static_cast<size_t>(indx_2),static_cast<size_t>(i_dep),static_cast<size_t>(min_ilat[n]),static_cast<size_t>(min_ilon[n])};
		  float val_2;
		  int status_2 = nc_get_var1_float(ncid_, ssp_varid_, ssp_index_2, &val_2);
		  double dval_2 = double(val_2);
		  if (status_1  && status_2 != NC_NOERR)
		    {
		      std::cout <<"ERROR - failed to get ssp value for "
				<< std::setprecision(std::numeric_limits<double>::digits10)
				<< min_lat_val[n]<<"N x "<< min_lon_val[n]<<"E x "
				<< dep_vec_[i_dep] << " m."<< std::endl;
		      std::cout << nc_strerror(status)<<std::endl;
		      z[n] = default_ssp;
		    }
		  else 
		    {
		      z[n] = dval_1 + (utc_rdcd+utc_ref-utc_vec_[indx_1])*(dval_2-dval_1)/nc_utc_delta;
		    }
		}
	      
	      if (debug)
		{
		  std::cout << "n,lat,lon,val=" << n << ", "
			    << min_lat_val[n]<<"N x "<< min_lon_val[n]<<"E x "
			    << dep_vec_[i_dep] << " m. "
			    << z[n] << std::endl;
		}
	    }
	  // check for invalid numbers
	  for (int i=0; i<3; i++)
	    if (z[i]<0)
	      if (i==0)
		z[i] = default_ssp;
	      else
		z[i] = z[i-1];
	  
	  //get lat/lon/bathy of closest grid point
	  svp[i_dep]  = interp_triangle(lat_m,lon_m,x,y,z);
	}
      // Interpolate in depth
      if (dep_indx_1 >= 0 && dep_indx_1 < dep_vec_.size())
	if (dep_indx_1 == dep_indx_2)
	  for (int i = 0; i< dep_.size(); i++)
	    svp_out[i] = svp[dep_indx_1];
	else
	  bool ok = vecUtils.interp(dep_vec_,svp,dep_,svp_out);
      return(svp_out);
    }
}

void LamssNetCdf:: get_hycom_profile( double lat_,
				      double lon_,
				      const std::vector<double>& dep_,
				      double curr_time,
				      std::vector<double>& temp,
				      std::vector<double>& salt
				      )
{
  int status;

  double lat_m;
  double lon_m;
  std::vector<int> min_ilat,min_ilon;
  std::vector<double> min_lat_val;
  std::vector<double> min_lon_val;
  std::vector<double> x,y,z_tmp,z_sal;

  bool debug = false;
  
  // HYCOM stores longitudes 0-360
  if (lon_ < 0.0)
    lon_ += 360.0;
  
  z_tmp.resize(3);
  z_sal.resize(3);

  std::vector<double> nc_temp;
  nc_temp.resize(dep_vec_.size());
  std::vector<double> nc_salt;
  nc_salt.resize(dep_vec_.size());
  double default_val = std::numeric_limits<double>::quiet_NaN() ; // NaN
  for (int i=0; i<dep_vec_.size(); i++)
    {
      nc_temp[i]=default_val;
      nc_salt[i]=default_val;
    }
  
  //convert lat/lon position to local UTM


  if(!geodesy_.LatLong2LocalUTM(lat_,lon_,lat_m,lon_m)) 
  {
    std::cout << "ERROR: Failed to calculate UTM position. " << std::endl;
    exit(EXIT_FAILURE);
  }

  //get temp and salt of closest 3 grid points
  // Note that here x is latitude and y longitude!

  // is closest point to auv over 5km away?
  // if not, get me that bathymetry!
    if (!closest_lat_lon(lat_,lon_,
		       min_lat_val,min_lon_val,x,y,min_ilat,min_ilon))
      {
	temp.resize(dep_.size());
	salt.resize(dep_.size());
	for (int i=0; i<temp.size(); i++)
	  {
	    temp[i]=default_val;
	    salt[i]=default_val;
	  }
      }
    else
      {
	double min_depth = vecUtils.vmin(dep_);
	double max_depth = vecUtils.vmax(dep_);
	
	int dep_indx_1 = dep_vec_.size()-1;
	for  (int i=0; i<dep_vec_.size(); i++)
	  if (dep_vec_[i] <= min_depth)
	    dep_indx_1 = i;
	int dep_indx_2 = dep_indx_1;
	for  (int i=dep_vec_.size()-1; i>=dep_indx_1; i--)
	  if (dep_vec_[i] >= max_depth)
	    dep_indx_2 = i;
	
	// time indexing

	double mission_time = curr_time - moos_start_time;
	double utc_ref;
	if (fixed_model_start)        // Mission start at file start
	  {
	    utc_ref = moos_start_time;
	  }
	else                          // Absolute time with rewind
	  {
	    utc_ref = nc_utc_start ;
	  }
	double utc_rel = curr_time - utc_ref;
	double utc_corr = floor(utc_rel/nc_utc_duration)*
	  nc_utc_duration;
	double utc_rdcd = utc_rel - utc_corr;
	
	int indx_1 = floor(utc_rdcd/nc_utc_delta);
	int indx_2 = indx_1+1;
	if (indx_2 >= utc_vec_.size())
	  indx_2 = 0;
	if (debug)
	  std::cout << "indx_1,indx_2: " << indx_1 << ", " << indx_2 << std::endl; 
	//get temp and salt at closest grid points

	for (int i_dep=dep_indx_1; i_dep<=dep_indx_2; i_dep++)
	  {
	    for(int n=0;n<3;n++)
	      {
		// temperature and salinity
		size_t ssp_index_1[] = {static_cast<size_t>(indx_1),static_cast<size_t>(i_dep),static_cast<size_t>(min_ilat[n]),static_cast<size_t>(min_ilon[n])};
		short int temp_val_1,salt_val_1;
		int status_t_1 = nc_get_var1_short(ncid_, temp_varid_,
						   ssp_index_1, &temp_val_1);
		int status_s_1 = nc_get_var1_short(ncid_, salt_varid_,
						   ssp_index_1, &salt_val_1);
		size_t ssp_index_2[] = {static_cast<size_t>(indx_2),static_cast<size_t>(i_dep),static_cast<size_t>(min_ilat[n]),static_cast<size_t>(min_ilon[n])};
		short int temp_val_2,salt_val_2;
		int status_t_2 = nc_get_var1_short(ncid_, temp_varid_,
						   ssp_index_2, &temp_val_2);
		int status_s_2 = nc_get_var1_short(ncid_, salt_varid_,
						   ssp_index_2, &salt_val_2);
		if ((status_s_1 && status_s_2 &&
		     status_t_1 && status_t_2) != NC_NOERR)
		  {
		    std::cout <<"ERROR - failed to get t and s values for "
			      << std::setprecision(std::numeric_limits<double>::digits10)
			      << min_lat_val[n]<<"N x "<< min_lon_val[n]<<"E x "
			      << dep_vec_[i_dep] << " m."<< std::endl;
		    std::cout << nc_strerror(status)<<std::endl;
		    z_tmp[n] = default_val;
		    z_sal[n] = default_val;
		  }
		else 
		  {
		    if (temp_val_1== (short)temp_missing ||
			temp_val_2== (short)temp_missing)
		      if (n==0)
			z_tmp[n] = default_val;
		      else
			z_tmp[n] = z_tmp[n-1];
		    else
		      z_tmp[n] = (temp_val_1 + (utc_rdcd+utc_ref-utc_vec_[indx_1])
				  *(temp_val_2-temp_val_1)/nc_utc_delta)*temp_scaling
			+ temp_offset;
		    
		    if (salt_val_1==(short)salt_missing ||
			salt_val_2==(short)salt_missing)
		      if (n==0)
			z_sal[n] = default_val;
		      else
			z_sal[n] = z_sal[n-1];
		    else
		      z_sal[n] = (salt_val_1 + (utc_rdcd+utc_ref-utc_vec_[indx_1])
				  *(salt_val_2-salt_val_1)/nc_utc_delta)*salt_scaling
			+ salt_offset;
		  }
	      }

	    if (debug)
	      std::cout << "done with z_tmp,z_sal" << std::endl;
	    //get lat/lon/bathy of closest grid point
	    nc_temp[i_dep]  = interp_triangle(lat_m,lon_m,x,y,z_tmp);
	    nc_salt[i_dep]  = interp_triangle(lat_m,lon_m,x,y,z_sal);
	    if (debug)
	      std::cout << "done with depth " << dep_vec_[i_dep] << std::endl;
	  }
	// Interpolate in depth
	temp.resize(dep_.size());
	salt.resize(dep_.size());
	if (dep_indx_1 == dep_indx_2)
	  for (int i = 0; i< dep_.size(); i++)
	    {
	      temp[i] = nc_temp[dep_indx_1];
	      salt[i] = nc_salt[dep_indx_1];
	    }
	else
	  {
	    bool ok = vecUtils.interp(dep_vec_,nc_temp,dep_,temp);
	    ok = vecUtils.interp(dep_vec_,nc_salt,dep_,salt);
	  }
	if (debug)
	  std::cout << "done with get_hycom_profile " << std::endl;
      }
}

double LamssNetCdf::interp_triangle(double x_here,double y_here,
				   const std::vector<double>& x,
				   const std::vector<double>& y,
				   const std::vector<double>& z)
{
      // determinant calculation to determine interpolated bathymetry between three closest points
      // define a plane Ax+By+Cz=D, say D=1
      // 
      // for a plane defined by 3 given points:
      // 	  | x-x1   y-y1   z-z1  |
      //      det | x2-x1  y2-y1  z2-z1 | = 0
      //	  | x3-x1  y3-y1  z3-z1 |
      
      double a, b, c, d, e, f, g, h, i, z_here;

      a=x_here-x[0];
      b=y_here-y[0];
      // c=z-z1; --> solve for c --> z
      d=x[1]-x[0];
      e=y[1]-y[0];
      f=z[1]-z[0];
      g=x[2]-x[0];
      h=y[2]-y[0];
      i=z[2]-z[0];
      double denom = (d*h-e*g);
      if (denom != 0) 
	c=(-a*(e*i-f*h)+b*(d*i-f*g))/denom;
      else
	c = 0;
      z_here=c+z[0];
      return(z_here);

}

void LamssNetCdf::init_geodesy(double lat_origin, double lon_origin)
{
  geodesy_.Initialise(lat_origin,lon_origin);
  geodesy_initialized = true;
}

bool LamssNetCdf:: closest_lat_lon(double lat_, double lon_, 
				   std::vector<double> & min_lat_val,
				   std::vector<double> & min_lon_val,
				   std::vector<double> & min_lat_m,
				   std::vector<double> & min_lon_m,
				   std::vector<int> & min_ilat, 
				   std::vector<int> & min_ilon)
{
  //convert lat/lon position to local UTM
  double dlat_m= std::numeric_limits<double>::max();
  double dlon_m= std::numeric_limits<double>::max();
  double lat_m, lon_m;
  //find closest 3 lat/lon grid points and their indices
  std::vector<double> mindist_sq;
  mindist_sq.resize(3);  
  min_lat_val.resize(3);
  min_lon_val.resize(3);
  min_lat_m.resize(3);
  min_lon_m.resize(3);
  min_ilat.resize(3);
  min_ilon.resize(3);

  if(!geodesy_.LatLong2LocalUTM(lat_,lon_,lat_m,lon_m)) 
  {
    std::cout << "ERROR: Failed to calculate UTM position. " << std::endl;
    exit(EXIT_FAILURE);
  }

  int lat_indx_1 = 0;
  int lat_indx_2 = lat_len-1;
  int lon_indx_1 = 0;
  int lon_indx_2 = lon_len-1;

  if (!scewed_grid)
    {
      // GDEM,HYCOM, NOAA use a regular grid W->E, S->N,
      // so we can first narrow down 4 closest grid points
      
      for(int k=0;k<lon_len-1;k++)
	if (lon_-lon_vec_[k] >=0 )  
	  lon_indx_1 = k;	  
      lon_indx_2 = lon_indx_1+1;
      
      for(int j=0;j<lat_len-1;j++)
	if (lat_-lat_vec_[j] >=0 )  
	  lat_indx_1 = j;	  
      lat_indx_2 = lat_indx_1+1;
      
    }
  
  for(int n=0;n<3;n++)
    {
      double mindist = std::numeric_limits<double>::max();
      
      for(size_t j=lat_indx_1;j<=lat_indx_2;j++)
	{
	  for(size_t k=lon_indx_1;k<=lon_indx_2;k++)
	    {
	      double dist_sq;
	      double grid_val_lon;
	      double grid_val_lat;
	      double grid_lat_m;
	      double grid_lon_m;
	      
	      if (scewed_grid)
		{ // MSEAS
		  grid_val_lat = tgrid_lat[j][k];
		  grid_val_lon = tgrid_lon[j][k];
		}
	      else
		{ // HYCOM,GDEM,NOAA
		  grid_val_lat = lat_vec_[j];
		  grid_val_lon = lon_vec_[k];
		}
	      // convert grid lat/lon to meters
              if(!geodesy_.LatLong2LocalUTM(grid_val_lat,grid_val_lon,
					    grid_lat_m,grid_lon_m))
		{
		  std::cout << "Failed to convert lat/lon to UTM"
			    << std::endl;
		  exit(EXIT_FAILURE);
		}
	      
	      // distance between auv and grid point
	      dlon_m=grid_lon_m-lon_m;
	      dlat_m=grid_lat_m-lat_m;
	      
	      dist_sq=dlat_m*dlat_m+dlon_m*dlon_m;
	      
	      
	      if ( dist_sq < mindist && (n==0 || (n>0 && dist_sq > mindist_sq[n-1] ) ) )
		{
		  mindist=dist_sq;
		  mindist_sq[n]=dist_sq;
		  min_lat_val[n]=grid_val_lat;
		  min_lon_val[n]=grid_val_lon;
		  min_ilat[n]=j;
		  min_ilon[n]=k;
		  min_lat_m[n]=grid_lat_m;
		  min_lon_m[n]=grid_lon_m;
		}
	    }
	}
    }
  if (mindist_sq[0] >= pow(std::max(fabs(min_lat_m[1]-min_lat_m[0]),
				    fabs(min_lat_m[2]-min_lat_m[0])),2) +
      pow(std::max(fabs(min_lon_m[1]-min_lon_m[0]),
		   fabs(min_lon_m[2]-min_lon_m[0])),2) )
    return(false);
  else
    return(true);
      
}

void LamssNetCdf:: get_mseas_profile( double lat_, double lon_,
				      const std::vector<double>& dep_,
				      double curr_time,
				      std::vector<double>& temp,
				      std::vector<double>& salt
				      )

{
  int status;

  double lat_m;
  double lon_m;
  std::vector<int> min_ilat,min_ilon;
  std::vector<double> min_lat_val;
  std::vector<double> min_lon_val;
  std::vector<double> x,y,z_tmp,z_sal;
  bool debug = false;
  
  z_tmp.resize(3);
  z_sal.resize(3);

  double default_val = std::numeric_limits<double>::quiet_NaN() ; // NaN

  std::vector<std::vector<double> > nc_temp;
  std::vector<std::vector<double> > nc_salt;


  // Set up buffers
  
  nc_temp.resize(3);
  nc_salt.resize(3);
 
  for (int i=0; i<3; i++)
    {
      nc_temp[i].resize(dep_.size());
      nc_salt[i].resize(dep_.size());
      for (int j=0; j<dep_.size(); j++)
	{
	  nc_temp[i][j]=default_val;
	  nc_salt[i][j]=default_val;
	}
    }  
 
  // Fill output arrays with default value
  
  temp.resize(dep_.size());
  salt.resize(dep_.size());
  for (int j=0; j<dep_.size(); j++)
    {
      temp[j]=default_val;
      salt[j]=default_val;
    }
  
      //convert lat/lon position to local UTM

  if(!geodesy_.LatLong2LocalUTM(lat_,lon_,lat_m,lon_m)) 
  {
    std::cout << "ERROR: Failed to calculate UTM position. " << std::endl;
    exit(EXIT_FAILURE);
  }

  // find closest 3 grid points and interpolate
  // Note that here x is latitude and y longitude!

  if (!closest_lat_lon(lat_,lon_,
		       min_lat_val,min_lon_val,x,y,min_ilat,min_ilon))

    {
      std::cout << "Closest grid point more than 30 km away " << std::endl;
      return;  
    }
  else
    {
      // time indexing
      double mission_time = curr_time - moos_start_time;
      double utc_ref;
      if (fixed_model_start)        // Mission start at file start
	{
	  utc_ref = moos_start_time;
	}
      else                          // Absolute time with rewind
	{
	  utc_ref = nc_utc_start ;
	}
      double utc_rel = curr_time - utc_ref;
      double utc_corr = floor(utc_rel/nc_utc_duration)*
	nc_utc_duration;
      double utc_rdcd = utc_rel - utc_corr;
      if (debug)
	std::cout << "utc_ref,utc_rel,utc_rdcd: "
		  << utc_ref << ", "
		  << utc_rel << ", "
		  << utc_rdcd << ", "
		  << std::endl;
      
      int indx_1 = floor(utc_rdcd/nc_utc_delta);
      int indx_2 = indx_1+1;
      if (indx_2 >= utc_vec_.size())
	indx_2 = 0;
      if (debug)
	std::cout << "indx_1,indx_2: " << indx_1 << ", " << indx_2 << std::endl; 
      //get depth, salinity and temerature vectors at closest grid points
      for(int n=0;n<3;n++)
	{
	  dep_vec_.resize(dep_len);
	  // Interpolate depth vector onto requested grid
	  std::vector<double> temp_buf;	  
	  std::vector<double> salt_buf;
	  temp_buf.resize(dep_len);
	  salt_buf.resize(dep_len);
	  
	  for (int i_dep=0; i_dep<dep_len; i_dep++)
	    {
	      size_t dep_ind[] = {static_cast<size_t>(min_ilat[n]),static_cast<size_t>(min_ilon[n]),static_cast<size_t>((size_t) i_dep),static_cast<size_t>(2)};
	      double dep_val;
	      int status = nc_get_var1_double(ncid_, tgrid3id_, dep_ind, &dep_val);
	      if (status != NC_NOERR)
		{
		  std::cout<<"ERROR - failed to get depth value for "
			   << lat_ <<"N x "<< lon_<<"E."<<std::endl;
		  std::cout<<nc_strerror(status)<<std::endl;
		}
	      // mseas depths are negative
	      dep_vec_[i_dep] = -dep_val;
	      
	      // temperature and salinity
	      size_t ssp_index_1[] = {static_cast<size_t>(indx_1),static_cast<size_t>(min_ilat[n]),static_cast<size_t>(min_ilon[n]),static_cast<size_t>(i_dep)};
	      double temp_val_1,salt_val_1;
	      int status_t_1 = nc_get_var1_double(ncid_, temp_varid_,
					       ssp_index_1, &temp_val_1);
	      int status_s_1 = nc_get_var1_double(ncid_, salt_varid_,
					       ssp_index_1, &salt_val_1);
	      size_t ssp_index_2[] = {static_cast<size_t>(indx_2),static_cast<size_t>(min_ilat[n]),static_cast<size_t>(min_ilon[n]),static_cast<size_t>(i_dep)};
	      double temp_val_2,salt_val_2;
	      int status_t_2 = nc_get_var1_double(ncid_, temp_varid_,
						 ssp_index_2, &temp_val_2);
	      int status_s_2 = nc_get_var1_double(ncid_, salt_varid_,
						 ssp_index_2, &salt_val_2);
	      if ((status_s_1 && status_s_2 &&
		   status_t_1 && status_t_2) != NC_NOERR)
		{
		  std::cout <<"ERROR - failed to get t and s values for "
			    << std::setprecision(std::numeric_limits<double>::digits10)
			    << min_lat_val[n]<<"N x "<< min_lon_val[n]<<"E x "
			    << dep_vec_[i_dep] << " m."<< std::endl;
		  std::cout << nc_strerror(status)<<std::endl;
		  temp_buf[i_dep] = default_val;
		  salt_buf[i_dep] = default_val;
		}
	      else 
		{
		  if (debug)
		    std::cout << "depth,tval_1,sval_1,tval_2,sval_2: "
			      << dep_vec_[i_dep] << ", "
			      << temp_val_1 << ", "
			      << salt_val_1 << ", "
			      << temp_val_2 << ", "
			      << salt_val_2 << ", "
			      << std::endl;
		      
		  temp_buf[i_dep] = (temp_val_1+
				     (utc_rdcd+utc_ref-utc_vec_[indx_1])
				     *(temp_val_2-temp_val_1)/nc_utc_delta);
		  salt_buf[i_dep] = (salt_val_1 +
				     (utc_rdcd+utc_ref-utc_vec_[indx_1])
				     *(salt_val_2-salt_val_1)/nc_utc_delta);
		}
	      if (debug)
		{
		  std::cout << "n,lat,lon,temp,salt=" << n << ", "
			    << min_lat_val[n]<<"N x "<< min_lon_val[n]<<"E x "
			    << dep_vec_[i_dep] << " m. "
			    << temp_buf[i_dep] << ", "
			    << salt_buf[i_dep] << ", "
			    << std::endl;
		}
	    } // end of depth loop.
	  // Interpolate onto requested depth grid
	  std::vector<double> buf;
	  bool ok = vecUtils.interp(dep_vec_,temp_buf,dep_,buf);
	  nc_temp[n] = buf;
	  ok = vecUtils.interp(dep_vec_,salt_buf,dep_,buf);
	  nc_salt[n] = buf;
	} // end of n loop

      for (int i_dep=0; i_dep<dep_.size();i_dep++)
	{
	  // check for invalid numbers
	  for (int i=0; i<3; i++)
	    {
	      z_tmp[i] = nc_temp[i][i_dep];
	      z_sal[i] = nc_salt[i][i_dep];
		
	      if (z_tmp[i]<0)
		if (i==0)
		  z_tmp[i] = default_val;
		else
		  z_tmp[i] = z_tmp[i-1];
	      if (z_sal[i]<0)
		if (i==0)
		  z_sal[i] = default_val;
		else
		  z_sal[i] = z_sal[i-1];
	    }
	  if (debug)
	    std::cout << "done with z_tmp,z_sal" << std::endl;
	  //get lat/lon/bathy of closest grid point
	  temp[i_dep]  = interp_triangle(lat_m,lon_m,x,y,z_tmp);
	  salt[i_dep]  = interp_triangle(lat_m,lon_m,x,y,z_sal);
	  if (debug)
	    std::cout << "done with depth " << dep_[i_dep] << std::endl;
	}
    }
  if (debug)
    std::cout << "done with get_mseas_profile " << std::endl;
}

double LamssNetCdf:: get_mseas_bathy( double lat_, double lon_)

{
  int status;

  double lat_m;
  double lon_m;
  std::vector<int> min_ilat,min_ilon;
  std::vector<double> min_lat_val;
  std::vector<double> min_lon_val;
  std::vector<double> x,y,z;
  double bathy_z;
  bool debug = false;
  
  z.resize(3);

  double default_bathy = std::numeric_limits<double>::quiet_NaN() ; // NaN

  //convert lat/lon position to local UTM


  if(!geodesy_.LatLong2LocalUTM(lat_,lon_,lat_m,lon_m)) 
  {
    std::cout << "ERROR: Failed to calculate UTM position. " << std::endl;
    exit(EXIT_FAILURE);
  }
  
  if (debug)
    std::cout << "lat,lon,lat_m,lon_m,lat_len,lon_len=" 
       << lat_ << ", "
       << lon_ << ", "
       << lat_m << ", "
       << lon_m << ", "
       << lat_len << ", "
       << lon_len << std::endl;

  //get bathymetry closest 3 grid points
  // Note that here x is latitude and y longitude!

  if ( !closest_lat_lon(lat_,lon_,
			min_lat_val,min_lon_val,x,y,min_ilat,min_ilon))
    {
      return(default_bathy);
    }
  else
    {
      for(int n=0;n<3;n++)
	{
	  //get lat/lon/bathy of closest grid point
	  size_t bathy_index[] = {static_cast<size_t>(min_ilat[n]),static_cast<size_t>(min_ilon[n])}; 
	  double val;
	  status = nc_get_var1_double(ncid_, bathyid_, bathy_index, &val);
	  if (status != NC_NOERR)
	    {
	      std::cout<<"ERROR - failed to get bathy value for "
		       << std::setprecision(std::numeric_limits<double>::digits10)
		       <<min_lat_val[n]<<"N x "
		       <<min_lon_val[n]<<"E."<<std::endl;
	      std::cout<<nc_strerror(status)<<std::endl;
	      if (n>0)
		z[n]=z[0];
	    }
	  else
	    z[n] = val;
	}
    }
  //get lat/lon/bathy of closest grid point
  bathy_z = interp_triangle(lat_m,lon_m,x,y,z);
  return(bathy_z);
}

void LamssNetCdf::get_mseas_lat_lon()
{
  int status;
  tgrid_lat.resize(lat_len);
  tgrid_lon.resize(lat_len);
  for(size_t j=0;j<lat_len;j++)
    {
      tgrid_lat[j].resize(lon_len);
      tgrid_lon[j].resize(lon_len);
      for(size_t k=0;k<lon_len;k++)
	{
	  size_t lon_ind[] = {j,k,0};
	  size_t lat_ind[] = {j,k,1};
	  double grid_val_lat;
	  status = nc_get_var1_double(ncid_, tgrid2id_, lat_ind, &grid_val_lat);
	  if (status != NC_NOERR)
	    {
	      std::cout<<"ERROR - failed to get grid(lat_index) value for "
		       << "j,k = " << j << ", " << k <<std::endl;
	      std::cout<<nc_strerror(status)<<std::endl;
	      exit(EXIT_FAILURE);
	    }
	  tgrid_lat[j][k]=grid_val_lat;
	  double grid_val_lon;	      
	  status = nc_get_var1_double(ncid_, tgrid2id_, lon_ind, &grid_val_lon);
	  if (status != NC_NOERR)
	    {
	      std::cout<<"ERROR - failed to get grid(lon_index) value for "
		       << "j,k = " << j << ", " << k <<std::endl;
	      std::cout<<nc_strerror(status)<<std::endl;
	      exit(EXIT_FAILURE);
	    }
	  tgrid_lon[j][k]=grid_val_lon;
	}
    }	      
}
	  
double LamssNetCdf::get_noaa_bathy( double lat_, double lon_)
{
  int status;

  double lat_m;
  double lon_m;
  std::vector<int> min_ilat,min_ilon;
  std::vector<double> min_lat_val;
  std::vector<double> min_lon_val;
  std::vector<double> x,y,z;
  double bathy_z;
  bool debug = false;
  
  z.resize(3);

  double default_bathy = std::numeric_limits<double>::quiet_NaN() ; // NaN

  //convert lat/lon position to local UTM

  if(!geodesy_.LatLong2LocalUTM(lat_,lon_,lat_m,lon_m)) 
  {
    std::cout << "ERROR: Failed to calculate UTM position. " << std::endl;
    exit(EXIT_FAILURE);
  }

  //get sound speed of closest 3 grid points
  // Note that here x is N and y E!
  if ( !closest_lat_lon(lat_,lon_,
			min_lat_val,min_lon_val,x,y,min_ilat,min_ilon))
    return(default_bathy);
  else
    {
      for(int n=0;n<3;n++)
	{
	  //get lat/lon/bathy of closest grid point
	  size_t bathy_index[] = {static_cast<size_t>(min_ilat[n]),static_cast<size_t>(min_ilon[n])}; 
	  short val;
	  status = nc_get_var1_short(ncid_, bathy_varid_, bathy_index, &val);
	  if (status != NC_NOERR)
	    {
	      std::cout<<"ERROR - failed to get bathy value for "
		       << std::setprecision(std::numeric_limits<double>::digits10)
		       <<min_lat_val[n]<<"N x "
		       <<min_lon_val[n]<<"E."<<std::endl;
	      std::cout<<nc_strerror(status)<<std::endl;
	      z[n] = default_bathy;
	    }
	  else
	    z[n] = val;
	}
      //get lat/lon/bathy of closest grid point
      bathy_z = interp_triangle(lat_m,lon_m,x,y,z);
      return(bathy_z);
    }
}

double LamssNetCdf::datenum(const std::vector<int>& date_vec)
{
  // Function returning equivalent of matlab datenum
  // day_vec: {yy,mm,dd,hh,mm,ss};
  // Note that datenum assumes a year 0 as a leap year.
  // base is date number since 0000-0-0 of 12/31 the year before
  // Default is matlab datenum origin: 0000-01-01 00:00:00
  int year = 0;
  int month = 1;
  int day = 1;
  double hours = 0;
  double mins = 0;
  double secs = 0;
  for (int i = 0; i < date_vec.size(), i<6; i++)
    {
      switch (i)
	{
	case 0:
	  year = date_vec[i];
	case 1:
	  month = date_vec[i];
	case 2:
	  day = date_vec[i];
	case 3:
	  hours = date_vec[i];
	case 4:
	  mins = date_vec[i];
	case 5:
	  secs = date_vec[i];
	}
    }
  int base = year*365 + (year-1)/4 - (year-1)/100 + (year-1)/400 + 1;
  int month_days = 0;
  // is this year a leap year?
  bool leap = ((year%4 == 0) && (year%100 != 0)) || (year%400) == 0;  
  for (int mm=2; mm<=month; mm++)
    {
      // add previous months
      switch (mm)
	{
	case 2:
	  month_days += 31;
	case 3:
	  month_days += 28 + (int) leap;
	case 4:
	  month_days += 31;
	case 5:
	  month_days += 30;
	case 6:
	  month_days += 31;
	case 7:
	  month_days += 30;
	case 8:
	  month_days += 31;
	case 9:
	  month_days += 31;
	case 10:
	  month_days += 30;
	case 11:
	  month_days += 31;
	case 12:
	  month_days += 30;
	}
    }
  double day_fraction = (hours + (mins/60.0 + secs)/60.0)/24.0;
  return(base + month_days + day + day_fraction);
}

bool LamssNetCdf::extract_date(const std::string& in, std::vector<int>& date,
			       double& units_secs)
{
  // First change date/time separators '-' and ':' with spaces
  std::string mod_in = in;
  size_t index = 0;
  while (index < mod_in.length())
    {
      index = mod_in.find("-",index);
      if (index == std::string::npos) break;
      mod_in.replace(index,1," ");
      index += 1;
    }
  index = 0;
  while (index < mod_in.length())
    {
      index = mod_in.find(":",index);
      if (index == std::string::npos) break;
      mod_in.replace(index,1," ");
      index += 1;
    }
  //std::cout << "Modified units: " << mod_in << std::endl;
  // Next convert to date vector yy.mm,dd,hh,mm,ss
  std::stringstream ss;
  std::string temp;
  ss << mod_in;
  int found;
  // first get unit string
  if (!ss.eof())
    ss >> temp;
  std::string secs_str = "seconds";
  std::string mins_str = "minutes";
  std::string hour_str = "hours";
  std::string days_str = "days";
  if (temp == secs_str)
    units_secs = 1;
  else if (temp == mins_str)
    units_secs = 60;
  else if (temp == hour_str)
    units_secs = 3600;
  else if (temp == days_str)
    units_secs = 24*3600;
  else
    units_secs = 1;
  
  std::cout << "Units: " << temp << " seconds " << units_secs << std::endl;
  date.clear();
  std::cout << "Date:";
  while (!ss.eof())
    {
      ss >> temp;
      // Integer?
      if (std::stringstream(temp) >> found)
	{
	  date.push_back(found);
	  std::cout << " " << found;
	}
      temp = "";
    }
  std::cout << std::endl;
  if (date.size() > 2)
    return(true);
  else
    return(false);
}

void LamssNetCdf::initialize_eof_nc(std::string nc_file)
{
  nc_is_open_  = false;
  
  if (open_eof_nc(nc_file)) {
    std::cout<<std::endl<<" in initialize eof" <<std::endl;
      // scewed_grid - false;
      // geodesy_.Initialise(lat_origin,lon_origin);

      // // conversion of nc times to utc
      // double utc_start;
      // double utc_incr;
      // double nc_unit;
      // if (datenum_start >= 0)
      //   {
      //     utc_start =  (datenum_start-UTC_ORIGIN)*24.0*3600.0 + time_vec_[0]*nc_time_unit;
      //     nc_unit = nc_time_unit;
      //   }
      // else
      //   {
      //     utc_start =  (HYCOM_TIME_ORIGIN-UTC_ORIGIN)*24.0*3600.0 + time_vec_[0]*
      //       HYCOM_TIME_UNIT;
      //     nc_unit = HYCOM_TIME_UNIT;
      //   }

      // if (time_vec_.size() > 1)
      //   utc_incr = ((time_vec_.back() - time_vec_[0])/(time_vec_.size()-1))*nc_unit;
      // else
      //   utc_incr = nc_unit;
      
      // init_timebase(moos_start,utc_start,utc_incr,fixed_start);
    }
}

bool LamssNetCdf::open_eof_nc(std::string nc_file_)
{
  
  if(nc_is_open_) return(true);
  std::cout << "Opening eof nc file " << nc_file_ << std::endl;
  open_nc_file(nc_file_);

  get_eof_varnames();

  eof_dim_eofs_ncname = "dim_eofs";
  eof_dim_depth_ncname = "dim_depth";

  // // Get dimensions
  get_eof_depth_dimension();
  get_eof_dimension();
  // Get variable IDs and attributes
  get_eof_depths();
  get_eof_weights();
  get_eof_basevals();
  get_num_eofs();
  get_num_depth();
  get_eofs();
  // // get nc coordinates  
  // get_lat_lon();

  return(true);
}

void LamssNetCdf::get_eof_varnames()
{
  // >>>>>>>>>> HYCOM variable names	

  char ch[NC_MAX_NAME+1] ;

  // >>>>>>>>>> Varid 0 is depths 	
  status = nc_inq_varname(ncid_, 0 , ch);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable name for varid 0" << std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
    }
  else
    {
      eof_depths = ch;
      std::cout << "varname 0 : " << eof_depths << std::endl;
    }

  // >>>>>>>>>> Varid 1 is weights
  status = nc_inq_varname(ncid_, 1 , ch);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable name for varid 1" << std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
    }
  else
    {
      eof_weights = ch;
      std::cout << "varname 1 : " << eof_weights << std::endl;
    }

  // >>>>>>>>>> Varid 2 is eofs
  status = nc_inq_varname(ncid_, 2 , ch);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable name for varid 2" << std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
    }
  else
    {
      eof_eofs = ch;
      std::cout << "varname 2 : " << eof_eofs << std::endl;
    }

  // >>>>>>>>>> Varid 3 is baseval 	
  status = nc_inq_varname(ncid_, 3 , ch);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable name for varid 3" << std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
    }
  else
    {
      eof_baseval = ch;
      std::cout << "varname 3 : " << eof_baseval << std::endl;
    }

  // >>>>>>>>>> Varid 4 is num_eofs
  status = nc_inq_varname(ncid_, 4 , ch);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable name for varid 4" << std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
    }
  else
    {
      eof_num_eofs = ch;
      std::cout << "varname 4 : " << eof_num_eofs << std::endl;
    }
 
  // >>>>>>>>>> Varid 5 is num_depth
  status = nc_inq_varname(ncid_, 5 , ch);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable name for varid 5" << std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
    }
  else
    {
      eof_num_depth = ch;
      std::cout << "varname 5: " << eof_num_depth << std::endl;
    }
  
 
}

void LamssNetCdf::get_eof_dimension()
{

  // >>>>>>>>>> EOF dimension

  status = nc_inq_dimid(ncid_, eof_dim_eofs_ncname.c_str(), &eofid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get dimension id for eofs."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "eofid_=" << eofid_ << std::endl;

  status = nc_inq_dimlen (ncid_, eofid_, &eof_len);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get eof_length."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }

  std::cout << "eof_len=" << eof_len<< std::endl;

}

void LamssNetCdf::get_eof_depth_dimension()
{

  // >>>>>>>>>> Depth	

  status = nc_inq_dimid(ncid_, eof_dim_depth_ncname.c_str(), &depid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get dimension id for dep."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "depid_=" << depid_ << std::endl;

  status = nc_inq_dimlen (ncid_, depid_, &dep_len);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get dep_length."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }

  std::cout << "dep_len=" << dep_len << std::endl;

}

void LamssNetCdf::get_eof_depths()
{
  status = nc_inq_varid(ncid_, eof_depths.c_str(), &dep_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for depth"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "dep_varid_=" << dep_varid_ << std::endl;

  dep_vec_.resize(dep_len);
  status = nc_get_var_double (ncid_, dep_varid_, &dep_vec_[0]);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get depth vector"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }

  std::cout << "depths: ";
  for (int ij=0;ij<dep_vec_.size();ij++) {
    std::cout << dep_vec_[ij] << ", ";
      }
  std::cout << std::endl;
}

void LamssNetCdf::get_eof_weights()
{
  status = nc_inq_varid(ncid_, eof_weights.c_str(), &eof_weights_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for weights"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "weights_varid_=" << eof_weights_varid_ << std::endl;

  weights_vec_.resize(eof_len);
  status = nc_get_var_double (ncid_, eof_weights_varid_, &weights_vec_[0]);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get weights vector"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }

  std::cout << "weights: ";
  for (int ij=0;ij<weights_vec_.size();ij++) {
    std::cout << weights_vec_[ij] << ", ";
  }
  std::cout << std::endl;
}

void LamssNetCdf::get_eof_basevals()
{
  status = nc_inq_varid(ncid_, eof_baseval.c_str(), &eof_baseval_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for baseval"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "baseval_varid_=" << eof_baseval_varid_ << std::endl;

  eof_baseval_values.resize(dep_len);

  status = nc_get_var_double (ncid_, eof_baseval_varid_, &eof_baseval_values[0]);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get baseval value"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }

  for(int k=0;k<dep_len;k++){
    std::cout << std::endl << " baseval[" << k << "]:" << eof_baseval_values[k];
  }
  std::cout<<std::endl;
}

void LamssNetCdf::get_num_eofs()
{
  status = nc_inq_varid(ncid_, eof_num_eofs.c_str(), &eof_num_eofs_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for num_eofs"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "num_eofs_varid_=" << eof_num_eofs_varid_ << std::endl;

  status = nc_get_var_double (ncid_, eof_num_eofs_varid_, &eof_num_eofs_value);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get num_eofs value"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }

  std::cout << std::endl << " num_eofs value: " << eof_num_eofs_value << std::endl;
}

void LamssNetCdf::get_num_depth()
{
  status = nc_inq_varid(ncid_, eof_num_depth.c_str(), &eof_num_depth_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for num_depth"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "num_depth_varid_=" << eof_num_depth_varid_ << std::endl;

  status = nc_get_var_double (ncid_, eof_num_depth_varid_, &eof_num_depth_value);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get num_depth value"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }

  std::cout << std::endl << " num_depth value: " << eof_num_depth_value << std::endl;
}

void LamssNetCdf::get_eofs()
{
  eof_eofs_values.resize(eof_num_eofs_value);
  for(size_t j=0;j<eof_num_eofs_value;j++){
    eof_eofs_values[j].resize(eof_num_depth_value);
  }
  

  status = nc_inq_varid(ncid_, eof_eofs.c_str(), &eof_eofs_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for eofs"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "eof_eofs_varid_=" << eof_eofs_varid_ << std::endl;

  for(size_t j=0;j < eof_num_eofs_value; j++){
    
    for(size_t k=0; k < eof_num_depth_value; k ++)
      {
        size_t eofs_index[] = {j,k};
        double grid_eof_val;
        status = nc_get_var1_double (ncid_, eof_eofs_varid_, eofs_index, &grid_eof_val);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get eof eofs values"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  eof_eofs_values[j][k] = grid_eof_val;
      }
  }


  for(size_t j=0;j<eof_num_eofs_value;j++){
    std::cout << std::endl << "eof number: " << j << std::endl;

    for(size_t k=0;k < eof_num_depth_value; k++){
      std::cout << eof_eofs_values[j][k] << " " ;
    }
  }
}

std::vector<double> LamssNetCdf::get_eof_profile(double lat_, double lon_, std::vector<double> dep_request)
{
  bool debug = false;
  // need to reinitialize this
  //get_eof_basevals();

  //resize baseval_weighted_eofs to match eof_eofs_values
  baseval_weighted_eofs.resize(eof_num_eofs_value);
  for(size_t j=0; j< eof_num_eofs_value;j++){
    baseval_weighted_eofs[j].resize(eof_num_depth_value);
  }

  //step 1: math for getting baseval_weighted_eofs
  for(size_t j = 0; j < eof_num_eofs_value; j++){
    for(size_t k = 0; k < eof_num_depth_value; k++){
      baseval_weighted_eofs[j][k] = eof_eofs_values[j][k] * weights_vec_[j]; 
    }
  }

  std::vector<double> svp_out;
  std::vector<double> svp;
  svp.resize(eof_num_depth_value);
  svp_out.resize(dep_request.size());

 //step 2: vector double k = summation of [:] [k]
  for(size_t k=0; k<eof_num_depth_value;k++){
    svp[k] = 0.0;
    for(size_t j=0; j< eof_num_eofs_value; j++){
      svp[k] += baseval_weighted_eofs[j][k];
    }
    svp[k] += eof_baseval_values[k];
  }
  //print svp for chcking
  if (debug)
    {
      std::cout<< std::endl;
      for(size_t k=0;k<eof_num_depth_value;k++)
	{
	  std::cout<< "dep_vec_[" <<k <<"] = " <<dep_vec_[k] <<" svp[" << k <<"]= " << svp[k] << std::endl;
	}
      std::cout<< std::endl;
    }
  //step 3: Interpolate by the  depth request
  double min_depth = vecUtils.vmin(dep_request);
  double max_depth = vecUtils.vmax(dep_request);

  if (debug)
    {
      std::cout << std::endl << "Interpolation stage: " << std::endl << std::endl;
      std::cout << "Depth request boundaries:  [" << min_depth << " , "
		<< max_depth << "]" << std::endl;
      std::cout << "Known profile boundaries:  [" << vecUtils.vmin(dep_vec_)
		<< " , " << vecUtils.vmax(dep_vec_) << "]" << std::endl;
    }
  // int dep_indx_1 = dep_vec_.size()-1;
  int dep_indx_1 = 0;
  for  (int i=0; i<dep_vec_.size(); i++)
    if (dep_vec_[i] <= min_depth)
      dep_indx_1 = i;

  // int dep_indx_2 = dep_indx_1;
  int dep_indx_2 = dep_vec_.size()-1;
  for  (int i=dep_vec_.size()-1; i>=dep_indx_1; i--)
    if (dep_vec_[i] >= max_depth)
      dep_indx_2 = i;
  if (debug)
    {
      std::cout << "Indices: " << dep_vec_.size()-1 << " =-> [" << dep_indx_1 << " , " << dep_indx_2 << "]" << std::endl;
    }

  if (dep_indx_1 >= 0 && dep_indx_1 < dep_vec_.size())
    if (dep_indx_1 == dep_indx_2)
      for (int i = 0; i< dep_request.size(); i++)
	svp_out[i] = svp[dep_indx_1];
    else
      bool ok = vecUtils.interp(dep_vec_,svp,dep_request,svp_out);
  if (debug)
    {
      std::cout<< std::endl;
      for(int i = 0, n=svp_out.size(); i<n; ++i)
	std::cout << i << " dep_request: " << dep_request[i]
		  << " svp_out: " << svp_out[i] << std::endl;
      std::cout<< std::endl;
    }
  return(svp_out);
}

bool LamssNetCdf::handle_eof_weight_update_message(int index,double weight_update)
{
  //let's do some error checking such as index makes sense?
  //must be between 0 and (size-1)
  if(weights_vec_.empty() || index < 0 || index >= weights_vec_.size())
    {
      return false;
    }
  else
    {
      weights_vec_[index] = weight_update;
    }

  return true;
}
