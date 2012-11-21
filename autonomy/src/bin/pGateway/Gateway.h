/*****************************************************************/
/*    NAME: Henrik Schmidt                                       */
/*    ORGN: MIT Cambridge MA                                     */
/*    FILE: Gateway.h                                 */
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

#ifndef GATEWAY_HEADER
#define GATEWAY_HEADER

#include "MOOSLIB/MOOSLib.h"
#include "MOOSGenLib/MOOSGenLib.h"
#include "MOOSUtilityLib/MOOSGeodesy.h"
#include <string>



class CGateway : public CMOOSApp
{
 public:
  CGateway();
  virtual ~CGateway();

  bool Iterate();
  bool OnNewMail(MOOSMSG_LIST &NewMail);
  bool OnConnectToServer();
  bool OnStartUp();
  
 protected:

  CMOOSGeodesy m_Geodesy; 
  double start_time,deploy_x,deploy_y, deploy_spd, deploy_hdg,deploy_depth,depth_trigger;
  double deploy_lat,deploy_lon;
  int dest_addr;

  //  bool UTM2LatLong(double, double, double&, double& );
  double latitude, longitude;

};

#endif 

