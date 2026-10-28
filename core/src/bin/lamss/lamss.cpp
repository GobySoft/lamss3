// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cmd/command.h"

int main(int argc, char* argv[])
{
    using namespace lamss::tool;
    Command root{"lamss",
                 {},
                 "Build, configure and run lamss3.",
                 {},
                 {},
                 {build_command(), toolbox_command(), dependencies_command(), env_command(),
                  clean_command()}};
    return dispatch(root, argc, argv);
}
