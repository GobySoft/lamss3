// H. Schmidt 3/9/2018 Using iMseasbathy as template
// s. petillo spetillo@mit.edu 03.27.09
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is iNoaaBathy.cpp 
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

#include "iNoaaBathy.h"
#include "goby/moos/moos_protobuf_helpers.h"

using namespace std;

// Construction / Destruction
CiNoaaBathy::CiNoaaBathy() : got_bathy_request_(false),
                               auv_lat_(0),
			       auv_lon_(0),
			       nc_is_open_(false),
			       publish_interval_(5),
			       start_time_(0),
                               publish_count_(0)
{

}
CiNoaaBathy::~CiNoaaBathy()
{
  /*
  int status;
  status = nc_close(ncid_);       // close netCDF dataset 
  if (status != NC_NOERR)
    {
      cout<<"ERROR - nc_ts_file did not close!"<<endl;
      cout<<nc_strerror(status)<<endl;
      exit(EXIT_FAILURE);
    }
  else 
   {
     nc_is_open_ = false;
     cout<<"netcdf file < "<<nc_bathy_file_<<" > closed successfully!"<<endl;
   }
  */
}

// OnNewMail: called when new mail (previously registered for)
// has arrived.
bool CiNoaaBathy::OnNewMail(MOOSMSG_LIST &NewMail)
{
  for(MOOSMSG_LIST::iterator p = NewMail.begin(); p != NewMail.end(); p++)
    {
      CMOOSMsg &msg = *p;
        
      string key   = msg.GetKey(); 	
      //bool is_dbl  = msg.IsDouble();
      //bool is_str  = msg.IsString();
      double dval  = msg.GetDouble();
      //string sval  = msg.GetString();

      // uncomment as needed
      // double msg_time = msg.GetTime();
      // string msg_src  = msg.GetSource();
      // string msg_community = msg.GetCommunity();
	

      if(MOOSStrCmp(key, "NAV_LAT")) // case insensitive
        {
	  auv_lat_=dval;
          //cout<<"auv_lat_="<<auv_lat_<<endl;
        }
      else if (MOOSStrCmp(key, "NAV_LONG")) 
        {
	  auv_lon_=dval;
          //cout<<"auv_lon_="<<auv_lon_<<endl;
        }		
      else if (MOOSStrCmp(key, "BATHY_REQUEST")) 
	{
	  handle_outside_bathy_request(msg);
	}
    }  
  return true;
}

// OnConnectToServer: called when connection to the MOOSDB is
// complete
bool CiNoaaBathy::OnConnectToServer()
{
  ReadConfiguration(); //reads moos block
  RegisterVariables();

  nc_is_open_ = NOAAbathy.open_noaa_nc(nc_bathy_file_);
  /*
  if(nc_is_open_) return true;
  //load files here!...after reading moos block
  int status;  // error status 

  status = nc_open (nc_bathy_file_.c_str(), 0, &ncid_);
  if (status != NC_NOERR)
    {
      cout<<"ERROR - nc_ts_file did not open!"<<endl;
      cout<<nc_strerror(status)<<endl;
      exit(EXIT_FAILURE);
    }
  else 
    {
      nc_is_open_ = true;
      cout<<"netcdf file < "<<nc_bathy_file_<<" > opened successfully!"<<endl;
    }
  // >>>>>>>>>>>> Latitude
  status = nc_inq_dimid(ncid_, "lat", &latid_);
  if (status != NC_NOERR)
    {
      cout<<"ERROR - failed to get dimension id for lat."<<endl;
      cout<<nc_strerror(status)<<endl;
      exit(EXIT_FAILURE);
    }
  cout << "latid_=" << latid_ << endl;

  status = nc_inq_varid(ncid_, "lat", &lat_varid_);
  if (status != NC_NOERR)
    {
      cout<<"ERROR - failed to get variable id for lat."<<endl;
      cout<<nc_strerror(status)<<endl;
      exit(EXIT_FAILURE);
    }
  cout << "lat_varid_=" << lat_varid_ << endl;

  
  // >>>>>>>>>> Longitude	
  status = nc_inq_dimid(ncid_, "lon", &lonid_);
  if (status != NC_NOERR)
    {
      cout<<"ERROR - failed to get dimension id for lon."<<endl;
      cout<<nc_strerror(status)<<endl;
      exit(EXIT_FAILURE);
    }
  cout << "lonid_=" << lonid_ << endl;

    status = nc_inq_varid(ncid_, "lon", &lon_varid_);
  if (status != NC_NOERR)
    {
      cout<<"ERROR - failed to get variable id for lon."<<endl;
      cout<<nc_strerror(status)<<endl;
      exit(EXIT_FAILURE);
    }
  cout << "lon_varid_=" << lon_varid_ << endl;

  size_t lat_len;
  size_t lon_len;
  //get length of lat/lon dimensions
  status = nc_inq_dimlen(ncid_, latid_, &lat_len); // get lat length 
  if (status != NC_NOERR)
    {
      cout<<"ERROR - failed to get lat_length."<<endl;
      cout<<nc_strerror(status)<<endl;
      exit(EXIT_FAILURE);
    }
  cout << "lat_len=" << lat_len << endl;

  status = nc_inq_dimlen(ncid_, lonid_, &lon_len); // get lon length 
  if (status != NC_NOERR)
    {
      cout<<"ERROR - failed to get lon_length."<<endl;
      cout<<nc_strerror(status)<<endl;
      exit(EXIT_FAILURE);
    }
  cout << "lon_len=" << lon_len << endl;

  status = nc_inq_varid (ncid_, "Band1", &bathy_varid_);
  if (status != NC_NOERR)
    {
      cout<<"ERROR - failed to get variable id for tbath."<<endl;
      cout<<nc_strerror(status)<<endl;
      exit(EXIT_FAILURE);
    }

  cout << "bathy_varid_=" << bathy_varid_ << endl;

  // read lat and log vectors
  lat_vec_.resize(lat_len);
  status = nc_get_var_double (ncid_, lat_varid_, &lat_vec_[0]);
  if (status != NC_NOERR)
    {
      cout<<"ERROR - failed to get latitude vector"<<endl;
      cout<<nc_strerror(status)<<endl;
      exit(EXIT_FAILURE);
    }
  cout << "latitudes: " << lat_vec_[0] << " , " << lat_vec_[lat_vec_.size()-1]
       << std::endl;

  lon_vec_.resize(lon_len);
  status = nc_get_var_double (ncid_, lon_varid_, &lon_vec_[0]);
  if (status != NC_NOERR)
    {
      cout<<"ERROR - failed to get longitude vector"<<endl;
      cout<<nc_strerror(status)<<endl;
      exit(EXIT_FAILURE);
    }
  cout << "latitudes: " << lon_vec_[0] << " , " << lon_vec_[lon_vec_.size()-1]
       << std::endl;
*/
  return true;
}

// Iterate: called AppTick times per second
bool CiNoaaBathy::Iterate()
{ 
  // don't do anything until we open the nc file (in OnConnectToServer)
   if(!nc_is_open_)
     return true;

  // read imseas bathy on frequency
  if(MOOSTime()-start_time_ > publish_count_*publish_interval_)
    {
      //      ReadNOAABathy(auv_lat_,auv_lon_); 
      bathy_z_ = NOAAbathy.get_noaa_bathy(auv_lat_,auv_lon_); 
      m_Comms.Notify("BATHY_Z",bathy_z_);	// to publishes list
      ++publish_count_;
    }

  // get bathy for uSimSoundVelocityMSEAS upon request
  if(got_bathy_request_)
    {
      send_bathy_response();
      got_bathy_request_ = false;
    }

  return true;
}

// OnStartUp: called when program is run
bool CiNoaaBathy::OnStartUp()
{
  start_time_ = MOOSTime();
  return true;
}

// RegisterVariables: register for variables we want to get mail for
void CiNoaaBathy::RegisterVariables()
{
  // m_Comms.Register(variable, min_time_between_updates)
  m_Comms.Register("NAV_LAT", 0); 
  m_Comms.Register("NAV_LONG", 0); 
  m_Comms.Register("BATHY_REQUEST", 0); 
}

// ReadConfiguration: reads in configuration values from the .moos file
// parameter keys are case insensitive
bool CiNoaaBathy::ReadConfiguration()
{
  //Initialize m_Geodesy from lat lon origin in .moos file
  string sVal;
  double dfLatOrigin;
  double dfLongOrigin;
  
  if(m_MissionReader.GetValue("LatOrigin",sVal))
    {
      dfLatOrigin = atof(sVal.c_str());
      //MOOSTrace("  LatOrigin  = %10.5f deg.\n",dfLatOrigin);
    }
  else
    {
      MOOSTrace("LatOrigin not specified in mission file - FAIL\n");
      return false;
    }
  
  if(m_MissionReader.GetValue("LongOrigin",sVal))
    {
      dfLongOrigin = atof(sVal.c_str());
      //MOOSTrace("  LongOrigin = %10.5f deg.\n",dfLongOrigin);
    }
  else
    {
      MOOSTrace("LongOrigin not specified in mission file - FAIL\n");
      return false;
    }
  if(!geodesy_.Initialise(dfLatOrigin,dfLongOrigin))
    {
      MOOSTrace("Geodesy Init failed - FAIL\n");
      return false;
    }
  // Initialize geodesy in LamssNetcdf class
  NOAAbathy.init_geodesy(dfLatOrigin,dfLongOrigin);
  //Done initializaing m_Geodesy

  // for reading global values, use m_MissionReader.GetValue(parameter, variable)
  if (!m_MissionReader.GetConfigurationParam("nc_bathy_file", nc_bathy_file_))
    {
      cout<<"nc_bathy_file not specified in .moos file."<<endl;
      exit(EXIT_FAILURE);
    }
  cout<<"nc_bathy_file: "<<nc_bathy_file_<<endl;
  if (!m_MissionReader.GetConfigurationParam("publish_interval", publish_interval_))
    {
      cout<<"publish_interval not specified in .moos file. using default 5 sec."<<endl;
    }
  cout<<"publish_interval: "<<publish_interval_<<endl;
  
  return true;
}

void CiNoaaBathy::ReadNOAABathy( double lat_, double lon_)
{
  int status;
  size_t lat_len;
  size_t lon_len;
  double dist_sq;
  /*
  double mindist_sq=DBL_MAX, mindist2_sq=DBL_MAX, mindist3_sq=DBL_MAX;
  size_t min_ilat, min_ilat2, min_ilat3;
  size_t min_ilon, min_ilon2, min_ilon3;
  double grid_val_lat;
  double grid_val_lon;
  double grid_lat_m;
  double grid_lon_m;
  double auv_lat_m;
  double auv_lon_m;
  double dlat_m=DBL_MAX;
  double dlon_m=DBL_MAX;
  double x1, x2, x3, y1, y2, y3, z1, z2, z3;
  */
  double mindist=DBL_MAX;
  std::vector<double> mindist_sq;
  std::vector<size_t> min_ilat,min_ilon;
  double grid_val_lat;
  double grid_val_lon;
  double grid_lat_m;
  double grid_lon_m;
  double lat_m;
  double lon_m;
  double dlat_m=DBL_MAX;
  double dlon_m=DBL_MAX;
  std::vector<double> min_lat_val;
  std::vector<double> min_lon_val;

  std::vector<double> x,y,z;  
  x.resize(3);
  y.resize(3);
  z.resize(3);
  min_ilat.resize(3);
  min_ilon.resize(3);
  min_lat_val.resize(3);
  min_lon_val.resize(3);
  mindist_sq.resize(3);


  //convert auv position to local UTM
  geodesy_.Initialise(lat_,lon_);/*initializes to auv lat/lon as origin (0,0)*/
  if(!geodesy_.LatLong2LocalUTM(lat_,lon_,lat_m,lon_m)) 
  {
      cout << "Warning: Failed to calculate AUV position. Perhaps no NAV_LAT / NAV_LONG?" << std::endl;
      return;
  }

  lon_len = lon_vec_.size();
  lat_len = lat_vec_.size();
  
  // GDEM is a regular grid, so we can first narrow down closest grid points
  int lat_indx_1 = 0;
  int lat_indx_2 = lat_len-1;
  int lon_indx_1 = 0;
  int lon_indx_2 = lon_len-1;

  for(int k=0;k<lon_len;k++)
    if (lon_-lon_vec_[k] >=0 )  
      lon_indx_1 = k;	  
  lon_indx_2 = lon_indx_1+1;

  for(int j=0;j<lat_len;j++)
    if (lat_-lat_vec_[j] >=0 )  
      lat_indx_1 = j;	  
  lat_indx_2 = lat_indx_1+1;

  cout << "lat_indx = " << lat_indx_1 << "," << lat_indx_2 << std::endl;
  cout << "lon_indx = " << lon_indx_1 << "," << lon_indx_2 << std::endl;


  for(int n=0;n<3;n++)
    {
      mindist=DBL_MAX;

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
		  cout << "Warning: Failed to calculate XY grid position. "
		       << "Is vehicle NAV_LAT=" << lat_ << "/NAV_LONG="<<lon_
		       <<" valid?" << std::endl;
		  return;
		}
	      
	      // distance between auv and grid point
	      dlon_m=grid_lon_m-lon_m;
	      dlat_m=grid_lat_m-lat_m;
	      
	      dist_sq=dlat_m*dlat_m+dlon_m*dlon_m;
	      
	      
	      // is it closest point to auv?
	      if(mindist_sq[n]>(5000*5000)) /*default to -100.000 m ssp_z if over 30 km from bathy grid edge*/
		{
		  
		}
	      else if ( dist_sq < mindist && (n==0 || (n>0 && dist_sq > mindist_sq[n-1] ) ) )
		{
		  mindist=dist_sq;
		  mindist_sq[n]=dist_sq;

		  min_lat_val[n]=grid_val_lat;
		  min_lon_val[n]=grid_val_lon;
		  cout <<"min pos: "<<min_lat_val[n]
					 <<"N x "<<min_lon_val[n]<<"E"<<std::endl;
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
  
  if(mindist_sq[0]>(5000*5000)) /*default to -100.000 m bathy_z if over 5 km from bathy grid edge*/
    {
      double default_bathy_z = -100.000;
      bathy_z_=default_bathy_z;
      cout<<"WARNING - AUV > 5 km from edge of known bathymetry grid. Defaulting bathy_z = "<<default_bathy_z<<" m."<<endl;
      cout<<"bathymetry_z (m): "<<bathy_z_<<" *default* at "<<lat_
	  <<"N x "<<lon_<<"E."<<endl;
    }
  else
    {      
      for(int n=0;n<3;n++)
	{
	  //get lat/lon/bathy of closest grid point
	  int indx_lon = min_ilon[n];
	  int indx_lat = min_ilat[n];
	  size_t bathy_index[] = {indx_lat,indx_lon};  /* where to get value from */
	  cout<<"closest lat: "
	      <<std::setprecision(std::numeric_limits<double>::digits10)
				 << min_lat_val[n]<<"N"<<std::endl;
	  cout<<"closest lon: "
	      <<std::setprecision(std::numeric_limits<double>::digits10)
				 << min_lon_val[n]<<"E"<<std::endl;
	  cout<<"distance from auv: "<<sqrt(mindist_sq[n])<<" m"<<std::endl;

	  short val;
	  status = nc_get_var1_short(ncid_, bathy_varid_, bathy_index, &val);
	  if (status != NC_NOERR)
	    {
	      cout<<"ERROR - failed to get bathy value for "
		  << std::setprecision(std::numeric_limits<double>::digits10)
		  <<lat_<<"N x "<<lon_<<"E."<<endl;
	      cout<<nc_strerror(status)<<endl;
	      cout << "min_ilon, min_ilat: " << min_ilon[n] << " , " << min_ilat[n]
					  << endl;
	      return;
	    }
	  else 
	    {
	      z[n] = val;
	      cout<<"bathy_z (m): "<<z[n]<<" at closest point to "
		  << std::setprecision(std::numeric_limits<double>::digits10)
		  <<lat_<<"N x "<<lon_<<"E."<<endl;
	    }		    
	}      
      
      bathy_z_ = interp_bathy(lat_m,lon_m,x,y,z);
      
      cout<<"bathymetry_z (m): "<<bathy_z_<<" *interpolated* at "
	  <<lat_<<"N x "<<lon_<<"E."<<endl;
      cout<<endl;
    }
  
  return;
}

double CiNoaaBathy::interp_bathy(double x_here,double y_here,
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
  
      c=(-a*(e*i-f*h)+b*(d*i-f*g))/(d*h-e*g);

      z_here=c+z[0];
      return(z_here);

}
  


void CiNoaaBathy::handle_outside_bathy_request(const CMOOSMsg& msg)
{
  // BATHY_REQUEST = "request_id=,lat=,lon="
  
  SVP::BathyRequest bathy_request;
  parse_for_moos(msg.GetString(), &bathy_request);

  if(!bathy_request.IsInitialized())
    {
      std::cout << "Got Bathy Request missing required fields!" << std::endl;      
      return;
    }
  std::cout << "Got Bathy Request that is: " << bathy_request.DebugString() << std::endl;

  // parse message
  bathy_request_id_ = bathy_request.request_id();

  bathy_request_lat_vec_.resize(bathy_request.latitude().size());
  bathy_request_lon_vec_.resize(bathy_request.longitude().size());
  std::copy(bathy_request.latitude().begin(),bathy_request.latitude().end(),bathy_request_lat_vec_.begin());
  std::copy(bathy_request.longitude().begin(),bathy_request.longitude().end(),bathy_request_lon_vec_.begin());

  got_bathy_request_ = true;
}

void CiNoaaBathy::send_bathy_response()
{
  SVP::BathyResponse bathy_response;

  // Publish Bathy_Response message for all range profiles
  // BATHY_RESPONSE = "request_id=,bathy="
  bathy_response.set_request_id(bathy_request_id_);

  double tmp_lat = auv_lat_, tmp_lon = auv_lon_;
  for(int i=0, n=bathy_request_lat_vec_.size(); i<n; i++){
    auv_lat_ = bathy_request_lat_vec_.at(i);
    auv_lon_ = bathy_request_lon_vec_.at(i);
    bathy_z_ = NOAAbathy.get_noaa_bathy(auv_lat_,auv_lon_); 
    //    ReadNOAABathy(auv_lat_,auv_lon_);
    bathy_response.add_bathy(bathy_z_);
  }
  auv_lat_ = tmp_lat; 
  auv_lon_ = tmp_lon;

  std::string serialized_bathy_response;
  serialize_for_moos(&serialized_bathy_response, bathy_response);
  m_Comms.Notify("BATHY_RESPONSE", serialized_bathy_response);

  bathy_response_sent_ = true;
}
