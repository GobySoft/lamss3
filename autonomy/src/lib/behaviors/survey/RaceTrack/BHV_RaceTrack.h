/*****************************************************************/
/*    NAME: Henrik Schmidt                                       */
/*    ORGN: MIT Cambridge MA                                     */
/*    FILE: BHV_RaceTrack.h                                   */
/*    DATE: Mar 3, 2007                                          */
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
 
#ifndef BHV_RACETRACK_HEADER
#define BHV_RACETRACK_HEADER

#include "goby/moos/moos_geodesy.h"

#include <string>
#include "IvPBehavior.h"

#include "XYPolygon.h"
#include "WaypointEngine.h"


class IvPDomain;
class BHV_RaceTrack : public IvPBehavior {
public:
  BHV_RaceTrack(IvPDomain);
  ~BHV_RaceTrack() {};
  
  bool         setParam(std::string, std::string);
  IvPFunction* onRunState();
  void         onIdleState();
  void         onCompleteState();

protected:
  void         updateStart();
  double       currentCompensatedHeading(double desired_current, double water_speed);

  void         postViewablePolygon();
  void         postErasablePolygon();
  void         postViewablePoint();
  void         postErasablePoint();
  void         handleVisualHint(std::string);

private: // Behavior Parameters

  goby::moos::CMOOSGeodesy m_Geodesy;

  // Dynamic UTM
  double lat_origin_;
  double lon_origin_;
  bool got_lat_origin;
  bool got_lon_origin; 
  bool geo_init;

  double track_length, approach_angle, dist_tolerance;
  double distance_traveled,heading,track_width,deploy_x,deploy_y;
  double head_x,head_y,new_heading,speed, heading_out, heading_in,head_angle;
  double bearing;
  double deploy_x_out,deploy_y_out;
  double deploy_lat_out,deploy_lon_out;
  double end_x_out,end_y_out;
  double deploy_x_in,deploy_y_in;
  double end_x_in,end_y_in;
  int leg;
  double peak_width;
  double summit_delta;

  double east_current_spd;
  double north_current_spd;

  bool m_clockwise;
  bool         m_deploy_pending; 
  bool         m_in_track;
  bool         m_in_track_posted;
  bool         m_on_trackline;
  bool         m_on_trackline_posted;
  bool         m_current_compensator;
 
  std::string  m_deploy_location;  
  double m_turn_distance;

private: // Behavior State Variables
  bool        first_iteration;
  double      bhv_start;
  std::string  m_hint_nextpt_color;
  std::string  m_hint_nextpt_lcolor;
  std::string  m_hint_vertex_color;
  std::string  m_hint_edge_color;
  std::string  m_hint_poly_label;
  double       m_hint_vertex_size;
  double       m_hint_edge_size;

  WaypointEngine m_waypoint_engine;
  
  double       m_ptx;            // Curr waypt   x/lon (mtrs)
  double       m_pty;            // Curr waypt   y/lat (mtrs)

};

extern "C" {
  IvPBehavior * createBehavior(std::string name, IvPDomain domain)
  {return new BHV_RaceTrack(domain);}
}

#endif
 
