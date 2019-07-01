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
    mission_state_("unknown")
{
    goby::moos::GobyMOOSApp::set_ignore_stale(false);
    
    subscribe("VIEW_POINT", &CViewMessage::handle_view_point, this);
    subscribe("VIEW_POLYGON", &CViewMessage::handle_view_polygon, this);
    subscribe("VIEW_SEGLIST", &CViewMessage::handle_view_seglist, this);
    subscribe("VEHICLE_NAME", &CViewMessage::handle_vehicle_name, this);
    subscribe("MISSION_STATE", &CViewMessage::handle_mission_state, this);


    subscribe("IN_VIEW_SEGLIST", &CViewMessage::handle_in_view_seglist, this);
    subscribe("IN_VIEW_POLYGON", &CViewMessage::handle_in_view_polygon, this);
    subscribe("IN_VIEW_POINT", &CViewMessage::handle_in_view_point, this);
}


void CViewMessage::handle_view_point(const CMOOSMsg& msg)
{
    if(msg.GetSource() == GetAppName())
        return;

    std::string view_point = msg.m_sVal;
    std::cout << "\ngot VIEW_POINT" << std::endl;
    XYPoint point = string2Point(view_point);
    std::cout << "XYPoint:" <<  point.get_spec() << std::endl;    
    lamss::protobuf::ViewPoint proto_point = to_protobuf(point);
    std::cout << "lamss.protobuf.ViewPoint: " << proto_point.ShortDebugString() << std::endl;

    std::string goby1_moos_str;

    to_moos_string(&goby1_moos_str, proto_point);

    std::cout << "as key=value, string: " << goby1_moos_str << std::endl;            

    if(proto_point.active())
        publish("OUT_VIEW_POINT", goby1_moos_str);
}

void CViewMessage::handle_in_view_point(const CMOOSMsg& msg)
{

    lamss::protobuf::ViewPoint proto_point;
    from_moos_string(msg.GetString(), &proto_point);
    std::cout << "back to proto: " << proto_point.ShortDebugString() << std::endl;


    std::string xypoint_str = from_protobuf(proto_point).get_spec();
    std::cout << "back to XYPoint:" << xypoint_str  << std::endl;

    publish("VIEW_POINT", xypoint_str);    
}

void CViewMessage::handle_view_polygon(const CMOOSMsg& msg)
{
    if(msg.GetSource() == GetAppName())
        return;

    std::string view_poly = msg.m_sVal;
    std::cout << "\ngot VIEW_POLYGON" << std::endl;
    MOOSTrace(view_poly + "\n\n");
    XYPolygon poly = string2Poly(view_poly);
    process_seglist(poly, true);
}

void CViewMessage::handle_view_seglist(const CMOOSMsg& msg)
{
    if(msg.GetSource() == GetAppName())
        return;

    std::string view_segl = msg.m_sVal;
    std::cout << "\ngot VIEW_SEGLIST" << std::endl;
    XYSegList seglist = string2SegList(view_segl);
    process_seglist(seglist);
}

void CViewMessage::process_seglist(const XYSegList& seglist, bool is_polygon)
{
    std::cout << "XYSegList:" <<  seglist.get_spec() << std::endl;
    lamss::protobuf::ViewSegList proto_seglist = to_protobuf(seglist);
    std::cout << "lamss.protobuf.ViewSegList: " << proto_seglist.ShortDebugString() << std::endl;

    std::string goby1_moos_str;

    to_moos_string(&goby1_moos_str, proto_seglist);

    std::cout << "as key=value, string: " << goby1_moos_str << std::endl;

    if(proto_seglist.active() && proto_seglist.point_size() != 0)
        publish(is_polygon ? "OUT_VIEW_POLYGON" : "OUT_VIEW_SEGLIST", goby1_moos_str);    

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
    
    std::cout << "back to proto: " << proto_seglist.ShortDebugString() << std::endl;

    std::string xyseglist_str = from_protobuf(proto_seglist).get_spec();
    std::cout << "back to XYSegList:" <<  xyseglist_str << std::endl; 

    publish(is_polygon ? "VIEW_POLYGON" : "VIEW_SEGLIST", xyseglist_str);
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
    out.set_destination(1);

    out.set_active(point.active());
    out.mutable_point()->set_x(point.x());
    out.mutable_point()->set_y(point.y());
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
    out.set_vertex(point.point().x(), point.point().y(), point.point().z());
    out.set_active(point.active());
    out.set_type(point.GetReflection()->GetEnum(point, point.GetDescriptor()->FindFieldByName(TYPE_FIELD_NAME))->name());
    out.set_label(point.vehicle_name() + "_" + out.get_type());
    return out;
}


lamss::protobuf::ViewSegList CViewMessage::to_protobuf(const XYSegList& seglist)
{
    lamss::protobuf::ViewSegList out;
    out.set_vehicle_name(ownship_);

    out.set_destination(1);

    std::cout << "Seglist size is: " << seglist.size();
    int seglist_inc = 1;
    if (seglist.size() >= 10)
	seglist_inc = int(seglist.size()/10.0)+1;
  
    for(unsigned i = 0, n = seglist.size(); i < n; ++i)
      if (i%seglist_inc == 0)
	{
	  lamss::protobuf::Point* point = out.add_point();
	  point->set_x(seglist.get_vx(i));
	  point->set_y(seglist.get_vy(i));
	  point->set_z(seglist.get_vz(i));
	}
  
    const google::protobuf::EnumDescriptor* enum_desc =
        lamss::protobuf::ViewSegList::MissionType_descriptor();

    //std::string::size_type start_vehicle_name = seglist.get_label().find(boost::to_lower_copy(ownship_));
    // std::string mission_state = (start_vehicle_name == std::string::npos) ?
    //     "unknown" :
    //     seglist.get_label().substr(start_vehicle_name + ownship_.size() + 1);
    
    boost::to_upper(mission_state_);

    std::cout << "Mission_State is: [" << mission_state_ << "]" <<  std::endl;
    
    const google::protobuf::EnumValueDescriptor* enum_value = enum_desc->FindValueByName(mission_state_);
    const google::protobuf::FieldDescriptor* enum_field = out.GetDescriptor()->FindFieldByName(MISSION_FIELD_NAME);
    if(enum_value && enum_field)
        out.GetReflection()->SetEnum(&out, enum_field, enum_value);
    
    out.set_active(seglist.active());
    return out;
}

XYSegList CViewMessage::from_protobuf(const lamss::protobuf::ViewSegList& seglist)
{
    XYSegList out;

    for(int i = 0, n = seglist.point_size(); i < n; ++i)
    {
        std::cout << "seglist.point(i).x() == NaN" << std::boolalpha << is_nan(seglist.point(i).x()) << std::endl;
        
        if(!is_nan(seglist.point(i).x()) &&
           !is_nan(seglist.point(i).y()) &&
           !is_nan(seglist.point(i).z()))
        {
            std::cout << "adding x: " << seglist.point(i).x() << ", y: " << seglist.point(i).y() << ", z: " << seglist.point(i).z() << std::endl;      
            
            out.add_vertex(seglist.point(i).x(), seglist.point(i).y(), seglist.point(i).z());
        }
    }
    
    out.set_active(seglist.active());
    out.set_label(seglist.vehicle_name() + "_" + seglist.GetReflection()->GetEnum(seglist, seglist.GetDescriptor()->FindFieldByName(MISSION_FIELD_NAME))->name());
    out.set_edge_size(1);
    return out;
}
