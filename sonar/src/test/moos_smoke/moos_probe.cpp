// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

// Usage: moos_probe <port> <timeout_seconds> [VAR=value]... [VAR:=number]... [?VAR[~substring]]...
//
// Connects to the MOOSDB on localhost:<port>, registers for every ?VAR, posts every VAR=value
// (as a string) and VAR:=number (as a double), then waits until each ?VAR has been posted (with a
// value containing <substring> when given). Exits 0 once all expectations are met, 1 on timeout,
// printing what it saw. A copy of core's lamss_core_moos_probe that also posts doubles.

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <unistd.h>

#include <MOOS/libMOOS/Comms/MOOSAsyncCommClient.h>

namespace
{
struct Expectation
{
    std::string var;
    std::string substring;
    bool met{false};
};
} // namespace

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "usage: moos_probe <port> <timeout_seconds> [VAR=value]... "
                     "[VAR:=number]... [?VAR[~substring]]..."
                  << std::endl;
        return 2;
    }
    const int port = std::stoi(argv[1]);
    const auto timeout = std::chrono::duration<double>(std::stod(argv[2]));

    std::vector<std::pair<std::string, std::string>> posts;
    std::vector<std::pair<std::string, double>> numeric_posts;
    std::vector<Expectation> expectations;
    for (int i = 3; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg.size() > 1 && arg[0] == '?')
        {
            auto tilde = arg.find('~');
            expectations.push_back(
                {arg.substr(1, tilde == std::string::npos ? std::string::npos : tilde - 1),
                 tilde == std::string::npos ? "" : arg.substr(tilde + 1)});
        }
        else
        {
            auto eq = arg.find('=');
            if (eq == std::string::npos)
            {
                std::cerr << "moos_probe: expected VAR=value or ?VAR, got: " << arg << std::endl;
                return 2;
            }
            if (eq > 0 && arg[eq - 1] == ':')
                numeric_posts.emplace_back(arg.substr(0, eq - 1), std::stod(arg.substr(eq + 1)));
            else
                posts.emplace_back(arg.substr(0, eq), arg.substr(eq + 1));
        }
    }

    MOOS::MOOSAsyncCommClient comms;
    comms.SetQuiet(true);
    // a unique name, so that several probes can be connected to one MOOSDB at once
    comms.Run("localhost", port, "moos_probe_" + std::to_string(getpid()));

    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!comms.IsConnected())
    {
        if (std::chrono::steady_clock::now() > deadline)
        {
            std::cerr << "moos_probe: could not connect to localhost:" << port << std::endl;
            return 1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    for (const auto& e : expectations) comms.Register(e.var, 0);
    // give the registrations time to reach the MOOSDB before posting what triggers the replies
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    for (const auto& [var, value] : posts) comms.Notify(var, value);
    for (const auto& [var, value] : numeric_posts) comms.Notify(var, value);
    if (!posts.empty() || !numeric_posts.empty())
        std::this_thread::sleep_for(
            std::chrono::milliseconds(200)); // let the posts flush before Close

    auto all_met = [&]()
    {
        for (const auto& e : expectations)
            if (!e.met)
                return false;
        return true;
    };
    while (!all_met() && std::chrono::steady_clock::now() < deadline)
    {
        MOOSMSG_LIST msgs;
        if (comms.Fetch(msgs))
        {
            for (auto& msg : msgs)
            {
                std::cout << msg.GetKey() << " [" << msg.GetSource() << "]: " << msg.GetAsString()
                          << std::endl;
                for (auto& e : expectations)
                    if (e.var == msg.GetKey() &&
                        msg.GetAsString().find(e.substring) != std::string::npos)
                        e.met = true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    for (const auto& e : expectations)
        if (!e.met)
            std::cerr << "moos_probe: never saw " << e.var
                      << (e.substring.empty() ? "" : " containing \"" + e.substring + "\"")
                      << std::endl;
    comms.Close(true);
    return all_met() ? 0 : 1;
}
