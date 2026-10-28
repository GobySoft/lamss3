// Copyright 2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cmd/command.h"

namespace lamss
{
namespace tool
{
int& exit_status()
{
    static int status = 0;
    return status;
}

goby::util::protobuf::GLogConfig::Verbosity& inherited_verbosity()
{
    static auto verbosity = goby::util::protobuf::GLogConfig::WARN;
    return verbosity;
}

class LamssTool : public Dispatcher<protobuf::LamssToolConfig>
{
  public:
    LamssTool();
};

} // namespace tool
} // namespace lamss

int main(int argc, char* argv[])
{
    using namespace lamss::tool;
    int status = goby::run<LamssTool>(Configurator<protobuf::LamssToolConfig>(argc, argv));
    return status != 0 ? status : exit_status();
}

lamss::tool::LamssTool::LamssTool()
{
    using protobuf::LamssToolConfig;
    auto helper = tool_helper();

    if (!helper.perform_action(app_cfg().action()))
    {
        switch (app_cfg().action())
        {
            case LamssToolConfig::help:
                int action_for_help;
                if (!helper.help(&action_for_help))
                {
                    switch (action_for_help)
                    {
                        case LamssToolConfig::build:
                            help<BuildTool>(helper, action_for_help);
                            break;
                        case LamssToolConfig::toolbox:
                            help<ToolboxTool>(helper, action_for_help);
                            break;
                        case LamssToolConfig::env: help<EnvTool>(helper, action_for_help); break;
                        case LamssToolConfig::clean:
                            help<CleanTool>(helper, action_for_help);
                            break;
                        default: unhandled_action();
                    }
                }
                break;

            case LamssToolConfig::build: run_subtool<BuildTool>(helper); break;
            case LamssToolConfig::toolbox: run_subtool<ToolboxTool>(helper); break;
            case LamssToolConfig::env: run_subtool<EnvTool>(helper); break;
            case LamssToolConfig::clean: run_subtool<CleanTool>(helper); break;
            default: unhandled_action();
        }
    }

    quit(exit_status());
}
