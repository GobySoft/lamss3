/*****************************************************************/
/*    NAME: Henrik Schmidt, Toby Schneider                       */
/*    ORGN: MIT Cambridge MA                                     */
/*    FILE: ViewMessage.cpp                                      */
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

#include "ViewMessage.h"
#include <sstream>
#include <limits>
#include "goby/util/debug_logger.h"

using goby::glog;

// name of the PointType enumeration field in view_message.proto
const std::string CViewMessage::TYPE_FIELD_NAME = "type";
// name of the MissionType enumeration field in view_message.proto
const std::string CViewMessage::MISSION_FIELD_NAME = "mission";

CViewMessage* CViewMessage::inst_ = 0;
lamss::protobuf::ViewMessageConfig CViewMessage::cfg_;

CViewMessage* CViewMessage::get_instance()
{
    if(!inst_)
        inst_ = new CViewMessage();
    return inst_;
}


CViewMessage::CViewMessage() :
    goby::moos::GobyMOOSApp(&cfg_),    
    ownship_("unknown"),
    mission_state_("unknown"),
    destination_(cfg_.destination())
{
    goby::moos::GobyMOOSApp::set_ignore_stale(false);

    if(cfg_.generate_view_objects())
    {
        subscribe("VIEW_POINT", &CViewMessage::handle_view_point, this);
        subscribe("VIEW_POLYGON", &CViewMessage::handle_view_polygon, this);
        subscribe("VIEW_SEGLIST", &CViewMessage::handle_view_seglist, this);
    }
    
    subscribe("VEHICLE_NAME", &CViewMessage::handle_vehicle_name, this);
    subscribe("MISSION_STATE", &CViewMessage::handle_mission_state, this);

    subscribe("IN_VIEW_SEGLIST", &CViewMessage::handle_in_view_seglist, this);
    subscribe("IN_VIEW_POLYGON", &CViewMessage::handle_in_view_polygon, this);
    subscribe("IN_VIEW_POINT", &CViewMessage::handle_in_view_point, this);

    subscribe_pb("ICEX_VEHICLE_TRANSMIT_SELECTION_IN", &CViewMessage::handle_icex_transmit_selection, this);

    subscribe("LAT_ORIGIN", &CViewMessage::handle_lat_origin, this);
    subscribe("LONG_ORIGIN", &CViewMessage::handle_lon_origin, this);

    geodesy_.Initialise(cfg_.common().lat_origin(),cfg_.common().lon_origin());
    // Dynamic UTM. H. Schmidt 8/9/21
    new_origin_ = false;
}

void CViewMessage::loop()
{
  if (new_origin_)
    {
      geodesy_.Initialise(lat_origin_, lon_origin_);
      new_origin_ = false;
    }

}


void CViewMessage::handle_lat_origin(const CMOOSMsg& msg)
{
 double new_lat = msg.GetDouble();
 if (!isnan(new_lat))
   {
     lat_origin_ = new_lat;
     new_origin_ = true;
   }
}

void CViewMessage::handle_lon_origin(const CMOOSMsg& msg)
{
 double new_lon = msg.GetDouble();
 if (!isnan(new_lon))
   {
     lon_origin_ = new_lon;
     new_origin_ = true;
   }
}

void CViewMessage::handle_view_point(const CMOOSMsg& msg)
{
    if(msg.GetSource() == GetAppName())
        return;

    std::string view_point = msg.m_sVal;
    glog << "\ngot VIEW_POINT" << std::endl;
    XYPoint point = string2Point(view_point);

    if(point.get_label().find("waypoint") == std::string::npos)
        return;
    
    glog << "XYPoint:" <<  point.get_spec() << std::endl;
    lamss::protobuf::ViewPoint proto_point = to_protobuf(point);

    glog << "lamss.protobuf.ViewPoint: " << proto_point.ShortDebugString() << std::endl;
    std::string goby1_moos_str;

    to_moos_string(&goby1_moos_str, proto_point);

    glog << "as key=value, string: " << goby1_moos_str << std::endl;            

    if(proto_point.active())
      {
	publish("OUT_VIEW_POINT", goby1_moos_str);
      }
}

void CViewMessage::handle_in_view_point(const CMOOSMsg& msg)
{
    lamss::protobuf::ViewPoint proto_point;

    from_moos_string(msg.GetString(), &proto_point);
    glog << "back to proto: " << proto_point.ShortDebugString() << std::endl;


    std::string xypoint_str = from_protobuf(proto_point).get_spec();
    glog << "back to XYPoint:" << xypoint_str  << std::endl;

    publish("VIEW_POINT", xypoint_str);
    // Create OLD_VIEW_POINT for GEO messages

    std::stringstream cc;
    std::string type = "UNKNOWN";
    if (proto_point.type() == lamss::protobuf::ViewPoint::WAYPOINT)
      type = "WAYPOINT";
    else if (proto_point.type() == lamss::protobuf::ViewPoint::TRAILPOINT)
      type = "TRAILPOINT";
    else if (proto_point.type() == lamss::protobuf::ViewPoint::ATTRACTOR)
      type = "ATTRACTOR";
    else if (proto_point.type() == lamss::protobuf::ViewPoint::STATION)
      type = "STATION";

    double x,y;
    geodesy_.LatLong2LocalUTM(proto_point.point().y(),
			      proto_point.point().x(),
			      y,x);
    std::string bl ="false";
    if (proto_point.active())
      bl = "true";
    cc << "active," << bl << ":"
       << "label," << proto_point.vehicle_name() << "_" << type << ":"
       << "type," << type << ":"
       << "source," << proto_point.vehicle_name() << "_" << type << ":"
       << x << "," << y ;
    publish("OLD_VIEW_POINT", cc.str());

}

void CViewMessage::handle_view_polygon(const CMOOSMsg& msg)
{
    if(msg.GetSource() == GetAppName())
        return;

    std::string view_poly = msg.m_sVal;
    glog << "\ngot VIEW_POLYGON" << std::endl;
    MOOSTrace(view_poly + "\n\n");
    XYPolygon poly = string2Poly(view_poly);
    process_seglist(poly, true);
}

void CViewMessage::handle_view_seglist(const CMOOSMsg& msg)
{
    if(msg.GetSource() == GetAppName())
        return;

    std::string view_segl = msg.m_sVal;
    glog << "\ngot VIEW_SEGLIST" << std::endl;
    XYSegList seglist = string2SegList(view_segl);
      
    process_seglist(seglist);
}

void CViewMessage::process_seglist(const XYSegList& seglist, bool is_polygon)
{
    glog << "XYSegList:" <<  seglist.get_spec() << std::endl;
    lamss::protobuf::ViewSegList proto_seglist ;
    unsigned int max_pnts = 13;

    if (seglist.size() > max_pnts)
    {
	// if more than 9 points plot surrounding polygon
	proto_seglist = convex_polygon(seglist);
	is_polygon = true;
    }
    else
    {
        proto_seglist= to_protobuf(seglist);
    }
    
    if(seglist.get_label() == "OpRegion")
    {
      proto_seglist.set_polytype(lamss::protobuf::ViewSegList::OPBOX);
    }
    else if(seglist.get_label().find("stvp") != std::string::npos)
    {
      proto_seglist.set_polytype(lamss::protobuf::ViewSegList::OBSTACLE);
    }
    else
    {
        proto_seglist.set_polytype(lamss::protobuf::ViewSegList::NORMAL);
    }
    
    glog << "lamss.protobuf.ViewSegList: " << proto_seglist.ShortDebugString() << std::endl;

    std::string goby1_moos_str;

    to_moos_string(&goby1_moos_str, proto_seglist);

    glog << "as key=value, string: " << goby1_moos_str << std::endl;

    if(proto_seglist.active() && proto_seglist.point_size() != 0)
        publish(is_polygon ? "OUT_VIEW_POLYGON" : "OUT_VIEW_SEGLIST",
		goby1_moos_str);    
}

void CViewMessage::handle_in_view_seglist(const CMOOSMsg& msg)
{
    process_in_seglist(msg.GetString());    
}


void CViewMessage::handle_in_view_polygon(const CMOOSMsg& msg)
{
    process_in_seglist(msg.GetString(), true);
}


void CViewMessage::process_in_seglist(const std::string& goby1_str, bool is_polygon)
{
    lamss::protobuf::ViewSegList proto_seglist;
    from_moos_string(goby1_str, &proto_seglist);
    
    glog << "back to proto: " << proto_seglist.ShortDebugString() << std::endl;

    std::string xyseglist_str = from_protobuf(proto_seglist).get_spec();
    glog << "back to XYSegList:" <<  xyseglist_str << std::endl; 

    publish(is_polygon ? "VIEW_POLYGON" : "VIEW_SEGLIST",
	    xyseglist_str);

    std::stringstream cc;
    std::string mission = "UNKNOWN";
    if (proto_seglist.mission() == lamss::protobuf::ViewSegList::DEPLOY)
      mission = "DEPLOY";
    else if (proto_seglist.mission() == lamss::protobuf::ViewSegList::PROSECUTE)
      mission = "PROSECUTE";
    else if (proto_seglist.mission() == lamss::protobuf::ViewSegList::RETURN)
      mission = "RETURN";
    else if (proto_seglist.mission() == lamss::protobuf::ViewSegList::TRANSIT)
      mission = "TRANSIT";

    std::string polytype = "NORMAL";
    if (proto_seglist.polytype() == lamss::protobuf::ViewSegList::OPBOX)
      polytype = "OPBOX";
    else if (proto_seglist.polytype() == lamss::protobuf::ViewSegList::OBSTACLE)
      polytype = "OBSTACLE";
    
    std::string bl ="false";
    if (proto_seglist.active())
      bl = "true";
    cc << "active," << bl << ":"
       << "label," << proto_seglist.vehicle_name() << "_" << mission
       << "-" << polytype << ":"
       << "edge_size,0" << ":"
       << "vertex_size,0" ;
    for (int i=0; i< proto_seglist.point_size(); i++)
      {
	if (!isnan(proto_seglist.point(i).x()) &&
	    !isnan(proto_seglist.point(i).y()))
	  {
	    double x,y;
	    geodesy_.LatLong2LocalUTM(proto_seglist.point(i).y(),
				      proto_seglist.point(i).x(),y,x);
	    cc << ":" << x << "," << y ;
	  }
      }
    if (is_polygon &&
	!isnan(proto_seglist.point(0).x()) &&
	!isnan(proto_seglist.point(0).y()))
      {
	double x,y;
	geodesy_.LatLong2LocalUTM(proto_seglist.point(0).y(),
				  proto_seglist.point(0).x(),y,x);
	cc << ":" << x << "," << y ;
      }
       publish(is_polygon ? "OLD_VIEW_POLYGON" : "OLD_VIEW_SEGLIST",
	    cc.str());

}



void CViewMessage::handle_vehicle_name(const CMOOSMsg& msg)
{
    ownship_ = msg.GetString();
    boost::to_lower(ownship_);
}

void CViewMessage::handle_mission_state(const CMOOSMsg& msg)
{
    mission_state_ = msg.GetString();
}

lamss::protobuf::ViewPoint CViewMessage::to_protobuf(const XYPoint& point)
{
    lamss::protobuf::ViewPoint out;

    out.set_vehicle_name(ownship_);
    out.set_destination(destination_);

    out.set_active(point.active());

    double lat, lon;
    geodesy_.UTM2LatLong(point.x(),point.y(),lat,lon);
    out.mutable_point()->set_x(lon);
    out.mutable_point()->set_y(lat);
    //endif

    out.mutable_point()->set_z(point.z());

    const google::protobuf::EnumDescriptor* enum_desc =
        lamss::protobuf::ViewPoint::PointType_descriptor();

    const google::protobuf::EnumValueDescriptor* enum_value = enum_desc->FindValueByName(boost::to_upper_copy(point.get_type()));
    const google::protobuf::FieldDescriptor* enum_field = out.GetDescriptor()->FindFieldByName(TYPE_FIELD_NAME);
    
    if(enum_value && enum_field)
        out.GetReflection()->SetEnum(&out, enum_field, enum_value);

    return out;
}

XYPoint CViewMessage::from_protobuf(const lamss::protobuf::ViewPoint& point)
{
    XYPoint out;

    double x,y;
    geodesy_.LatLong2LocalUTM(point.point().y(), point.point().x(),y,x);
    out.set_vertex(x, y, point.point().z());

    out.set_active(point.active());
    out.set_type(point.GetReflection()->GetEnum(point, point.GetDescriptor()->FindFieldByName(TYPE_FIELD_NAME))->name());
    out.set_label(point.vehicle_name() + "_" + out.get_type());
    return out;
}

lamss::protobuf::ViewSegList CViewMessage::convex_polygon(const XYSegList& seglist)
{
   lamss::protobuf::ViewSegList out;

   out.set_vehicle_name(ownship_);

    out.set_destination(destination_);

    glog << "Seglist size is: " << seglist.size() << std::endl;
    std::vector<int> indx;
    indx.clear();
    // find smallest x point
    double min_x = 1e200;
    int min_indx = 0;

    for (unsigned int i=0; i< seglist.size(); i++)
      if (seglist.get_vx(i) < min_x)
	{
	  min_x = seglist.get_vx(i);
	  min_indx = i;
	}
    lamss::protobuf::Point* point = out.add_point();

    double lat, lon;
    geodesy_.UTM2LatLong(seglist.get_vx(min_indx),seglist.get_vy(min_indx),lat,lon);
    point->set_x(lon);
    point->set_y(lat);

    point->set_z(seglist.get_vz(min_indx));
    indx.push_back(min_indx);
    false && glog << "convex_polygon: Lefmost vertex: x,y = "
	 << seglist.get_vx(min_indx) << ", "
	 << seglist.get_vy(min_indx) << std::endl;
    double prev_bearing = 0;
    bool done = false;
    while (!done)
      {
	unsigned int curr_indx = indx[indx.size()-1];
	// find point with minimum positive bearing change
	//double vert_x = seeglist.get_vx(min_indx);
	//double vert_y glist.size(); i++)
	double len_max = 0;
	double delta_min = 2*M_PI;
	double bearing = 0;
	min_indx = 0;
	for (unsigned int i=0; i<seglist.size(); i++)
	  if (i != curr_indx)
	    {
	      double ang = atan2(seglist.get_vx(i)-seglist.get_vx(curr_indx),
				 seglist.get_vy(i)-seglist.get_vy(curr_indx));
	     
	      double delta = ang - prev_bearing;
	      if (delta < 0)
		delta += 2*M_PI;
	      if (delta >= 2*M_PI)
		delta -= 2*M_PI;
	      if (delta <= delta_min+0.01)
		{
		  if (delta < delta_min-0.01)
		    len_max = 0;
		    
		  double len =
		    sqrt(pow(seglist.get_vx(i)-seglist.get_vx(curr_indx),2)+
			 pow(seglist.get_vy(i)-seglist.get_vy(curr_indx),2));
		  false && glog << "curr_indx, i, x, y, delta, bearing: "
		       << curr_indx << ", "
		       << i << ", "
		       << seglist.get_vx(i) << ", "
		       << seglist.get_vy(i) << ", "
		       << len << ", "
		       << len_max << ", "
		       << delta*180/M_PI << ", "
		       << (delta+prev_bearing)*180/M_PI << std::endl;
		  if (len > len_max)
		    {
		      len_max = len;
		      min_indx = i;
		      delta_min = delta;
		      bearing = prev_bearing + delta;
		      false && glog << "min_indx= " << min_indx << std::endl;
		    }
		}
	    }
	if (min_indx != indx[0] && indx.size() < 9)
	  {
	    // set the min_indx point as vertex
	    point = out.add_point();

	    double lat, lon;
	    geodesy_.UTM2LatLong(seglist.get_vx(min_indx),seglist.get_vy(min_indx),
				 lat,lon);
	    point->set_x(lon);
	    point->set_y(lat);

	    point->set_z(seglist.get_vz(min_indx));
	    indx.push_back(min_indx);
	    prev_bearing = bearing;
	    glog << "convex_polygon: Adding to vertex: x,y = "
		 << seglist.get_vx(min_indx) << ", "
		 << seglist.get_vy(min_indx) << std::endl;
	  }
	else
	  done = true;
      }

    const google::protobuf::EnumDescriptor* enum_desc =
        lamss::protobuf::ViewSegList::MissionType_descriptor();

    //std::string::size_type start_vehicle_name = seglist.get_label().find(boost::to_lower_copy(ownship_));
    // std::string mission_state = (start_vehicle_name == std::string::npos) ?
    //     "unknown" :
    //     seglist.get_label().substr(start_vehicle_name + ownship_.size() + 1);
    
    boost::to_upper(mission_state_);

    glog << "Mission_State is: [" << mission_state_ << "]" <<  std::endl;
    
    const google::protobuf::EnumValueDescriptor* enum_value = enum_desc->FindValueByName(mission_state_);
    const google::protobuf::FieldDescriptor* enum_field = out.GetDescriptor()->FindFieldByName(MISSION_FIELD_NAME);
    if(enum_value && enum_field)
        out.GetReflection()->SetEnum(&out, enum_field, enum_value);
    
    out.set_active(seglist.active());
    return(out);
 }

lamss::protobuf::ViewSegList CViewMessage::to_protobuf(const XYSegList& seglist)
{
    lamss::protobuf::ViewSegList out;

    out.set_vehicle_name(ownship_);

    out.set_destination(destination_);

    glog << "Seglist size is: " << seglist.size() << std::endl;
    int seglist_inc = 1;

    unsigned int max_pnt = 13;

    if (seglist.size() > max_pnt)
      seglist_inc = int(seglist.size()/(max_pnt+1))+1;
    
    for(unsigned i = 0, n = seglist.size(); i < n; ++i)
      if (i%seglist_inc == 0)
	{
	  lamss::protobuf::Point* point = out.add_point();

	  double lat, lon;
	  geodesy_.UTM2LatLong(seglist.get_vx(i),seglist.get_vy(i),
			       lat,lon);
	  point->set_x(lon);
	  point->set_y(lat);
	  point->set_z(seglist.get_vz(i));
	}

    const google::protobuf::EnumDescriptor* enum_desc =
        lamss::protobuf::ViewSegList::MissionType_descriptor();

    boost::to_upper(mission_state_);

    glog << "Mission_State is: [" << mission_state_ << "]" <<  std::endl;
    
    const google::protobuf::EnumValueDescriptor* enum_value =
      enum_desc->FindValueByName(mission_state_);
    const google::protobuf::FieldDescriptor* enum_field =
      out.GetDescriptor()->FindFieldByName(MISSION_FIELD_NAME);
    if(enum_value && enum_field)
        out.GetReflection()->SetEnum(&out, enum_field, enum_value);
    
    out.set_active(seglist.active());
    return out;
}

XYSegList CViewMessage::from_protobuf
  (const lamss::protobuf::ViewSegList& seglist)
{
  XYSegList out;
  
  for(int i = 0, n = seglist.point_size(); i < n; ++i)
    {
      glog << "seglist.point(i).x() == NaN" << std::boolalpha << is_nan(seglist.point(i).x()) << std::endl;
      
      if(!is_nan(seglist.point(i).x()) &&
	 !is_nan(seglist.point(i).y()) &&
	 !is_nan(seglist.point(i).z()))
        {
	  double x, y;
	  geodesy_.LatLong2LocalUTM(seglist.point(i).y(),seglist.point(i).x(),y,x);
	  glog << "adding x: " << x << ", y: " << ", z: "
	       << seglist.point(i).z() << std::endl;      
	  
	  out.add_vertex(x, y, seglist.point(i).z());
        }
    }
  
  out.set_active(seglist.active());
  out.set_label(seglist.vehicle_name() + "_" + seglist.GetReflection()->GetEnum(seglist, seglist.GetDescriptor()->FindFieldByName(MISSION_FIELD_NAME))->name());
  out.set_edge_size(1);
  return out;
}

void CViewMessage::handle_icex_transmit_selection(const lamss::icex::protobuf::VehicleTransmitSelection& selection)
{
    switch(selection.modem())
    {
        case lamss::icex::protobuf::VehicleTransmitSelection::TRANSMIT_10KHZ:
            destination_ = cfg_.destination();
            break;
        case lamss::icex::protobuf::VehicleTransmitSelection::TRANSMIT_25KHZ:
            destination_ = cfg_.icex_25khz_id();
            break;
            
    }
    
}
    
