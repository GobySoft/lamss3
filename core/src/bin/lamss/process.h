// Copyright 2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LAMSS_TOOL_PROCESS_H
#define LAMSS_TOOL_PROCESS_H

#include <filesystem>
#include <string>
#include <vector>

namespace lamss
{
namespace tool
{
using Argv = std::vector<std::string>;

struct RunOptions
{
    std::filesystem::path cwd;
    bool discard_stderr{false};
};

// Shell-quoted, for display and so the user can reproduce a command by hand.
std::string format_command(const Argv& command);

// Runs with inherited stdio; returns the exit status (128 + signal if killed, 127 if exec fails).
int run_process(const Argv& command, const RunOptions& options = {});

struct Captured
{
    int status;
    std::string out;
};
// Runs with stdout captured.
Captured capture_output(const Argv& command, const RunOptions& options = {});

} // namespace tool
} // namespace lamss

#endif
