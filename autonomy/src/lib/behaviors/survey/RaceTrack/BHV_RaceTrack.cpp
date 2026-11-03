/*****************************************************************/
/*    NAME: Henrik Schmidt                                       */
/*    ORGN: MIT Cambridge MA                                     */
/*    FILE: BHV_RaceTrack.cpp                                 */
/*    DATE: Mar. 3, 2007                                         */
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

#ifdef _WIN32
#pragma warning(disable : 4786)
#pragma warning(disable : 4503)
#endif
#include <math.h> 
#include <stdlib.h>
#include "BHV_RaceTrack.h"
#include "MBUtils.h"
#include "BuildUtils.h"
#include "AngleUtils.h"
#include "ZAIC_PEAK.h"
#include "OF_Coupler.h"
#include "XYFormatUtilsPoly.h"

using namespace std;

//-----------------------------------------------------------
// Procedure: Constructor

BHV_RaceTrack::BHV_RaceTrack(IvPDomain gdomain) : 
  IvPBehavior(gdomain)
{
  this->setParam("descriptor", "(d)bhv_racetrack");

  // Behavior Parameter Default Values:
  track_length               = 500; // meters
  track_width            = 100;   // meters
  heading              = 0;     // degrees
  heading_out          = 0;     // degrees
  heading_in           = 0;     // degrees
  distance_traveled    = 0;   // meters
  approach_angle = 80;
  dist_tolerance = 10;
  // zaic defaults
  peak_width = 30;
  summit_delta = 50;
  // Behavior State Variable Initial Values:
  first_iteration      = true;
  deploy_x = 0;
  deploy_y = 0;
  m_current_compensator = false;
  m_clockwise = false;
  m_deploy_pending = false;
  m_deploy_location = "";
  m_turn_distance = 100;
  m_in_track = false;
  m_in_track_posted = false;
  postMessage("IN_TRACK","FALSE");
  m_on_trackline = false;
  m_on_trackline_posted = false;
  postMessage("ON_TRACKLINE","FALSE");

  m_domain = subDomain(m_domain, "speed,course");

  north_current_spd = 0.0;
  east_current_spd = 0.0;

  // the following requested by Stephanie Kemna
  m_hint_vertex_size = 5;
  m_hint_edge_size =1;

  // Declare the variables we will need from the info_buffer
  addInfoVars("NAV_X");
  addInfoVars("NAV_Y");
  addInfoVars("CURRENT_X");
  addInfoVars("CURRENT_Y");
  addInfoVars("LAT_ORIGIN");
  addInfoVars("LONG_ORIGIN");

  lat_origin_ = 0;
  lon_origin_ = 0;
  got_lat_origin = false;
  got_lon_origin = false;
  geo_init = false;

}

//-----------------------------------------------------------
// Procedure: onIdleState
//      Note: This function overrides the onIdleState() virtual
//            function defined for the IvPBehavior superclass
//            This function will be executed by the helm each
//            time the behavior FAILS to meet its run conditions.

void BHV_RaceTrack::onIdleState()
{
  bool ok,ok1;
  // Do your thing here
  first_iteration      = true;
  distance_traveled    = 0;

  postErasablePoint();
  postErasablePolygon();
}

//-----------------------------------------------------------
// Procedure: onCompleteState
//      Note: Invoked once by helm prior to behavior deletion.

void BHV_RaceTrack::onCompleteState()
{
  postErasablePoint();
  postErasablePolygon();
}



//-----------------------------------------------------------
// Procedure: setParam

bool BHV_RaceTrack::setParam(string g_param, string g_val) 
{
  if(IvPBehavior::setParamCommon(g_param, g_val))
    return(true);

  g_val = stripBlankEnds(g_val);

  if(g_param == "track_length") 
    {
      double dval = atof(g_val.c_str());
      if((dval <= 0) || (!isNumber(g_val)))
	return(false);
      track_length = dval;
	
      return(true);
    } 
  else if(g_param == "track_width") 
    {
      double dval = atof(g_val.c_str());
      if((dval < 0) || (!isNumber(g_val)))
	return(false);
      track_width = dval;
      return(true);
    }
  if(g_param == "distance_tolerance") 
    {
      double dval = atof(g_val.c_str());
      if((dval <= 0) || (!isNumber(g_val)))
	return(false);
      dist_tolerance = dval;
	
      return(true);
    } 
  if(g_param == "approach_angle") 
    {
      double dval = atof(g_val.c_str());
      if((dval <= 0) || (!isNumber(g_val)))
	return(false);
      approach_angle = dval;
	
      return(true);
    } 
  if(g_param == "speed") 
    {
      double dval = atof(g_val.c_str());
      if((dval < 0) || (!isNumber(g_val)))
	return(false);
      speed = dval;
	
      return(true);
    } 
  else if(g_param == "heading") 
    {
      double dval = atof(g_val.c_str());
      if((dval < 0) || (!isNumber(g_val)))
	return(false);
      heading = angle360(dval);
      return(true);
    }
  else if(g_param == "peak_width") 
    {
      double dval = atof(g_val.c_str());
      if((dval < 0) || (dval > 180) || (!isNumber(g_val)))
	return(false);
      peak_width = dval;
      return(true);
    }
  else if(g_param == "summit_delta") 
    {
      double dval = atof(g_val.c_str());
      if((dval < 0) || (dval > 100) || (!isNumber(g_val)))
	return(false);
      summit_delta = dval;
      return(true);
    }
  else if(g_param == "points") {
    m_deploy_location  = g_val;
    m_deploy_pending = true;
    first_iteration = true;
    return(true);
  }  
  else if(g_param == "clockwise") 
    {
      g_val = tolower(g_val);
      if((g_val!="true")&&(g_val!="false"))
	return(false);
      m_clockwise = (g_val == "true");
      return(true);
    }  
  else if(g_param == "current_compensator") 
    {
      g_val = tolower(g_val);
      if((g_val!="on")&&(g_val!="off"))
	return(false);
      m_current_compensator = (g_val == "on");
      return(true);
    }  
  
  return(false);
}


//-----------------------------------------------------------
// Procedure: onRunState

IvPFunction *BHV_RaceTrack::onRunState() 
{
  bool   ok,ok1,ok2;
  double curr_x = getBufferDoubleVal("NAV_X", ok);
  double curr_y = getBufferDoubleVal("NAV_Y", ok1);
  double dir;

  double curr_time = getBufferCurrTime();

  heading_out = heading;
  heading_in = angle360(heading_out + 180.0);

  // Check for UTM update
  double lat_origin=getBufferDoubleVal("LAT_ORIGIN",got_lat_origin);
  double lon_origin=getBufferDoubleVal("LONG_ORIGIN",got_lon_origin);
  if (got_lat_origin && got_lon_origin)
    {
      if (lat_origin != lat_origin_ || lon_origin != lon_origin_)
	{
	  lat_origin_ = lat_origin;
	  lon_origin_ = lon_origin;
	  m_Geodesy.Initialise(lat_origin_,lon_origin_);
	  geo_init = true;
	  // If datum update, deploy_lat/long must be invariant
	  if (!first_iteration)
	    m_Geodesy.LatLong2LocalUTM(deploy_lat_out,deploy_lon_out,
					 deploy_y_out,deploy_x_out); 
	}
    }
  

  //  deploy_x_out = deploy_x;
  //  deploy_y_out = deploy_y;
  if (geo_init && first_iteration)
    updateStart();

  head_angle = headingToRadians(heading_out); 

  head_x = cos(head_angle);
  head_y = sin(head_angle);
  
  // postMessage("RTRK_LEG_HEADING",heading_out);
  // postMessage("RTRK_HEAD_ANGLE",head_angle);
  // postMessage("RTRK_HEAD_X",head_x);
  // postMessage("RTRK_HEAD_Y",head_y);
  
  if (m_clockwise)
    dir = -1;
  else
    dir =1;

  end_x_out = deploy_x_out +  track_length * head_x; 
  end_y_out = deploy_y_out +  track_length * head_y;
  deploy_x_in = end_x_out - dir * track_width * head_y;
  deploy_y_in = end_y_out + dir * track_width * head_x;
  end_x_in = deploy_x_out - dir * track_width * head_y;
  end_y_in = deploy_y_out + dir * track_width * head_x;
  
  // Prepare for viewable polygon
  string polstr = doubleToString(deploy_x_out,1)+","+doubleToString(deploy_y_out,1)+":"
    + doubleToString(end_x_out,1)+","+doubleToString(end_y_out,1)+":" 
    + doubleToString(deploy_x_in,1)+","+doubleToString(deploy_y_in,1)+":"
    + doubleToString(end_x_in,1)+","+doubleToString(end_y_in,1) ;
  //      postMessage("RACETRACK",polstr);
  XYPolygon new_poly = string2Poly(polstr);
  //      if(!new_poly.is_convex())  // Should be convex - false otherwise
  //	return(false);
  new_poly.apply_snap(0.1); // snap to tenth of meter
  m_waypoint_engine.setSegList(new_poly);
  
  if(first_iteration) 
    {
      // define track waypoints starting at current position
      first_iteration = false;
      bhv_start = curr_time;

      deploy_x = curr_x;
      deploy_y = curr_y;
      distance_traveled = 0;
      leg = 0;
     }  
  else if (leg > 0)
    {
      // Projected distance in desired heading direction
      distance_traveled = (curr_x - deploy_x_out)*head_x + (curr_y - deploy_y_out)*head_y;
      // postMessage("RTRK_DEPLOY_X",deploy_x_out);
      // postMessage("RTRK_DEPLOY_Y",deploy_y_out);
      // postMessage("RTRK_END_X",end_x_out);
      // postMessage("RTRK_END_Y",end_y_out);
    }
  else 
    {
      // Projected distance in desired heading direction
      distance_traveled = (deploy_x_in -  curr_x)*head_x + (deploy_y_in - curr_y)*head_y;
      // postMessage("RTRK_DEPLOY_X",deploy_x_in);
      // postMessage("RTRK_DEPLOY_Y",deploy_y_in);
      // postMessage("RTRK_END_X",end_x_in);
      // postMessage("RTRK_END_Y",end_y_in);
    }


  if (distance_traveled > track_length)
    {
	leg = -leg;
    }


  if (distance_traveled < 0.0)
    {
      m_in_track = false;
    }
  else
    {
      m_in_track = true;
    }


  // Determine heading for trackline to other end
  double diff, corr;
  double fac = 5;

  if (leg == 0 || !m_in_track)
    {
      double dist_out = sqrt(pow(curr_x-deploy_x_out,2) + pow(curr_y-deploy_y_out,2));
      double dist_in = sqrt(pow(curr_x-deploy_x_in,2) + pow(curr_y-deploy_y_in,2));
      if (dist_out <= dist_in)
	{
	  leg = 1;

	  new_heading = radToHeading(atan2(deploy_y_out - curr_y,deploy_x_out - curr_x));
	  m_ptx = deploy_x_out;
	  m_pty = deploy_y_out;
	}      
      else
	{
	  leg = -1;

	  new_heading = radToHeading(atan2(deploy_y_in - curr_y,deploy_x_in - curr_x));
          m_ptx = deploy_x_in;
          m_pty = deploy_y_in;
	}
      m_on_trackline = false;
    }
  else if (leg > 0)
    {
      bearing = radToHeading(atan2(end_y_out - curr_y,end_x_out - curr_x));
      diff = angle180(angle360(bearing - heading_out));
      double dist = fabs(tan(M_PI/180.0 * diff)) * (track_length-distance_traveled);
      if ( fabs(diff)*exp((fac-1)*fabs(dist/dist_tolerance)) < approach_angle)
	{
	  corr = diff * exp( (fac-1) * fabs(dist/dist_tolerance) );
	  m_on_trackline = true;
	}
      else 
	{
	  if ( diff < 0 )
	    corr = - approach_angle;
	  else
	    corr = approach_angle;

	  m_on_trackline = false;
	}
      new_heading = angle360(heading_out + corr);
      m_ptx = end_x_out;
      m_pty = end_y_out;
    }
  else
    {
      bearing = radToHeading(atan2(end_y_in - curr_y,end_x_in - curr_x));
      diff = angle180(angle360(bearing - heading_in));
      double dist = fabs(tan(M_PI/180.0 * diff)) * (track_length-distance_traveled);
      if ( fabs(diff)*exp((fac-1)*fabs(dist/dist_tolerance)) < approach_angle)
	{
	  corr = diff * exp((fac-1)*fabs(dist/dist_tolerance));
	  m_on_trackline = true;
	}
      else 
	{
	  if ( diff < 0 )
	    corr = - approach_angle;
	  else
	    corr = approach_angle;
	  
	  m_on_trackline = false;
	}
      new_heading = angle360(heading_in + corr);
      m_ptx = end_x_in;
      m_pty = end_y_in;
    }


  if (m_in_track != m_in_track_posted)
    {
      if (m_in_track)
	postMessage("IN_TRACK","TRUE");
      else
	postMessage("IN_TRACK","FALSE");
      m_in_track_posted = m_in_track;
    }


  if (m_on_trackline != m_on_trackline_posted)
    {
      if (m_on_trackline)
        postMessage("ON_TRACKLINE","TRUE");
      else
        postMessage("ON_TRACKLINE","FALSE");
      m_on_trackline_posted = m_on_trackline;
    }

        postMessage("DIST_TRAVELED",distance_traveled);
        postMessage("RACETRACK_LEG",double(leg));
  //    postMessage("RTRK_BEARING",bearing);
  //    postMessage("RTRK_DIFF",diff);
  //    postMessage("RTRK_NEW_HEADING",new_heading);
  
	if (m_current_compensator)
	  {
	    new_heading = currentCompensatedHeading(new_heading, speed);
            postMessage("COMPENSATED_HEADING",new_heading);
	  }

	ZAIC_PEAK zaic_heading(m_domain,"course");
	zaic_heading.setSummit(new_heading);
	zaic_heading.setValueWrap(true);
	zaic_heading.setPeakWidth(peak_width);
	zaic_heading.setBaseWidth(180.0 - peak_width);
	zaic_heading.setSummitDelta(summit_delta);
	
	IvPFunction *ipf_heading = zaic_heading.extractOF();
	
	ZAIC_PEAK zaic_speed(m_domain, "speed");
	zaic_speed.setSummit(speed);
	zaic_speed.setSummitDelta(100.0);
	zaic_speed.setPeakWidth(0.5);
	zaic_speed.setMinMaxUtil(0, 100);
	
	IvPFunction *ipf_speed = zaic_speed.extractOF();
	
	OF_Coupler coupler;
	IvPFunction *new_ipf = coupler.couple(ipf_speed, ipf_heading);
	
	new_ipf->setPWT(m_priority_wt);

	// wait 2 seconds before showing view, to allow previous behaviors to die first	
	if (curr_time - bhv_start > 2.0)
	  {
	    postViewablePolygon();
	    postViewablePoint();
	  }

	return(new_ipf);
	
}

void BHV_RaceTrack::updateStart()
{
  bool ok,ok1;
  if (!m_deploy_pending)
    {
      m_deploy_location  = "present_position";
    }

  m_deploy_location = tolower(m_deploy_location);

  if(m_deploy_location == "present_position") 
    {
      deploy_x_out = getBufferDoubleVal("NAV_X", ok);
      deploy_y_out = getBufferDoubleVal("NAV_Y", ok1);
      
    }
  else 
    {
      vector<string> svector = parseString(m_deploy_location, ',');
      if(svector.size() == 2) 
	{
	  if(isNumber(svector[0]) && isNumber(svector[1]))
	    svector[0] = stripBlankEnds(svector[0]);
	  svector[1] = stripBlankEnds(svector[1]);
	  double xval = atof(svector[0].c_str());
	  double yval = atof(svector[1].c_str());
	  deploy_x_out = xval;
	  deploy_y_out = yval;
	}
    }
  m_Geodesy.UTM2LatLong(deploy_x_out,deploy_y_out,deploy_lat_out,deploy_lon_out); 

  double tmp_x = getBufferDoubleVal("CURRENT_X", ok);
  double tmp_y = getBufferDoubleVal("CURRENT_Y", ok1);
  if (ok && ok1)
    {
      north_current_spd = tmp_y;
      east_current_spd = tmp_x;
    }
  
  m_deploy_location = "";
}

double BHV_RaceTrack::currentCompensatedHeading(double desired_heading, double u)
{
  double theta;
  double phi = headingToRadians(desired_heading); 

  double b = -2*(east_current_spd*cos(phi) + north_current_spd*sin(phi));
  double c = east_current_spd * east_current_spd + north_current_spd * north_current_spd - u * u;
  
  double lambda = 0.0;
  
  double D = b*b - 4*c;
  if(D >= 0)
    lambda = (-1*b+sqrt(D))/2;
  else
    lambda = -1;
  
  if(lambda < 0)
    {
      //The flow is too strong for the glider
	theta = atan2(north_current_spd, east_current_spd) + M_PI;
    }
  else
    {
      if(lambda*cos(phi) - east_current_spd != 0.0)
	theta = atan2(lambda*sin(phi)-north_current_spd,lambda*cos(phi)-east_current_spd);
    }
  return(radToHeading(theta));
}

//-----------------------------------------------------------
// Procedure: postViewablePolygon()
//      Note: Even if the polygon is posted on each iteration, the
//            helm will filter out unnecessary duplicate posts.

void BHV_RaceTrack::postViewablePolygon()
{
  XYSegList seglist = m_waypoint_engine.getSegList();
  seglist.set_color("vertex",m_hint_vertex_color);
  seglist.set_color("edge",m_hint_edge_color);
  seglist.set_edge_size(m_hint_edge_size);
  seglist.set_vertex_size(m_hint_vertex_size);
  // Handle the label setting
  string bhv_tag = tolower(getDescriptor());
  bhv_tag = m_us_name + "_" + bhv_tag;
  seglist.set_label(bhv_tag);
  if(m_hint_poly_label == "")
    seglist.set_label(bhv_tag);
  else
    seglist.set_label(m_hint_poly_label);

  string poly_spec = seglist.get_spec();
  postMessage("VIEW_POLYGON", poly_spec);
}

//-----------------------------------------------------------
// Procedure: postErasablePolygon()
//      Note: Even if the polygon is posted on each iteration, the
//            helm will filter out unnecessary duplicate posts.

void BHV_RaceTrack::postErasablePolygon()
{
  XYSegList seglist = m_waypoint_engine.getSegList();
  string bhv_tag = tolower(getDescriptor());
  bhv_tag = findReplace(bhv_tag, "(d)", "");
  bhv_tag = m_us_name + "_" + bhv_tag;
  seglist.set_label(bhv_tag);
  seglist.set_active(false);

  string null_poly_spec = seglist.get_spec();
  postMessage("VIEW_POLYGON", null_poly_spec);
}

//-----------------------------------------------------------
// Procedure: postViewablePoint()

void BHV_RaceTrack::postViewablePoint()
{
  string bhv_tag = tolower(getDescriptor());

  XYPoint view_point(m_ptx, m_pty);
  //  view_point.set_label(m_us_name + "'s next waypoint");
  view_point.set_label(m_us_name + "_waypoint");
  view_point.set_type("waypoint");
  view_point.set_source(m_us_name + "_" + bhv_tag);
  view_point.set_color("label",m_hint_nextpt_lcolor);
  view_point.set_color("vertex",m_hint_nextpt_color);
  postMessage("VIEW_POINT", view_point.get_spec());
}


//-----------------------------------------------------------
// Procedure: postErasablePoint()

void BHV_RaceTrack::postErasablePoint()
{
  string bhv_tag = tolower(getDescriptor());

  XYPoint view_point(m_ptx, m_pty);
  view_point.set_label(m_us_name + "_waypoint");
  view_point.set_type("waypoint");
  view_point.set_source(m_us_name + "_" + bhv_tag);
  view_point.set_active(false);
  postMessage("VIEW_POINT", view_point.get_spec());
}


//-----------------------------------------------------------
// Procedure: handleVisualHint()

void BHV_RaceTrack::handleVisualHint(string hint)
{
  string param = tolower(stripBlankEnds(biteString(hint, '=')));
  string value = stripBlankEnds(hint);
  
  if((param == "nextpt_color") && isColor(value))
    m_hint_nextpt_color = value;
  else if((param == "nextpt_lcolor") && isColor(value))
    m_hint_nextpt_lcolor = value;
  else if((param == "vertex_color") && isColor(value))
    m_hint_vertex_color = value;
  else if((param == "edge_color") && isColor(value))
    m_hint_edge_color = value;
  else if((param == "edge_size") && isNumber(value))
    m_hint_edge_size = atof(value.c_str());
  else if((param == "vertex_size") && isNumber(value))
    m_hint_vertex_size = atof(value.c_str());
  else if(param == "label")
    m_hint_poly_label = value;
}
