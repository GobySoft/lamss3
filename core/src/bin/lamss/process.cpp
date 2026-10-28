// Copyright 2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

#include "goby/util/debug_logger.h"

#include "process.h"
#include "repo.h"

namespace lamss
{
namespace tool
{
namespace
{
int decode_status(int status)
{
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);
    return 1;
}

// Like system(), the parent ignores SIGINT/SIGQUIT while the child runs so Ctrl-C stops the child
// and we still report its status.
class IgnoreInterrupts
{
  public:
    IgnoreInterrupts()
    {
        struct sigaction ignore = {};
        ignore.sa_handler = SIG_IGN;
        sigemptyset(&ignore.sa_mask);
        sigaction(SIGINT, &ignore, &old_int_);
        sigaction(SIGQUIT, &ignore, &old_quit_);
    }
    ~IgnoreInterrupts()
    {
        sigaction(SIGINT, &old_int_, nullptr);
        sigaction(SIGQUIT, &old_quit_, nullptr);
    }

  private:
    struct sigaction old_int_ = {};
    struct sigaction old_quit_ = {};
};

[[noreturn]] void exec_child(const Argv& command, const RunOptions& options)
{
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);

    if (!options.cwd.empty() && chdir(options.cwd.c_str()) != 0)
    {
        std::cerr << "lamss: cannot change directory to " << options.cwd.string() << ": "
                  << std::strerror(errno) << std::endl;
        _exit(127);
    }
    if (options.discard_stderr)
    {
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0)
            dup2(devnull, STDERR_FILENO);
    }

    std::vector<char*> argv;
    for (const auto& arg : command) argv.push_back(const_cast<char*>(arg.c_str()));
    argv.push_back(nullptr);
    execvp(argv[0], argv.data());
    if (!options.discard_stderr)
        std::cerr << "lamss: cannot run " << command[0] << ": " << std::strerror(errno)
                  << std::endl;
    _exit(127);
}

int wait_child(pid_t pid)
{
    int status = 0;
    while (waitpid(pid, &status, 0) < 0)
    {
        if (errno != EINTR)
            throw Error(std::string("waitpid failed: ") + std::strerror(errno));
    }
    return decode_status(status);
}

void log_command(const Argv& command, const RunOptions& options)
{
    goby::glog.is_verbose() &&
        goby::glog << "running: "
                   << (options.cwd.empty()
                           ? ""
                           : "(cd " + format_command({options.cwd.string()}) + " && ")
                   << format_command(command) << (options.cwd.empty() ? "" : ")") << std::endl;
}
} // namespace

std::string format_command(const Argv& command)
{
    std::string out;
    for (const auto& arg : command)
    {
        if (!out.empty())
            out += ' ';
        bool plain =
            !arg.empty() && arg.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                  "abcdefghijklmnopqrstuvwxyz"
                                                  "0123456789_-+=/.,:@%") == std::string::npos;
        if (plain)
        {
            out += arg;
            continue;
        }
        out += '\'';
        for (char c : arg)
        {
            if (c == '\'')
                out += "'\\''";
            else
                out += c;
        }
        out += '\'';
    }
    return out;
}

int run_process(const Argv& command, const RunOptions& options)
{
    if (command.empty())
        throw Error("empty command");
    log_command(command, options);
    std::cout.flush();
    std::cerr.flush();

    IgnoreInterrupts ignore;
    pid_t pid = fork();
    if (pid < 0)
        throw Error(std::string("fork failed: ") + std::strerror(errno));
    if (pid == 0)
        exec_child(command, options);
    return wait_child(pid);
}

Captured capture_output(const Argv& command, const RunOptions& options)
{
    if (command.empty())
        throw Error("empty command");
    log_command(command, options);
    std::cout.flush();
    std::cerr.flush();

    int fds[2];
    if (pipe(fds) != 0)
        throw Error(std::string("pipe failed: ") + std::strerror(errno));

    IgnoreInterrupts ignore;
    pid_t pid = fork();
    if (pid < 0)
    {
        close(fds[0]);
        close(fds[1]);
        throw Error(std::string("fork failed: ") + std::strerror(errno));
    }
    if (pid == 0)
    {
        close(fds[0]);
        dup2(fds[1], STDOUT_FILENO);
        close(fds[1]);
        exec_child(command, options);
    }

    close(fds[1]);
    Captured result{0, ""};
    char buffer[4096];
    while (true)
    {
        ssize_t n = read(fds[0], buffer, sizeof(buffer));
        if (n > 0)
            result.out.append(buffer, static_cast<std::size_t>(n));
        else if (n == 0 || errno != EINTR)
            break;
    }
    close(fds[0]);
    result.status = wait_child(pid);
    return result;
}

} // namespace tool
} // namespace lamss
