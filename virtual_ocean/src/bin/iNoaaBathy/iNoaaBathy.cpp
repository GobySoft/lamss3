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
  int status;
  status = nc_close(ncid_);       /* close netCDF dataset */
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

  if(nc_is_open_) return true;
  //load files here!...after reading moos block
  int status;  /* error status */

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
  status = nc_inq_dimlen(ncid_, latid_, &lat_len); /* get lat length */
  if (status != NC_NOERR)
    {
      cout<<"ERROR - failed to get lat_length."<<endl;
      cout<<nc_strerror(status)<<endl;
      exit(EXIT_FAILURE);
    }
  cout << "lat_len=" << lat_len << endl;

  status = nc_inq_dimlen(ncid_, lonid_, &lon_len); /* get lon length */
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

  cout << "bathyid_=" << bathy_varid_ << endl;

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

  bathy_mat_.resize(lon_len);
  for (int i=0; i< lon_len; i++)
    bathy_mat_.resize(lat_len);

  short val;
  size_t bathy_index[] ={0,0};
  status = nc_get_var1_short (ncid_, bathy_varid_, bathy_index, &val);
  if (status != NC_NOERR)
    {
      cout<<"ERROR - failed to get bathy[0][0] "<<endl;
      cout<<nc_strerror(status)<<endl;
      exit(EXIT_FAILURE);
    }
  cout << "bathy: " << val << " , "
       << std::endl;
  

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
      ReadBathy(); 
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

void CiNoaaBathy::ReadBathy()
{
  int status;
  size_t lat_len;
  size_t lon_len;
  double dist_sq;
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
  

  //convert auv position to local UTM
  geodesy_.Initialise(auv_lat_,auv_lon_);/*initializes to auv lat/lon as origin (0,0)*/
  if(!geodesy_.LatLong2LocalUTM(auv_lat_,auv_lon_,auv_lat_m,auv_lon_m)) 
  {
      cout << "Warning: Failed to calculate AUV position. Perhaps no NAV_LAT / NAV_LONG?" << std::endl;
      return;
  }

  lon_len = lon_vec_.size();
  lat_len = lat_vec_.size();
  
  //get bathy of closest 3 grid points
  for(size_t n=0;n<3;++n)
    {
      for(size_t k=0;k<lon_len;++k)
	{
	  grid_val_lon = lon_vec_[k];	      
	  for(size_t j=0;j<lat_len;++j)
	    {
	      grid_val_lat = lat_vec_[j];	      
	      // convert grid lat/lon to meters
	      
              if(!geodesy_.LatLong2LocalUTM(grid_val_lat,grid_val_lon,grid_lat_m,grid_lon_m))
		{
                  cout << "Warning: Failed to calculate XY grid position. Is vehicle NAV_LAT=" << auv_lat_ << "/NAV_LONG="<<auv_lon_ <<" valid?" << std::endl;
                  return;
		}
	      
	      // distance between auv and grid point
	      dlon_m=grid_lon_m-auv_lon_m;
	      dlat_m=grid_lat_m-auv_lat_m;
	      
	      dist_sq=dlat_m*dlat_m+dlon_m*dlon_m;
	      
	      
	      if (n==0)
		{
		  
		  // is it closest point to auv?
		if (dist_sq < mindist_sq)
		  {
		    mindist_sq=dist_sq;
		    // cout<<"mindist: "<<sqrt(mindist_sq)<<endl;
		    min_lat_val_=grid_val_lat;
		    min_lon_val_=grid_val_lon;
		    // cout<<"min pos: "<<min_lat_val_<<"N x "<<min_lon_val_<<"E"<<endl;
		    min_ilat=j;
		    min_ilon=k;
		    x1=grid_lat_m;
		    y1=grid_lon_m;
		  }
		}
	      if (n==1)
		{
		// is it 2nd closest point to auv?
		  if(mindist_sq>(5000*5000)) /*default to -100.000 m bathy_z if over 5 km from bathy grid edge*/
		  {
		  
		  }
		else if (dist_sq > mindist_sq && dist_sq < mindist2_sq)
		  {
		    mindist2_sq=dist_sq;
		    //cout<<"mindist2_sq: "<<mindist2_sq<<endl;
		    min_lat_val2_=grid_val_lat;
		    min_lon_val2_=grid_val_lon;
		    min_ilat2=j;
		    min_ilon2=k;
		    x2=grid_lat_m;
		    y2=grid_lon_m;
		  }		  
		}
	      if (n==2)
		{
		// is it 3rd closest point to auv?
		  if(mindist_sq>(5000*5000)) /*default to -100.000 m bathy_z if over 5 km from bathy grid edge*/
		  {
		  
		  }
		else if (dist_sq > mindist2_sq && dist_sq < mindist3_sq)
		  {
		    mindist3_sq=dist_sq;
		    //cout<<"mindist3_sq: "<<mindist3_sq<<endl;;
		    min_lat_val3_=grid_val_lat;
		    min_lon_val3_=grid_val_lon;
		    min_ilat3=j;
		    min_ilon3=k;
		    x3=grid_lat_m;
		    y3=grid_lon_m;
		  }		  
		}
	    }
	}
    }

  
  // is closest point to auv over 5km away?
  // if not, get me that bathymetry!
  
  if(mindist_sq>(5000*5000)) /*default to -100.000 m bathy_z if over 5 km from bathy grid edge*/
    {
      double default_bathy_z = -100.000;
      bathy_z_=default_bathy_z;
      cout<<"WARNING - AUV > 5 km from edge of known bathymetry grid. Defaulting bathy_z = "<<default_bathy_z<<" m."<<endl;
      cout<<"bathymetry_z (m): "<<bathy_z_<<" *default* at "<<auv_lat_<<"N x "<<auv_lon_<<"E."<<endl;
    }
  else
    {
      
      //get lat/lon/bathy of closest grid point
      size_t bathy_index1[] = {min_ilon,min_ilat};  /* where to get value from */
      
      cout<<"closest lat1: "<<std::setprecision(std::numeric_limits<double>::digits10) << min_lat_val_<<"N"<<endl;
      cout<<"closest lon1: "<<std::setprecision(std::numeric_limits<double>::digits10) << min_lon_val_<<"E"<<endl;
      cout<<"distance1 from auv: "<<sqrt(mindist_sq)<<" m"<<endl;

      short val;
      status = nc_get_var1_short(ncid_, bathyid_, bathy_index1, &val);
      if (status != NC_NOERR)
	{
            cout<<"ERROR - failed to get bathy1 value for "<< std::setprecision(std::numeric_limits<double>::digits10) <<auv_lat_<<"N x "<<auv_lon_<<"E."<<endl;
	  cout<<nc_strerror(status)<<endl;
	  return;
	}
      else 
	{
	  z1 = val;
	  cout<<"bathy_z1 (m): "<<z1<<" at closest point to " << std::setprecision(std::numeric_limits<double>::digits10) <<auv_lat_<<"N x "<<auv_lon_<<"E."<<endl;
	}		    
      
      //get lat/lon/bathy of closest grid point
      size_t bathy_index2[] = {min_ilon2,min_ilat2};  /* where to get value from */
		    
          cout<<"closest lat2: " << std::setprecision(std::numeric_limits<double>::digits10) <<min_lat_val2_<<"N"<<endl;
      cout<<"closest lon2: "<<std::setprecision(std::numeric_limits<double>::digits10) <<min_lon_val2_<<"E"<<endl;
      cout<<"distance2 from auv: "<<sqrt(mindist2_sq)<<" m"<<endl;
      
      status = nc_get_var1_short(ncid_, bathyid_, bathy_index2, &val);
      if (status != NC_NOERR)
	{
            cout<<"ERROR - failed to get bathy2 value for "<< std::setprecision(std::numeric_limits<double>::digits10) <<auv_lat_<<"N x "<<auv_lon_<<"E."<<endl;
	  cout<<nc_strerror(status)<<endl;
	  return;
	}
      else 
	{
	  z2 = val;
	  cout<<"bathy_z2 (m): "<<z2<<" at 2nd closest point to "<< std::setprecision(std::numeric_limits<double>::digits10) <<auv_lat_<<"N x "<<auv_lon_<<"E."<<endl;
	}		    
      
      
      //get lat/lon/bathy of closest grid point
      size_t bathy_index3[] = {min_ilon3,min_ilat3};  /* where to get value from */
		    
      cout<<"closest lat3: "<<std::setprecision(std::numeric_limits<double>::digits10)<<min_lat_val3_<<"N"<<endl;
      cout<<"closest lon3: "<<std::setprecision(std::numeric_limits<double>::digits10)<<min_lon_val3_<<"E"<<endl;
      cout<<"distance3 from auv: "<<sqrt(mindist3_sq)<<" m"<<endl;
      
      status = nc_get_var1_short(ncid_, bathyid_, bathy_index3, &val);
      if (status != NC_NOERR)
	{
	  cout<<"ERROR - failed to get bathy3 value for "<<std::setprecision(std::numeric_limits<double>::digits10)<<auv_lat_<<"N x "<<auv_lon_<<"E."<<endl;
	  cout<<nc_strerror(status)<<endl;
	  return;
	}
      else 
	{
	  z3 = val;
	  cout<<"bathy_z3 (m): "<<z3<<" at 3rd closest point to "<<std::setprecision(std::numeric_limits<double>::digits10)<<auv_lat_<<"N x "<<auv_lon_<<"E."<<endl;
	}		    
      
      
      // determinant calculation to determine interpolated bathymetry between three closest points
      // define a plane Ax+By+Cz=D, say D=1
      // 
      // for a plane defined by 3 given points:
      // 	  | x-x1   y-y1   z-z1  |
      //      det | x2-x1  y2-y1  z2-z1 | = 0
      //	  | x3-x1  y3-y1  z3-z1 |
      
      double a, b, c, d, e, f, g, h, i, x, y, z;
      x=auv_lat_m;
      y=auv_lon_m;

      //cout<<"x: "<<x<<endl;
      //cout<<"y: "<<y<<endl;
      
      a=x-x1;
      b=y-y1;
      // c=z-z1; --> solve for c --> z
      d=x2-x1;
      e=y2-y1;
      f=z2-z1;
      g=x3-x1;
      h=y3-y1;
      i=z3-z1;
  
      c=(-a*(e*i-f*h)+b*(d*i-f*g))/(d*h-e*g);
      //cout<<"c: "<<c<<endl;
      z=c+z1;
      bathy_z_=z;
      cout<<"bathymetry_z (m): "<<bathy_z_<<" *interpolated* at "<<auv_lat_<<"N x "<<auv_lon_<<"E."<<endl;
      cout<<endl;
    }
  
  
  
  return;
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
    ReadBathy();
    bathy_response.add_bathy(bathy_z_);
  }
  auv_lat_ = tmp_lat; 
  auv_lon_ = tmp_lon;

  std::string serialized_bathy_response;
  serialize_for_moos(&serialized_bathy_response, bathy_response);
  m_Comms.Notify("BATHY_RESPONSE", serialized_bathy_response);

  bathy_response_sent_ = true;
}
