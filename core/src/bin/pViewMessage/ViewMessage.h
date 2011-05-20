/*****************************************************************/
/*    NAME: Henrik Schmidt, Toby Schneider                       */
/*    ORGN: MIT Cambridge MA                                     */
/*    FILE: ViewMessage.h                                        */
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

#include <string>

#include "lib_geometry/XYFormatUtilsPoint.h" 
#include "lib_geometry/XYFormatUtilsPoly.h" 
#include "lib_geometry/XYFormatUtilsSegl.h" 
#include "lib_geometry/XYPoint.h"
#include "lib_geometry/XYPolygon.h"
#include "lib_geometry/XYSegList.h"

#include "goby/moos/libmoos_util/moos_protobuf_helpers.h"
#include "goby/moos/libmoos_util/tes_moos_app.h"

#include "view_message.pb.h"

class CViewMessage : public TesMoosApp
{
  public:
    static CViewMessage* get_instance();

  private:
    CViewMessage();
    void loop() { }    

    // take publishes from BHVs and write appropriate output for pAcommsHandler
    void handle_view_point(const CMOOSMsg& msg);
    void handle_view_polygon(const CMOOSMsg& msg);
    void handle_view_seglist(const CMOOSMsg& msg);
    void process_seglist(const XYSegList& seglist, bool is_polygon = false);


    // write appropriate variables on receiving side
    void handle_in_view_point(const CMOOSMsg& msg);
    void handle_in_view_seglist(const CMOOSMsg& msg);
    void handle_in_view_polygon(const CMOOSMsg& msg);
    void process_in_seglist(const std::string& goby1_str, bool is_polygon = false);
        
    
    // set state
    void handle_vehicle_name(const CMOOSMsg& msg);
    void handle_mission_state(const CMOOSMsg& msg);


    lamss::protobuf::ViewPoint to_protobuf(const XYPoint& point);
    XYPoint from_protobuf(const lamss::protobuf::ViewPoint& point);    
    lamss::protobuf::ViewSegList to_protobuf(const XYSegList& point);
    XYSegList from_protobuf(const lamss::protobuf::ViewSegList& point);

    // NaN values have the odd property that comparisons involving them are always false
    bool is_nan(double a)
    { return a != a; }
    
    
  private:    
    std::string ownship_;  
    std::string mission_state_;

    static const std::string TYPE_FIELD_NAME;
    static const std::string MISSION_FIELD_NAME;

    static lamss::protobuf::ViewMessageConfig cfg_;
    static CViewMessage* inst_;    

    
    
};

#endif
