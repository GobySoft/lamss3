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
                               auv_x_(0),
			       auv_y_(0),
			       nc_is_open_(false),
			       publish_interval_(5),
			       start_time_(0),
                               publish_count_(0)
{

}
CiNoaaBathy::~CiNoaaBathy()
{
}

// OnNewMail: called when new mail (previously registered for)
// has arrived.
bool CiNoaaBathy::OnNewMail(MOOSMSG_LIST &NewMail)
{
  double dfLatOrigin, dfLongOrigin;
  bool new_origin = false;
  
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
	

      if(msg.m_sKey == "LAT_ORIGIN") 
	{
	  dfLatOrigin = msg.GetDouble();;
	  new_origin = true;
	}
    
      if(msg.m_sKey == "LONG_ORIGIN") 
	{
	  dfLongOrigin = msg.GetDouble();
	  new_origin = true;
	}
    
      if(MOOSStrCmp(key, "NAV_X")) // case insensitive
        {
	  auv_x_=dval;
          //cout<<"auv_x_="<<auv_x_<<endl;
        }
      else if (MOOSStrCmp(key, "NAV_Y")) 
        {
	  auv_y_=dval;
          //cout<<"auv_y_="<<auv_y_<<endl;
        }		
      else if (MOOSStrCmp(key, "BATHY_REQUEST")) 
	{
	  handle_outside_bathy_request(msg);
	}
    }  

  if(new_origin)
    {
      if(!geodesy_.Initialise(dfLatOrigin,dfLongOrigin))
	MOOSTrace("Geodesy Init failed - FAIL\n");
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
      geodesy_.UTM2LatLong(auv_x_,auv_y_,auv_lat_,auv_lon_);
      bathy_z_ = NOAAbathy.get_noaa_bathy(auv_lat_,auv_lon_); 
      m_Comms.Notify("BATHY_Z",bathy_z_);	// to publishes list
      ++publish_count_;
    }

  // get bathy for uSimSoundVelocityMSEAS upon request
  if(got_bathy_request_)
    {
      std::cout << "Got BATHY_REQUEST " << std::endl;
      send_bathy_response();
      std::cout << "Sent BATHY_RESPONSE " << std::endl;
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
  m_Comms.Register("LAT_ORIGIN", 0); 
  m_Comms.Register("LONG_ORIGIN", 0); 
  m_Comms.Register("NAV_X", 0); 
  m_Comms.Register("NAV_Y", 0); 
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
  
  // Initialize LamssNetcdf class and open file
  NOAAbathy.initialize_noaa_nc(nc_bathy_file_,
			       dfLatOrigin,dfLongOrigin,start_time_,false);

  return true;
}



void CiNoaaBathy::handle_outside_bathy_request(const CMOOSMsg& msg)
{
  // BATHY_REQUEST = "request_id=,lat=,lon="
  
  netsim::SVP::protobuf::BathyRequest bathy_request;
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
  netsim::SVP::protobuf::BathyResponse bathy_response;

  // Publish Bathy_Response message for all range profiles
  // BATHY_RESPONSE = "request_id=,bathy="
  bathy_response.set_request_id(bathy_request_id_);

  double tmp_lat = auv_lat_, tmp_lon = auv_lon_;
  for(int i=0, n=bathy_request_lat_vec_.size(); i<n; i++){
    auv_lat_ = bathy_request_lat_vec_.at(i);
    auv_lon_ = bathy_request_lon_vec_.at(i);
    bathy_z_ = NOAAbathy.get_noaa_bathy(auv_lat_,auv_lon_); 
    bathy_response.add_bathy(bathy_z_);
  }
  auv_lat_ = tmp_lat; 
  auv_lon_ = tmp_lon;

  std::string serialized_bathy_response;
  serialize_for_moos(&serialized_bathy_response, bathy_response);
  m_Comms.Notify("BATHY_RESPONSE", serialized_bathy_response);

  bathy_response_sent_ = true;
}
