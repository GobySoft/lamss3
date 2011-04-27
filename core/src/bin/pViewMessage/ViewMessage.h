/*****************************************************************/
/*    NAME: Henrik Schmidt                                       */
/*    ORGN: MIT Cambridge MA                                     */
/*    FILE: ViewMessage.h                                 */
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

#ifndef VIEWMESSAGE_HEADER
#define VIEWMESSAGE_HEADER

#include "MOOSLIB/MOOSLib.h"
#include "MOOSGenLib/MOOSGenLib.h"
#include "lib_mbutil/MBUtils.h"
#include "MOOSUtilityLib/MOOSGeodesy.h"
#include "lib_geometry/XYPoint.h"
#include <string>




class CViewMessage : public CMOOSApp
{
 public:
  CViewMessage();
  virtual ~CViewMessage();

  bool Iterate();
  bool OnNewMail(MOOSMSG_LIST &NewMail);
  bool OnConnectToServer();
  bool OnStartUp();
  bool insertPoint(std:: string);
  bool deletePoint(std:: string);
  bool insertPolygon(std:: string);
  bool insertSeglist(std:: string);
  bool sampleSeglist(int n_pnt, std::vector<double>& x,std::vector<double>& y);

 protected:

  CMOOSGeodesy m_Geodesy; 

  std::string source;
  std::string vehicle;
  std::string ownship;


  std::string pt_active;
  std::string pt_type;
  std::string pt_label;

  double pt_x;
  double pt_y;

  int m_poly_max;

  std::vector<double> m_pnt_x;
  std::vector<double> m_pnt_y;
  std::vector<bool> m_pnt_valid;
  std::vector<std::string> m_pnt_type;

  std::string poly_active;
  std::string poly_type;
  std::string poly_label;

  std::string segl_active;
  std::string segl_type;
  std::string segl_label;

  std::string mission_state;
  std::string old_state;

  std::vector<double> x;
  std::vector<double> y;

  std::vector<double> x_sl;
  std::vector<double> y_sl;

  bool new_point;
  bool new_poly;
  bool new_segl;

  bool active;
  bool kill_old;
  bool polygon;
  bool seglist;

  double edge_size;
  double vertex_size;

};

#endif 

