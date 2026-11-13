// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>

#include "command.h"
#include "process.h"
#include "repo.h"

namespace
{
using namespace lamss::tool;

int run_docs(const Args& args)
{
    Repo repo(find_root());
    setenv("LAMSS_ROOT", repo.root().c_str(), 1);
    setenv("LAMSS_PREFIX", repo.prefix().c_str(), 1);
    // the checkout's own sources, so the site builds without `lamss build`
    const auto python = (repo.root() / "core" / "src" / "python").string();
    const char* current = std::getenv("PYTHONPATH");
    setenv("PYTHONPATH", (current && *current ? python + ":" + current : python).c_str(), 1);

    Argv command{"python3", "-m", "lamss.docs"};
    for (const auto& arg : args.values("args")) command.push_back(arg);
    exec_process(command);
}
} // namespace

lamss::tool::Command lamss::tool::docs_command()
{
    Command docs{"docs",
                 {},
                 "Build, serve and check the documentation site (`lamss docs --help`)",
                 {},
                 {{"args", "Passed to python3 -m lamss.docs", false, true}},
                 {},
                 run_docs};
    docs.passthrough = true;
    return docs;
}
