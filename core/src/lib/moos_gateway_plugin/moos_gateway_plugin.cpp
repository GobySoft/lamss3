#include "goby/middleware/transport-common.h"

#include "goby/moos/middleware/moos_plugin_translator.h"

#include "lamss/lib_goby3_lamss_gateway/groups.h"
#include "lamss/lib_lamss_protobuf/lamss_deploy.pb.h"

using goby::glog;
using namespace goby::util::logger;

class pAcommsHandlerMultiplexTranslation : public goby::moos::Translator
{
  public:
    pAcommsHandlerMultiplexTranslation(const goby::apps::moos::protobuf::GobyMOOSGatewayConfig& cfg)
        : goby::moos::Translator(cfg)
    {
        sleep(1);

        std::cout << "pAcommsHandlerMultiplexTranslation cfg: " << cfg.ShortDebugString()
                  << std::endl;

        std::cout << "plugin: middleware::SerializationSubscriptionRegex: " << std::type_index(typeid(goby::middleware::SerializationSubscriptionRegex<>)).hash_code() << std::endl;
        std::cout << "plugin: middleware::SerializationHandlerBase<>: " << std::type_index(typeid(goby::middleware::SerializationHandlerBase<>)).hash_code() << std::endl;
        
        this->goby()
            .interprocess()
            .subscribe_regex<lamss::groups::pAcommsHandler_multiplex, google::protobuf::Message>(
                [&](const std::vector<unsigned char>& data, int scheme, const std::string& type,
                    const goby::middleware::Group& group) {
                    std::cout << "Received: " << data.size() << " bytes from scheme: " << scheme
                              << ", type: " << type << ", group: " << group << std::endl;
                });

        this->goby()
            .interprocess()
            .subscribe<lamss::groups::pAcommsHandler_multiplex, LAMSS_DEPLOY, goby::middleware::MarshallingScheme::PROTOBUF>(
                [this](const LAMSS_DEPLOY& deploy) 
                {
                    std::cout << "Deploy: " << deploy.ShortDebugString() << std::endl;
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
