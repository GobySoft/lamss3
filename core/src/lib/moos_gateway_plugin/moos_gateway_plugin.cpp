#include <ivp/XYCircle.h>
#include <ivp/XYSegList.h>

#include "goby/middleware/marshalling/cstr.h"
#include "goby/middleware/marshalling/protobuf.h"

#include "goby/moos/middleware/moos_plugin_translator.h"
#include "goby/moos/moos_translator.h"
#include "goby/moos/protobuf/pAcommsHandler_config.pb.h"

#include "lamss/lib_goby3_lamss_gateway/groups.h"

#include "lamss/lib_lamss_protobuf/lamss_goby3_gateway_plugin.pb.h"
#include "lamss/lib_lamss_protobuf/lamss_status.pb.h"
#include "netsim/messages/netsim.pb.h"
#include "lamss/lib_lamss_protobuf/node_status.pb.h"
#include "lamss/lib_lamss_protobuf/tagged_acomms.pb.h"
#include "lamss/lib_lamss_protobuf/view_circle.pb.h"
#include "lamss/lib_lamss_protobuf/view_seglist.pb.h"

#include "lamss/lib_lamss_protobuf/icex_groups.h"
#include "lamss/lib_lamss_protobuf/icex_tracking_20.pb.h"

using goby::glog;
using namespace goby::util::logger;

// ensure that liblamss_protobuf is linked in
LAMSS_STATUS unused;

class pAcommsHandlerMultiplexTranslation : public goby::moos::Translator
{
  public:
    pAcommsHandlerMultiplexTranslation(const goby::apps::moos::protobuf::GobyMOOSGatewayConfig& cfg)
        : goby::moos::Translator(cfg)
    {

        for (int i = 0, n = cfg.GetExtension(lamss::protobuf::goby_gateway_config).load_shared_library_size(); i < n; ++i)
        {
            auto lib = cfg.GetExtension(lamss::protobuf::goby_gateway_config).load_shared_library(i);
            void* handle =
                dccl::DynamicProtobufManager::load_from_shared_lib(lib);

            if (!handle)
            {
                glog.is(DIE) && glog << "Failed to load library " << lib << ", check path provided or add to /etc/ld.so.conf "
                                     << "or LD_LIBRARY_PATH" << std::endl;
            }
        }

        
        this->goby()
            .interprocess()
            .subscribe_type_regex<lamss::groups::pAcommsHandler_multiplex,
                                  google::protobuf::Message>(
                [&](std::shared_ptr<const google::protobuf::Message> msg, const std::string& type) {
                    glog.is_debug2() && glog << "To MOOS: " << msg->ShortDebugString() << std::endl;

                    std::string serialized;
                    bool is_binary = serialize_for_moos(&serialized, *msg);
                    CMOOSMsg moos_msg = goby::moos::MOOSTranslator::make_moos_msg(
                        cfg.GetExtension(lamss::protobuf::goby_gateway_config).multiplex_moos_var(),
                        serialized, is_binary, goby::moos::moos_technique,
                        msg->GetDescriptor()->full_name());
                    moos().comms().Post(moos_msg);
                });

        this->moos().add_trigger("ACOMMS_ACK_ORIGINAL", [&](const CMOOSMsg& msg) {
            auto pb_msg = dynamic_parse_for_moos(msg.GetString());
            if (!pb_msg)
            {
                goby::glog.is_warn() && goby::glog << "Failed to parse message: " << msg.GetString()
                                                   << std::endl;
                return;
            }
            this->goby().interprocess().publish<lamss::groups::acomms_ack_original>(pb_msg);
        });

        this->moos().add_trigger("ACOMMS_EXPIRE", [&](const CMOOSMsg& msg) {
            auto pb_msg = dynamic_parse_for_moos(msg.GetString());
            if (!pb_msg)
            {
                goby::glog.is_warn() && goby::glog << "Failed to parse message: " << msg.GetString()
                                                   << std::endl;
                return;
            }
            this->goby().interprocess().publish<lamss::groups::acomms_expire>(pb_msg);
        });

        this->moos().add_trigger("ACOMMS_MAC_SLOT_START", [&](const CMOOSMsg& msg) {
            auto pb_msg = dynamic_parse_for_moos(msg.GetString());
            if (!pb_msg)
            {
                goby::glog.is_warn() && goby::glog << "Failed to parse message: " << msg.GetString()
                                                   << std::endl;
                return;
            }
            this->goby().interprocess().publish<lamss::groups::acomms_mac_slot_start>(pb_msg);
        });

        this->moos().add_trigger("HYDROMAN_HEALTH_IN", [&](const CMOOSMsg& msg) {
            auto pb_msg = dynamic_parse_for_moos(msg.GetString());
            if (!pb_msg)
            {
                goby::glog.is_warn() && goby::glog << "Failed to parse message: " << msg.GetString()
                                                   << std::endl;
                return;
            }
            this->goby().interprocess().publish<lamss::groups::hydroman_health>(pb_msg);
        });

        this->goby()
            .interprocess()
            .subscribe<lamss::groups::acomms_raw_in, lamss::protobuf::NamedModemRaw>(
                [this](const lamss::protobuf::NamedModemRaw& message) {
                    // post GPS for iSerialNMEA
                    // $GPRMC
                    //    |
                    // 0123
                    const std::string::size_type talker_start_pos = 3;
                    const std::string::size_type talker_size = 3;
                    if (message.msg().raw().size() > (talker_start_pos + talker_size) &&
                        (message.msg().raw().substr(talker_start_pos, talker_size) == "RMC" ||
                         message.msg().raw().substr(talker_start_pos, talker_size) == "GGA"))
                    {
                        this->moos().comms().Notify(
                            boost::to_upper_copy(message.name()) + "_GPS_IN", message.msg().raw());
                    }
                });
    }
};

class NavStatusTranslation : public goby::moos::Translator
{
  public:
    NavStatusTranslation(const goby::apps::moos::protobuf::GobyMOOSGatewayConfig& cfg)
        : goby::moos::Translator(cfg)
    {
        this->moos().add_trigger("LAMSS_STATUS_IN", [&](const CMOOSMsg& msg) {
            auto pb_msg = dynamic_parse_for_moos(msg.GetString());
            if (!pb_msg)
            {
                goby::glog.is_warn() && goby::glog << "Failed to parse message: " << msg.GetString()
                                                   << std::endl;
                return;
            }
            this->goby().interprocess().publish<lamss::groups::lamss_status_in>(pb_msg);
        });
    }
};

class NodeReportTranslation : public goby::moos::Translator
{
  public:
    NodeReportTranslation(const goby::apps::moos::protobuf::GobyMOOSGatewayConfig& cfg)
        : goby::moos::Translator(cfg)
    {
        {
            goby::moos::protobuf::TranslatorEntry entry;
            entry.set_protobuf_name(lamss::protobuf::LegacyNodeReport::descriptor()->full_name());
            auto& create = *entry.add_create();
            create.set_technique(goby::moos::protobuf::TranslatorEntry::
                                     TECHNIQUE_COMMA_SEPARATED_KEY_EQUALS_VALUE_PAIRS);
            create.set_moos_var("NODE_REPORT");
            auto& publish = *entry.add_publish();
            publish.set_technique(goby::moos::protobuf::TranslatorEntry::
                                      TECHNIQUE_COMMA_SEPARATED_KEY_EQUALS_VALUE_PAIRS);
            publish.set_moos_var("NODE_REPORT");
            translator_.add_entry(entry);
        }

        this->moos().add_trigger("NODE_REPORT", [&](const CMOOSMsg& moos_msg) {
            std::map<std::string, CMOOSMsg> moos_msgs = {{moos_msg.GetKey(), moos_msg}};
            auto pb_msg = translator_.moos_to_protobuf<std::shared_ptr<google::protobuf::Message>>(
                moos_msgs, "lamss.protobuf.LegacyNodeReport");

            auto nr = std::dynamic_pointer_cast<lamss::protobuf::LegacyNodeReport>(pb_msg);
            if (nr->has_time() && !nr->has_utc_time())
                nr->set_utc_time(nr->time());
            else if (nr->has_utc_time() && !nr->has_time())
                nr->set_time(nr->utc_time());

            this->goby().interprocess().publish<lamss::groups::node_report>(nr);
        });

        this->goby()
            .interprocess()
            .subscribe<lamss::groups::node_report_out, lamss::protobuf::LegacyNodeReport>(
                [this](const lamss::protobuf::LegacyNodeReport& nr) {
                    std::multimap<std::string, CMOOSMsg> moos_msgs =
                        translator_.protobuf_to_moos(nr);
                    for (auto moos_msg_p : moos_msgs) this->moos().comms().Post(moos_msg_p.second);
                });
    }

  private:
    goby::moos::MOOSTranslator translator_;
};

class ViewGeometryTranslation : public goby::moos::Translator
{
  public:
    ViewGeometryTranslation(const goby::apps::moos::protobuf::GobyMOOSGatewayConfig& cfg)
        : goby::moos::Translator(cfg)
    {
        this->goby()
            .interprocess()
            .subscribe<lamss::groups::view_geometry, lamss::protobuf::ViewCircle>(
                [this](const lamss::protobuf::ViewCircle& circle) {
                    XYCircle ivp_circle(circle.center_x(), circle.center_y(), circle.radius());
                    ivp_circle.set_active(circle.active());

                    if (circle.has_label())
                        ivp_circle.set_label(circle.label());
                    this->moos().comms().Notify("VIEW_CIRCLE", ivp_circle.get_spec());
                });

        this->goby()
            .interprocess()
            .subscribe<lamss::groups::view_geometry, VIEW_SEGLIST,
                       goby::middleware::MarshallingScheme::PROTOBUF>(
                [this](const VIEW_SEGLIST& seglist) {
                    XYSegList ivp_seglist;
                    ivp_seglist.set_color("edge", "darkred");
                    ivp_seglist.set_color("vertex", "darkred");

                    // for GEOV
                    std::stringstream old_view_polygon;
                    old_view_polygon << "active,true:label," << seglist.label()
                                     << ":edge_size,0:vertex_size,0";

                    for (int i = 0; i < std::min(seglist.point_x_size(), seglist.point_y_size());
                         ++i)
                    {
                        ivp_seglist.add_vertex(seglist.point_x(i), seglist.point_y(i));
                        old_view_polygon << ":" << seglist.point_x(i) << "," << seglist.point_y(i);
                    }

                    ivp_seglist.set_active(seglist.point_x_size() > 0);

                    if (seglist.has_label())
                        ivp_seglist.set_label(seglist.label());
                    this->moos().comms().Notify("VIEW_SEGLIST", ivp_seglist.get_spec());

                    this->moos().comms().Notify("OLD_VIEW_POLYGON", old_view_polygon.str());
                });
    }
};

class NetSimTranslation : public goby::moos::Translator
{
  public:
    NetSimTranslation(const goby::apps::moos::protobuf::GobyMOOSGatewayConfig& cfg)
        : goby::moos::Translator(cfg)
    {
        this->moos().add_trigger("TRANSMISSION_GRP_VEL", [&](const CMOOSMsg& msg) {
            auto pb_msg = dynamic_parse_for_moos(msg.GetString());
            if (!pb_msg)
            {
                goby::glog.is_warn() && goby::glog << "Failed to parse message: " << msg.GetString()
                                                   << std::endl;
                return;
            }
            this->goby().interprocess().publish<lamss::groups::transmission_grp_vel>(pb_msg);
        });

        this->goby().interprocess().subscribe<lamss::groups::objfunc_request, netsim::protobuf::ObjFuncRequest>(
            [this](const netsim::protobuf::ObjFuncRequest& request) {
                std::string serialized;
                bool is_binary = serialize_for_moos(&serialized, request);
                CMOOSMsg moos_msg = goby::moos::MOOSTranslator::make_moos_msg(
                    "PERFORMANCE_REQUEST", serialized, is_binary, goby::moos::moos_technique,
                    request.GetDescriptor()->full_name());
                moos().comms().Post(moos_msg);
            });

        this->moos().add_trigger("PERFORMANCE_RESPONSE", [&](const CMOOSMsg& msg) {
            auto pb_msg = dynamic_parse_for_moos(msg.GetString());
            if (!pb_msg)
            {
                goby::glog.is_warn() && goby::glog << "Failed to parse message: " << msg.GetString()
                                                   << std::endl;
                return;
            }
            this->goby().interprocess().publish<lamss::groups::objfunc_response>(pb_msg);
        });
    }
};

class ICEXTranslation : public goby::moos::Translator
{
  public:
    ICEXTranslation(const goby::apps::moos::protobuf::GobyMOOSGatewayConfig& cfg)
        : goby::moos::Translator(cfg)
    {
        this->goby()
            .interprocess()
            .subscribe<lamss::groups::icex_transmit_change_notification,
                       lamss::icex::protobuf::TransmitChangeNotification>(
                [this](const lamss::icex::protobuf::TransmitChangeNotification& notification) {
                    // publish values for iSerialNMEA, skipping hydrohole (0)
                    for (int i = 1, n = notification.receive_size(); i < n; ++i)
                    {
                        this->moos().comms().Notify(
                            "ICEX_HYDROPHONE_RX_DEPTH_H" + std::to_string(i),
                            notification.receive(i).depth_with_units() / boost::units::si::meters);
                    }
                });

        this->goby().interprocess().subscribe<lamss::groups::acomms_config_request, std::string>(
            [this](const std::string& request) {
                this->moos().comms().Notify("ACOMMS_CONFIG_REQUEST", request);
            });

        this->goby()
            .interprocess()
            .subscribe<lamss::groups::acomms_queue_flush, goby::acomms::protobuf::QueueFlush>(
                [this](const goby::acomms::protobuf::QueueFlush& flush) {
                    std::string serialized;
                    bool is_binary = serialize_for_moos(&serialized, flush);
                    CMOOSMsg moos_msg = goby::moos::MOOSTranslator::make_moos_msg(
                        "ACOMMS_FLUSH_QUEUE", serialized, is_binary, goby::moos::moos_technique,
                        flush.GetDescriptor()->full_name());
                    moos().comms().Post(moos_msg);
                });

        this->moos().add_trigger("ACOMMS_CONFIG", [&](const CMOOSMsg& msg) {
            goby::apps::moos::protobuf::pAcommsHandlerConfig acomms_config;
            acomms_config.ParseFromString(dccl::b64_decode(msg.GetString()));
            this->goby().interprocess().publish<lamss::groups::acomms_config_response>(
                acomms_config);
        });

        this->moos().add_trigger("ACOMMS_QSIZE", [&](const CMOOSMsg& msg) {
            goby::acomms::protobuf::QueueSize qsize;
            parse_for_moos(msg.GetString(), &qsize);
            this->goby().interprocess().publish<lamss::groups::acomms_queue_size>(qsize);
        });

        this->moos().add_trigger("ACOMMS_QUEUE_RECEIVE", [&](const CMOOSMsg& msg) {
            auto pb_msg = dynamic_parse_for_moos(msg.GetString());
            if (!pb_msg)
            {
                goby::glog.is_warn() && goby::glog << "Failed to parse message: " << msg.GetString()
                                                   << std::endl;
                return;
            }
            this->goby().interprocess().publish<lamss::groups::acomms_queue_receive>(pb_msg);
        });

        this->moos().add_trigger("ACOMMS_MODEM_TRANSMIT", [&](const CMOOSMsg& msg) {
            goby::acomms::protobuf::ModemTransmission tx_msg;
            parse_for_moos(msg.GetString(), &tx_msg);
            this->goby().interprocess().publish<lamss::groups::acomms_modem_transmit>(tx_msg);
        });

        this->moos().add_trigger("ACOMMS_MODEM_RECEIVE", [&](const CMOOSMsg& msg) {
            goby::acomms::protobuf::ModemTransmission rx_msg;
            parse_for_moos(msg.GetString(), &rx_msg);
            this->goby().interprocess().publish<lamss::groups::acomms_modem_receive>(rx_msg);
        });
    }
};

extern "C"
{
    void goby3_moos_gateway_load(
        goby::zeromq::MultiThreadApplication<goby::apps::moos::protobuf::GobyMOOSGatewayConfig>*
            handler)
    {
        handler->launch_thread<pAcommsHandlerMultiplexTranslation>();
        handler->launch_thread<NavStatusTranslation>();
        handler->launch_thread<NodeReportTranslation>();
        handler->launch_thread<ViewGeometryTranslation>();
        handler->launch_thread<NetSimTranslation>();
        handler->launch_thread<ICEXTranslation>();
    }

    void goby3_moos_gateway_unload(
        goby::zeromq::MultiThreadApplication<goby::apps::moos::protobuf::GobyMOOSGatewayConfig>*
            handler)
    {
        handler->join_thread<pAcommsHandlerMultiplexTranslation>();
        handler->join_thread<NavStatusTranslation>();
        handler->join_thread<NodeReportTranslation>();
        handler->join_thread<ViewGeometryTranslation>();
        handler->join_thread<NetSimTranslation>();
        handler->join_thread<ICEXTranslation>();
    }
}
