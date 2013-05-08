/*****************************************************************/
/*    NAME: Henrik Schmidt                                       */
/*    ORGN: MIT Cambridge MA                                     */
/*    FILE: Gateway.cpp                                 */
/*    DATE: Aug. 2, 2008                                         */
/*                                                               */
/* This program is free software; you can redistribute it and/or */
/* modify it under the terms of the GNU General Public License   */
/* as published by the Free Software Foundation; either version  */
/* 2 of the License, or (at your option) any later version.      */
/*                                                               */
/* This program is distributed in the hope that it will be       */
/* useful, but WITHOUT ANY WARRANTY; without even the implied    */
/* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       */
/* PURPOSE. See the GNU General Public License for more details. */
/*                                                               */
/* You should have received a copy of the GNU General Public     */
/* License along with this program; if not, write to the Free    */
/* Software Foundation, Inc., 59 Temple Place - Suite 330,       */
/* Boston, MA 02111-1307, USA.                                   */
/*****************************************************************/

#include "Gateway.h"

#include <iostream>
#include <sstream>
#include <math.h>

using namespace std;


CGateway::CGateway()
{
  deploy_x = 0;
  deploy_y = 0;
}

CGateway::~CGateway()
{
}

bool CGateway::OnStartUp()
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
  if(!m_Geodesy.Initialise(dfLatOrigin,dfLongOrigin))
    {
      MOOSTrace("Geodesy Init inside pTargetSim failed - FAIL\n");
      return false;
    }
  //Done initializaing m_Geodesy
  
  double d_x;
  if(m_MissionReader.GetConfigurationParam("deploy_x",d_x))
    {
      deploy_x = d_x;
      // m_Geodesy.UTM2LatLong(deploy_x,deploy_y,deploy_lat,deploy_lon);
      m_Geodesy.UTM2LatLong(deploy_x,deploy_y, latitude, longitude);
      deploy_lat = latitude;
      deploy_lon = longitude;
    }     
  
  double d_y;
  if(m_MissionReader.GetConfigurationParam("deploy_y",d_y))
    {
      deploy_y = d_y;
      // m_Geodesy.UTM2LatLong(deploy_x,deploy_y,deploy_lat,deploy_lon);    
      m_Geodesy.UTM2LatLong(deploy_x,deploy_y, latitude, longitude);
      deploy_lat = latitude;
      deploy_lon = longitude;
    }

  if(m_MissionReader.GetConfigurationParam("deploy_longitude",d_x))
    {
      deploy_lon = d_x;
    }     
  
  if(m_MissionReader.GetConfigurationParam("deploy_latitude",d_y))
    {
      deploy_lat = d_y;
    }     
  
  double d_depth;
  
  if(m_MissionReader.GetConfigurationParam("deploy_depth",d_depth))
    {
      deploy_depth = d_depth;
    }
  else
    {
      MOOSTrace("pGateway:: deploy depth not set - FAIL\n");        
      return false;
    } 
  
  start_time = MOOSTime();
  
  if (m_Geodesy.LatLong2LocalUTM(deploy_lat,deploy_lon,deploy_y,deploy_x))
    {
      MOOSTrace("Gateway Deploy location: Lat = %10.6f, Long =  %10.6f\n", deploy_lat,deploy_lon);
      MOOSTrace("Gateway Deploy location: x = %f, Long =  %f\n", deploy_x,deploy_y);
    }
  else
    {
      MOOSTrace("Error converting Latitude/Longitude to local coordinates\n");
      return(false);
    }
  return(true);
}

bool CGateway::OnConnectToServer()
{
  
  return(true);
}


bool CGateway::Iterate()
{
  
  double curr_time = MOOSTime();
  m_Comms.Notify("NAV_X",deploy_x);
  m_Comms.Notify("NAV_Y",deploy_y);
  double d_hdg = 0.;
  double d_spd = 0;
  m_Comms.Notify("NAV_HEADING",d_hdg);
  m_Comms.Notify("NAV_SPEED",d_spd);
  m_Comms.Notify("NAV_DEPTH",deploy_depth);
  m_Comms.Notify("NAV_HEADING",d_hdg);
  m_Comms.Notify("NAV_LAT",deploy_lat);
  m_Comms.Notify("NAV_LONG",deploy_lon);

  return(true);
}


//-------------------------------------------------------------
// Procedure: OnNewMail

bool CGateway::OnNewMail(MOOSMSG_LIST &NewMail)

{
  return(true);
}

/*
bool CGateway::UTM2LatLong(double x_in, double y_in, double& new_lat, double& new_lon)
{
  double lat, lon, new_x, new_y ;

  double err = 1e20;
  double x=x_in;
  double y=y_in;

  while (err > 1)
    { 
      // first guess geodesic
      if (!m_Geodesy.UTM2LatLong(x,y,lat,lon))
	return(false);

      // MOOSTrace("x=%f lon = %f\n",x,lon);
      
      // now convert latlong to UTM
      if (!m_Geodesy.LatLong2LocalUTM(lat,lon,new_y,new_x))
	return(false);

      // MOOSTrace("new_x=%f \n",new_x);
      
      // how different
      double diff_x = new_x -x_in;
      double diff_y = new_y -y_in;
      // subtract difference and reconvert
      
      x = x - diff_x;
      y = y - diff_y;

      // MOOSTrace("corr_x=%f \n",x);

      err = sqrt(pow(new_x-x_in,2)+pow(new_y-y_in,2));

      MOOSTrace("UTM2LatLong: error = %f\n",err); 
    }

  if (!m_Geodesy.UTM2LatLong(x, y, new_lat, new_lon))
    return(false);

  //  latitude = new_lat;
  //  longitude = new_lon;
  
  return(true);
}
*/
