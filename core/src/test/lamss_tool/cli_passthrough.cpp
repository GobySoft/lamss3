// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests that a passthrough command receives every argument after it unparsed, options and --help
// included, while the commands above it still parse their own.

#include "check.h"
#include "cli.h"

using namespace lamss::tool;

int main()
{
    std::vector<std::string> received;
    Command mission{"mission",
                    {"m"},
                    "",
                    {},
                    {},
                    {},
                    [&](const Args& args)
                    {
                        received = args.values("args");
                        return 7;
                    }};
    mission.passthrough = true;
    Command root{"lamss", {}, "", {}, {}, {mission}};

    std::vector<std::string> tokens{"lamss", "-v",      "m",      "sim", "example",
                                    "--set", "warp=10", "--help", "--",  "-x"};
    std::vector<char*> argv;
    for (auto& t : tokens) argv.push_back(t.data());
    CHECK(dispatch(root, static_cast<int>(argv.size()), argv.data()) == 7);
    CHECK((received ==
           std::vector<std::string>{"sim", "example", "--set", "warp=10", "--help", "--", "-x"}));
    CHECK(verbose());

    received = {"unchanged"};
    std::vector<std::string> bare{"lamss", "mission"};
    std::vector<char*> bare_argv;
    for (auto& t : bare) bare_argv.push_back(t.data());
    CHECK(dispatch(root, 2, bare_argv.data()) == 7);
    CHECK(received.empty());
    return 0;
}
