// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VIEWMESSAGE_HEADER
#define VIEWMESSAGE_HEADER

#include <string>

#include "XYFormatUtilsPoint.h"
#include "XYFormatUtilsPoly.h"
#include "XYFormatUtilsSegl.h"
#include "XYPoint.h"
#include "XYPolygon.h"
#include "XYSegList.h"

#include "goby/moos/moos_geodesy.h"
#include "goby/version.h"

#include "goby/moos/goby_moos_app.h"
#include "goby/moos/moos_protobuf_helpers.h"

#include "view_message.pb.h"

class CViewMessage : public goby::moos::GobyMOOSApp
{
  public:
    static CViewMessage* get_instance();

  private:
    CViewMessage();
    void loop() override; // from GobyMOOSApp

    //  void loop() {}

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

    lamss::core::ViewPoint to_protobuf(const XYPoint& point);
    XYPoint from_protobuf(const lamss::core::ViewPoint& point);
    lamss::core::ViewSegList convex_polygon(const XYSegList& point);
    lamss::core::ViewSegList to_protobuf(const XYSegList& point);
    XYSegList from_protobuf(const lamss::core::ViewSegList& point);

    void handle_lat_origin(const CMOOSMsg& msg);
    void handle_lon_origin(const CMOOSMsg& msg);

    // NaN values have the odd property that comparisons involving them are always
    // false
    bool is_nan(double a) { return a != a; }

    void to_moos_string(std::string* out, const google::protobuf::Message& in)
    {
#if GOBY_VERSION_MAJOR >= 2
        goby::moos::MOOSTranslation<
            goby::moos::protobuf::TranslatorEntry::
                TECHNIQUE_COMMA_SEPARATED_KEY_EQUALS_VALUE_PAIRS>::serialize(out, in);
#else
        to_moos_comma_equals_string(in, out);
#endif
    }

    void from_moos_string(const std::string& in, google::protobuf::Message* out)
    {
#if GOBY_VERSION_MAJOR >= 2
        goby::moos::MOOSTranslation<
            goby::moos::protobuf::TranslatorEntry::
                TECHNIQUE_COMMA_SEPARATED_KEY_EQUALS_VALUE_PAIRS>::parse(in, out);
#else
        from_moos_comma_equals_string(out, in);
#endif
    }

  private:
    std::string ownship_;
    std::string mission_state_;

    static const std::string TYPE_FIELD_NAME;
    static const std::string MISSION_FIELD_NAME;

    static lamss::core::ViewMessageConfig cfg_;
    static CViewMessage* inst_;

    int destination_;

    goby::moos::CMOOSGeodesy geodesy_;
    double lat_origin_;
    double lon_origin_;
    bool new_origin_;
};

#endif
