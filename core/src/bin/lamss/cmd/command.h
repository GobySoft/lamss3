// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LAMSS_TOOL_CMD_COMMAND_H
#define LAMSS_TOOL_CMD_COMMAND_H

#include <cassert>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>

#include "goby/middleware/application/configurator.h"
#include "goby/middleware/application/interface.h"
#include "goby/middleware/application/tool.h"

#include "lamss.pb.h"

namespace lamss
{
namespace tool
{
// ToolHelper::run_subtool discards the subtool's return value, so it is passed up through here.
int& exit_status();
constexpr int usage_error = 2;
// Subtools only see the arguments after their action, so "lamss -v build" is passed down here.
goby::util::protobuf::GLogConfig::Verbosity& inherited_verbosity();

template <typename Config>
class Configurator : public goby::middleware::ProtobufConfigurator<Config>
{
  public:
    Configurator(int argc, char* argv[])
        : goby::middleware::ProtobufConfigurator<Config>(argc, argv)
    {
        auto& cfg = this->mutable_cfg();
        auto& verbosity = inherited_verbosity();
        auto& glog_config = *cfg.mutable_app()->mutable_glog_config();
        if (glog_config.has_tty_verbosity())
            verbosity = glog_config.tty_verbosity();
        else
            glog_config.set_tty_verbosity(verbosity);

        // binary is only left unset when the command line failed to parse (and help follows)
        if (!cfg.app().has_binary())
        {
            exit_status() = usage_error;
            cfg.mutable_app()->set_binary(binary_name(argc, argv));
        }
    }

  protected:
    void validate() const override
    {
        try
        {
            goby::middleware::ProtobufConfigurator<Config>::validate();
        }
        catch (const goby::middleware::ConfigException&)
        {
            exit_status() = usage_error;
            throw;
        }
    }

  private:
    static std::string binary_name(int argc, char* argv[])
    {
        const std::string flag = "--binary=";
        for (int i = 1; i < argc; ++i)
        {
            std::string arg(argv[i]);
            if (arg.compare(0, flag.size(), flag) == 0)
                return arg.substr(flag.size());
        }
        return std::filesystem::path(argv[0]).filename().string();
    }
};

template <typename Config> class Command : public goby::middleware::Application<Config>
{
  protected:
    void execute(const std::function<int()>& body)
    {
        int status = 1;
        try
        {
            status = body();
        }
        catch (const std::exception& e)
        {
            std::cerr << "lamss: error: " << e.what() << std::endl;
        }
        exit_status() = status;
        this->quit(status);
    }

  private:
    void run() override { assert(false); }
};

template <typename Config> class Dispatcher : public goby::middleware::Application<Config>
{
  protected:
    goby::middleware::ToolHelper tool_helper()
    {
        return goby::middleware::ToolHelper(this->app_cfg().app().binary(),
                                            this->app_cfg().app().tool_cfg(),
                                            Config::Action_descriptor());
    }

    template <typename App> void run_subtool(goby::middleware::ToolHelper& helper)
    {
        try
        {
            helper.run_subtool<App, Configurator<typename App::ConfigType>>();
        }
        catch (const goby::middleware::ConfigException&)
        {
            // already reported by the subtool's configurator
            exit_status() = usage_error;
        }
    }

    template <typename App> void help(goby::middleware::ToolHelper& helper, int action)
    {
        helper.help<App, Configurator<typename App::ConfigType>>(action);
    }

    [[noreturn]] void unhandled_action()
    {
        throw goby::Exception("Action was expected to be handled by an external tool");
    }

  private:
    void run() override { assert(false); }
};

class BuildTool : public Command<protobuf::BuildToolConfig>
{
  public:
    BuildTool();
};

class EnvTool : public Command<protobuf::EnvToolConfig>
{
  public:
    EnvTool();
};

class CleanTool : public Command<protobuf::CleanToolConfig>
{
  public:
    CleanTool();
};

class ToolboxListTool : public Command<protobuf::ToolboxListToolConfig>
{
  public:
    ToolboxListTool();
};

class ToolboxAddTool : public Command<protobuf::ToolboxAddToolConfig>
{
  public:
    ToolboxAddTool();
};

class ToolboxRemoveTool : public Command<protobuf::ToolboxRemoveToolConfig>
{
  public:
    ToolboxRemoveTool();
};

class ToolboxActivateTool : public Command<protobuf::ToolboxActivateToolConfig>
{
  public:
    ToolboxActivateTool();
};

class ToolboxDeactivateTool : public Command<protobuf::ToolboxDeactivateToolConfig>
{
  public:
    ToolboxDeactivateTool();
};

class ToolboxTool : public Dispatcher<protobuf::ToolboxToolConfig>
{
  public:
    ToolboxTool();
};

} // namespace tool
} // namespace lamss

#endif
