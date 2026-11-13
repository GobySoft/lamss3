// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LAMSS_TOOL_PROCESS_H
#define LAMSS_TOOL_PROCESS_H

#include <filesystem>
#include <functional>
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

// Replaces this process with command; throws if it cannot be run.
[[noreturn]] void exec_process(const Argv& command);

struct Captured
{
    int status;
    std::string out;
};
// Runs with stdout captured.
Captured capture_output(const Argv& command, const RunOptions& options = {});

// Streams stdout line by line, dropping the lines hide() returns true for; stderr is inherited.
int run_filtered(const Argv& command, const std::function<bool(const std::string&)>& hide,
                 const RunOptions& options = {});

} // namespace tool
} // namespace lamss

#endif
