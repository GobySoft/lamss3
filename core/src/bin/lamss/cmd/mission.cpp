// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>

#include "command.h"
#include "process.h"
#include "repo.h"

namespace
{
using namespace lamss::tool;

void prepend_env(const char* variable, const std::string& entry)
{
    const char* current = std::getenv(variable);
    std::string value = entry;
    if (current && *current)
        value += std::string(":") + current;
    setenv(variable, value.c_str(), 1);
}

int run_mission(const Args& args)
{
    Repo repo(find_root());
    const auto prefix = repo.prefix();
    setenv("LAMSS_ROOT", repo.root().c_str(), 1);
    setenv("LAMSS_PREFIX", prefix.c_str(), 1);
    prepend_env("PATH", (prefix / "bin").string());
    prepend_env("LD_LIBRARY_PATH", (prefix / "lib").string());
    prepend_env("PYTHONPATH", (prefix / "lib" / "python3" / "dist-packages").string());

    Argv command{"python3", "-m", "lamss.mission"};
    for (const auto& arg : args.values("args")) command.push_back(arg);
    exec_process(command);
}
} // namespace

lamss::tool::Command lamss::tool::mission_command()
{
    Command mission{"mission",
                    {"m"},
                    "Generate and run missions (`lamss mission --help`)",
                    {},
                    {{"args", "Passed to python3 -m lamss.mission", false, true}},
                    {},
                    run_mission};
    mission.passthrough = true;
    return mission;
}
