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
  if(nc_is_open_) return(true);
  //load files here!...after reading moos block
  int status;  /* error status */

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
				     double moos_start)
{
  if (open_gdem_nc(nc_file))
    {
      geodesy_.Initialise(lat_origin,lon_origin);
      geodesy_initialized = true;
      init_timebase(moos_start);
    }
}

void LamssNetCdf::initialize_noaa_nc(std::string nc_file,
				     double lat_origin,
				     double lon_origin,
				      double moos_start)
{
  if (open_noaa_nc(nc_file))
    {
      geodesy_.Initialise(lat_origin,lon_origin);
      geodesy_initialized = true;
      init_timebase(moos_start);
    }
}

void LamssNetCdf::initialize_mseas_nc(std::string nc_file,
				      double lat_origin,
				      double lon_origin,
				      double moos_start)
{
  if (open_mseas_nc(nc_file))
    {
      geodesy_.Initialise(lat_origin,lon_origin);
      geodesy_initialized = true;
      init_timebase(moos_start);
    }
}

bool LamssNetCdf::open_gdem_nc(std::string nc_file_)
{

  if (open_nc_file(nc_file_))
    return(true);

  int status;  /* error status */

  // >>>>>>>>>> Longitude	

  std::string lon_ncname;
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


  status = nc_inq_varid(ncid_, lon_ncname.c_str(), &lon_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for lon."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "lon_varid_=" << lon_varid_ << std::endl;

  // >>>>>>>>>>>> Latitude

  std::string lat_ncname;
  status = nc_inq_varname(ncid_, 1 , ch);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable name for lat."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  lat_ncname = ch;
  std::cout << "lat_ncname_=" << lat_ncname << std::endl;

  status = nc_inq_varid(ncid_, lat_ncname.c_str(), &lat_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for lat."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "lat_varid_=" << lat_varid_ << std::endl;

  
  // >>>>>>>>>> Depth	

  std::string dep_ncname;
  status = nc_inq_varname(ncid_, 2 , ch);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable name for depth."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  dep_ncname = ch;
  std::cout << "dep_ncname=" << dep_ncname << std::endl;

  status = nc_inq_varid(ncid_, dep_ncname.c_str(), &dep_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for depth"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "dep_varid_=" << dep_varid_ << std::endl;

  // >>>>>>>>>> month	

  std::string time_ncname;
  status = nc_inq_varname(ncid_, 4 , ch);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable name for varid=4."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  time_ncname = ch;
  std::cout << "time_ncname=" << time_ncname << std::endl;

  status = nc_inq_varid(ncid_, time_ncname.c_str(), &time_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for month"<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "time_varid_=" << time_varid_ << std::endl;

  // >>>>>>>>>> Sound Speed, temperature or salinity	

  std::string tsc_ncname;
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

  // >>>>>>>>>>>>>>>>>
  //get length of lat/lon dimensions
  size_t lat_len;
  size_t lon_len;
  size_t dep_len;
  size_t time_len;

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

  return(true);
}
  
bool LamssNetCdf::open_mseas_nc(std::string nc_file_)
{

  // First open NetCdf file
  
  if (open_nc_file(nc_file_))
    return(true);

  int status;  /* error status */

  // >>>>>>>>>> Latitude	

  status = nc_inq_dimid(ncid_, "tlat", &latid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get dimension id for tlat."<< std::endl;
      std::cout<<nc_strerror(status)<< std::endl;
      exit(EXIT_FAILURE);
    }
	
  // >>>>>>>>>> Longitude	

  status = nc_inq_dimid(ncid_, "tlon", &lonid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get dimension id for tlon."<< std::endl;
      std::cout<<nc_strerror(status)<< std::endl;
      exit(EXIT_FAILURE);
    }

  // >>>>>>>>>> Depth/Level	

  status = nc_inq_dimid(ncid_, "level", &depid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get dimension id for level(depth)."
	       << std::endl;
      std::cout<<nc_strerror(status)<< std::endl;
      exit(EXIT_FAILURE);
    }

  // >>>>>>>>>> Bathymetry
  status = nc_inq_varid (ncid_, "tbath", &bathyid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for tbath."<< std::endl;
      std::cout<<nc_strerror(status)<< std::endl;
      exit(EXIT_FAILURE);
    }
  // >>>>>>>>>> Lat/Long Grid 
  status = nc_inq_varid (ncid_, "tgrid2", &tgrid2id_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for tgrid2."<< std::endl;
      std::cout<<nc_strerror(status)<< std::endl;
      exit(EXIT_FAILURE);
    }

  // >>>>>>>>>> Lat/Long/depth Grid 
  status = nc_inq_varid (ncid_, "tgrid3", &tgrid3id_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for tgrid3."<< std::endl;
      std::cout<<nc_strerror(status)<< std::endl;
      exit(EXIT_FAILURE);
    }

  return true;
}


bool LamssNetCdf::open_noaa_nc(std::string nc_file_)
{

  // First open NetCdf file
  
  if (open_nc_file(nc_file_))
    return(true);

  int status;  /* error status */

  // >>>>>>>>>>>> Latitude
  status = nc_inq_dimid(ncid_, "lat", &latid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get dimension id for lat."<< std::endl;
      std::cout<<nc_strerror(status)<< std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "latid_=" << latid_ << std::endl;

  status = nc_inq_varid(ncid_, "lat", &lat_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for lat."<< std::endl;
      std::cout<<nc_strerror(status)<< std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "lat_varid_=" << lat_varid_ << std::endl;

  
  // >>>>>>>>>> Longitude	
  status = nc_inq_dimid(ncid_, "lon", &lonid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get dimension id for lon."<< std::endl;
      std::cout<<nc_strerror(status)<< std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "lonid_=" << lonid_ << std::endl;

    status = nc_inq_varid(ncid_, "lon", &lon_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for lon."<< std::endl;
      std::cout<<nc_strerror(status)<< std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "lon_varid_=" << lon_varid_ << std::endl;

  size_t lat_len;
  size_t lon_len;
  //get length of lat/lon dimensions
  status = nc_inq_dimlen(ncid_, latid_, &lat_len); /* get lat length */
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get lat_length."<< std::endl;
      std::cout<<nc_strerror(status)<< std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "lat_len=" << lat_len << std::endl;

  status = nc_inq_dimlen(ncid_, lonid_, &lon_len); /* get lon length */
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get lon_length."<< std::endl;
      std::cout<<nc_strerror(status)<< std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "lon_len=" << lon_len << std::endl;

  status = nc_inq_varid (ncid_, "Band1", &bathy_varid_);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get variable id for tbath."<< std::endl;
      std::cout<<nc_strerror(status)<< std::endl;
      exit(EXIT_FAILURE);
    }

  std::cout << "bathy_varid_=" << bathy_varid_ << std::endl;

  // read lat and log vectors
  lat_vec_.resize(lat_len);
  status = nc_get_var_double (ncid_, lat_varid_, &lat_vec_[0]);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get latitude vector"<< std::endl;
      std::cout<<nc_strerror(status)<< std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "latitudes: " << lat_vec_[0] << " , "
	    << lat_vec_[lat_vec_.size()-1]
       << std::endl;

  lon_vec_.resize(lon_len);
  status = nc_get_var_double (ncid_, lon_varid_, &lon_vec_[0]);
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get longitude vector"<< std::endl;
      std::cout<<nc_strerror(status)<< std::endl;
      exit(EXIT_FAILURE);
    }
  std::cout << "latitudes: " << lon_vec_[0] << " , " << lon_vec_[lon_vec_.size()-1]
       << std::endl;

  return true;
}

void LamssNetCdf::init_timebase(double m_start_time)
{
  moos_start_time = m_start_time;
}

std::vector<double> LamssNetCdf:: get_gdem_profile( double lat_, double lon_,
					      std::vector<double> dep_,
					      int month)
{
  int status;
  size_t lat_len;
  size_t lon_len;
  size_t dep_len;
  double dist_sq;
  double mindist = std::numeric_limits<double>::max();
  std::vector<double> mindist_sq;
  std::vector<int> min_ilat,min_ilon;
  double grid_val_lat;
  double grid_val_lon;
  double grid_lat_m;
  double grid_lon_m;
  double lat_m;
  double lon_m;
  double dlat_m= std::numeric_limits<double>::max();
  double dlon_m= std::numeric_limits<double>::max();
  std::vector<double> min_lat_val;
  std::vector<double> min_lon_val;
  std::vector<double> x,y,z;

  bool debug = false;
  
  x.resize(3);
  y.resize(3);
  z.resize(3);
  min_ilat.resize(3);
  min_ilon.resize(3);
  min_lat_val.resize(3);
  min_lon_val.resize(3);
  mindist_sq.resize(3);
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

  lon_len = lon_vec_.size();
  lat_len = lat_vec_.size();
  dep_len = dep_vec_.size();
  
  //get sound speed of closest 3 grid points
  // Note that here x is latitude and y longitude!

  // GDEM is a regular grid W->E, S->N,
  // so we can first narrow down 4 closest grid points

  int lat_indx_1 = 0;
  int lat_indx_2 = lat_len-1;
  int lon_indx_1 = 0;
  int lon_indx_2 = lon_len-1;

  for(int k=0;k<lon_len-1;k++)
    if (lon_-lon_vec_[k] >=0 )  
      lon_indx_1 = k;	  
  lon_indx_2 = lon_indx_1+1;

  for(int j=0;j<lat_len-1;j++)
    if (lat_-lat_vec_[j] >=0 )  
      lat_indx_1 = j;	  
  lat_indx_2 = lat_indx_1+1;

  // Now reduce to 3 and interpolate for all depths
  
  for(int n=0;n<3;n++)
    {
      mindist= std::numeric_limits<double>::max();

      for(int k=lon_indx_1;k<=lon_indx_2;k++)
	{
	  grid_val_lon = lon_vec_[k];	      
	  for(int j=lat_indx_1;j<=lat_indx_2;j++)
	    {
	      grid_val_lat = lat_vec_[j];	      

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
		  x[n]=grid_lat_m;
		  y[n]=grid_lon_m;
		}
	    }
	}
    }
  
  // is closest point to auv over 5km away?
  // if not, get me that bathymetry!
  
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

  if (debug)
    {
      std::cout << "lat,lon: " << lat_ << "N, " << lon_ << "E." << std::endl;
      std::cout << "lat_indx_1,lat_indx_2: " << lat_indx_1 << ", "
		<< lat_indx_2 << std::endl;
      std::cout << "lat_1, lat_2: " <<  lat_vec_[lat_indx_1] << "N, " << lat_vec_[lat_indx_2]  << "N." << std::endl;
      std::cout << "lon_indx_1,lon_indx_2: " << lon_indx_1 << ", "
		<< lon_indx_2 << std::endl;
      std::cout << "lon_1, lon_2: " <<  lon_vec_[lon_indx_1] << "E, " << lon_vec_[lon_indx_2]  << "E." << std::endl;
      
      std::cout << "dep_indx_1,dep_indx_2: " << dep_indx_1 << ", "
	    << dep_indx_2 << std::endl;
      std::cout << "min_depth,max_depth,dep_1,dep_2: " << min_depth << ", "
		<< max_depth << ", " << dep_vec_[dep_indx_1] << ", "
	    << dep_vec_[dep_indx_2] << std::endl;
      std::cout << "mindist0 " << sqrt(mindist_sq[0])
		<< "mindist1 " << sqrt(mindist_sq[1])
		<< "mindist2 " << sqrt(mindist_sq[2]) << std::endl;
    }
  std::vector<double> svp_out;

  
  if(mindist_sq[0]>(30000*30000)) /*default to -100.000 m ssp_z if over 30 km from bathy grid edge*/
    {
      for (int i=0; i<svp.size(); i++)
	svp[i]=default_ssp;

      std::cout << "mindist: " << sqrt(mindist_sq[0]) << std::endl;
  
    }
  else
    {
      //get lat/lon/bathy of closest grid points
      for (int i_dep=dep_indx_1; i_dep<=dep_indx_2; i_dep++)
	{
	  for(int n=0;n<3;n++)
	    {
	      int m_indx = month-1;
	      m_indx = std::max(0,std::min(11,m_indx));
	      size_t ssp_index[] = {m_indx,i_dep,min_ilat[n],min_ilon[n]};
	      
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
	      if (debug)
		{
		  std::cout << "n,lat,lon,val=" << n << ", "
		       << min_lat_val[n]<<"N x "<< min_lon_val[n]<<"E x "
		       << dep_vec_[i_dep] << " m. "
		       << z[n] << std::endl;
		}
	    }
	  //get lat/lon/bathy of closest grid point
	  svp[i_dep]  = interp_triangle(lat_m,lon_m,x,y,z);
	}
    }
  // Interpolate in depth
  if (dep_indx_1 == dep_indx_2)
    for (int i = 0; i< dep_.size(); i++)
      svp_out[i] = svp[dep_indx_1];
  else
    bool ok = vecUtils.interp(dep_vec_,svp,dep_,svp_out);
  return(svp_out);
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

double LamssNetCdf:: get_mseas_bathy( double lat_, double lon_)

{
  int status;
  size_t lat_len;
  size_t lon_len;
  double dist_sq;
  double mindist = std::numeric_limits<double>::max();
  std::vector<double> mindist_sq;
  std::vector<int> min_ilat,min_ilon;
  double grid_val_lat;
  double grid_val_lon;
  double grid_lat_m;
  double grid_lon_m;
  double lat_m;
  double lon_m;
  double dlat_m= std::numeric_limits<double>::max();
  double dlon_m= std::numeric_limits<double>::max();
  std::vector<double> min_lat_val;
  std::vector<double> min_lon_val;
  std::vector<double> x,y,z;
  double bathy_z;
  bool debug = false;
  
  x.resize(3);
  y.resize(3);
  z.resize(3);
  min_ilat.resize(3);
  min_ilon.resize(3);
  min_lat_val.resize(3);
  min_lon_val.resize(3);
  mindist_sq.resize(3);

  double default_bathy = std::numeric_limits<double>::quiet_NaN() ; // NaN

  //get length of lat/lon dimensions
  status = nc_inq_dimlen(ncid_, latid_, &lat_len); /* get lat length */
  if (status != NC_NOERR)
    {
      std::cout<<"ERROR - failed to get lat_length."<<std::endl;
      std::cout<<nc_strerror(status)<<std::endl;
      return(default_bathy);
    }

  status = nc_inq_dimlen(ncid_, lonid_, &lon_len); /* get lon length */
  if (status != NC_NOERR)
    {
      std::cout <<"ERROR - failed to get lon_length." << std::endl;
      std::cout <<nc_strerror(status) << std::endl;
      return(default_bathy);
    }

  //convert lat/lon position to local UTM


  if(!geodesy_.LatLong2LocalUTM(lat_,lon_,lat_m,lon_m)) 
  {
    std::cout << "ERROR: Failed to calculate UTM position. " << std::endl;
    exit(EXIT_FAILURE);
  }


  
  //get bathymetry closest 3 grid points
  // Note that here x is latitude and y longitude!

  for(int n=0;n<3;n++)
    {
      mindist= std::numeric_limits<double>::max();
      
      for(int k=0;k<=lon_len;k++)
	{
	  for(int j=0;j<=lat_len;j++)
	    {
	      size_t lon_ind[] = {j,k,0};
	      size_t lat_ind[] = {j,k,1};
	      
	      status = nc_get_var1_double(ncid_, tgrid2id_, lat_ind, &grid_val_lat);
	      if (status != NC_NOERR)
		{
		  std::cout<<"ERROR - failed to get grid(lat_index) value for "
			    << lat_ <<"N x "<< lon_<<"E."<<std::endl;
		  std::cout<<nc_strerror(status)<<std::endl;
		}
	      
	      status = nc_get_var1_double(ncid_, tgrid2id_, lon_ind, &grid_val_lon);
	      if (status != NC_NOERR)
		{
		  std::cout<<"ERROR - failed to get grid(lon_index) value for "
			   <<lat_<<"N x "<<lon_<<"E."<<std::endl;
		  std::cout<<nc_strerror(status)<<std::endl;
		  return(default_bathy);
		}
	      
	      // convert grid lat/lon to meters

              if(!geodesy_.LatLong2LocalUTM(grid_val_lat,grid_val_lon,grid_lat_m,grid_lon_m))
              {
		std:: cout << "Warning: Failed to calculate XY grid position. Is vehicle NAV_LAT=" << lat_ << "/NAV_LONG="<< lon_ <<" valid?" << std::endl;
		return(default_bathy);
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
		  x[n]=grid_lat_m;
		  y[n]=grid_lon_m;
		}
	    }
	}

      if(mindist_sq[n]>(5000*5000)) /*default to NaN bathy_z if over 5 km from bathy grid edge*/
	{
	  bathy_z=default_bathy;
	  std::cout<<"WARNING - AUV > 5 km from edge of known bathymetry grid. "
		   << "Defaulting bathy_z = "<<default_bathy <<" m."<< std::endl;
	  z[n] = bathy_z;
	}
      else
	{
	  
	  //get lat/lon/bathy of closest grid point
	  size_t bathy_index[] = {min_ilat[n],min_ilon[n]}; 
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

double LamssNetCdf::get_noaa_bathy( double lat_, double lon_)
{
  int status;
  size_t lat_len;
  size_t lon_len;
  size_t dep_len;
  double dist_sq;
  double mindist = std::numeric_limits<double>::max();
  std::vector<double> mindist_sq;
  std::vector<int> min_ilat,min_ilon;
  double grid_val_lat;
  double grid_val_lon;
  double grid_lat_m;
  double grid_lon_m;
  double lat_m;
  double lon_m;
  double dlat_m= std::numeric_limits<double>::max();
  double dlon_m= std::numeric_limits<double>::max();
  std::vector<double> min_lat_val;
  std::vector<double> min_lon_val;
  std::vector<double> x,y,z;
  double bathy_z;
  bool debug = false;
  
  x.resize(3);
  y.resize(3);
  z.resize(3);
  min_ilat.resize(3);
  min_ilon.resize(3);
  min_lat_val.resize(3);
  min_lon_val.resize(3);
  mindist_sq.resize(3);

  double default_bathy = std::numeric_limits<double>::quiet_NaN() ; // NaN

  //convert lat/lon position to local UTM

  if(!geodesy_.LatLong2LocalUTM(lat_,lon_,lat_m,lon_m)) 
  {
    std::cout << "ERROR: Failed to calculate UTM position. " << std::endl;
    exit(EXIT_FAILURE);
  }

  lon_len = lon_vec_.size();
  lat_len = lat_vec_.size();
  
  //get sound speed of closest 3 grid points
  // Note that here x is latitude and y longitude!

  // GDEM is a regular grid W->E, S->N,
  // so we can first narrow down 4 closest grid points

  int lat_indx_1 = 0;
  int lat_indx_2 = lat_len-1;
  int lon_indx_1 = 0;
  int lon_indx_2 = lon_len-1;

  for(int k=0;k<lon_len-1;k++)
    if (lon_-lon_vec_[k] >=0 )  
      lon_indx_1 = k;	  
  lon_indx_2 = lon_indx_1+1;

  for(int j=0;j<lat_len-1;j++)
    if (lat_-lat_vec_[j] >=0 )  
      lat_indx_1 = j;	  
  lat_indx_2 = lat_indx_1+1;

  // Now reduce to 3 and interpolate for all depths
  
  for(int n=0;n<3;n++)
    {
      mindist= std::numeric_limits<double>::max();

      for(int k=lon_indx_1;k<=lon_indx_2;k++)
	{
	  grid_val_lon = lon_vec_[k];	      
	  for(int j=lat_indx_1;j<=lat_indx_2;j++)
	    {
	      grid_val_lat = lat_vec_[j];	      

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
		  x[n]=grid_lat_m;
		  y[n]=grid_lon_m;
		}
	    }
	}

      if(mindist_sq[n]>(5000*5000)) /*default to NaN bathy_z if over 5 km from bathy grid edge*/
	{
	  bathy_z=default_bathy;
	  std::cout<<"WARNING - AUV > 5 km from edge of known bathymetry grid. "
		   << "Defaulting bathy_z = "<<default_bathy <<" m."<< std::endl;
	  z[n] = bathy_z;
	}
      else
	{
	  
	  //get lat/lon/bathy of closest grid point
	  size_t bathy_index[] = {min_ilat[n],min_ilon[n]}; 
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
    }
  //get lat/lon/bathy of closest grid point
  bathy_z = interp_triangle(lat_m,lon_m,x,y,z);
  return(bathy_z);
}
