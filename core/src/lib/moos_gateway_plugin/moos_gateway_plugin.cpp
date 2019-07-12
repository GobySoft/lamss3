#include "goby/middleware/transport-common.h"

#include "goby/moos/middleware/moos_plugin_translator.h"
#include "goby/moos/moos_translator.h"

#include "lamss/lib_goby3_lamss_gateway/groups.h"

#include "lamss/lib_lamss_protobuf/lamss_goby3_gateway_plugin.pb.h"
#include "lamss/lib_lamss_protobuf/lamss_status.pb.h"

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
        this->goby()
            .interprocess()
            .subscribe_type_regex<lamss::groups::pAcommsHandler_multiplex,
                                  google::protobuf::Message>(
                [&](std::shared_ptr<const google::protobuf::Message> msg, const std::string& type) {
                    std::cout << "goby --> moos: type: " << type << ", msg: " << msg->ShortDebugString()
                              << std::endl;

                    std::string serialized;
                    bool is_binary = serialize_for_moos(&serialized, *msg);
                    CMOOSMsg moos_msg = goby::moos::MOOSTranslator::make_moos_msg(
                        cfg.GetExtension(lamss::protobuf::goby_gateway_config).multiplex_moos_var(),
                        serialized, is_binary, goby::moos::moos_technique,
                        msg->GetDescriptor()->full_name());
                    moos().comms().Post(moos_msg);
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
    }

    void goby3_moos_gateway_unload(
        goby::zeromq::MultiThreadApplication<goby::apps::moos::protobuf::GobyMOOSGatewayConfig>*
            handler)
    {
        handler->join_thread<pAcommsHandlerMultiplexTranslation>();
    }
}
