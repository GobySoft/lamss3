// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LAMSS_TOOL_CLI_H
#define LAMSS_TOOL_CLI_H

#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace lamss
{
namespace tool
{
constexpr int usage_error = 2;

class UsageError : public std::runtime_error
{
  public:
    using std::runtime_error::runtime_error;
};

struct Option
{
    std::string name;
    char short_name = 0;
    // empty for a flag
    std::string value_name = {};
    std::string help = {};
    std::vector<std::string> choices = {};
    bool repeated = false;
};

struct Positional
{
    std::string name;
    std::string help;
    bool required = false;
    bool variadic = false;
};

class Args
{
  public:
    bool flag(const std::string& name) const { return values_.count(name) > 0; }
    std::optional<std::string> value(const std::string& name) const;
    std::vector<std::string> values(const std::string& name) const;

    void add(const std::string& name, const std::string& value) { values_[name].push_back(value); }

  private:
    std::map<std::string, std::vector<std::string>> values_;
};

struct Command
{
    std::string name;
    std::vector<std::string> aliases;
    std::string summary;
    std::vector<Option> options = {};
    std::vector<Positional> positionals = {};
    std::vector<Command> subcommands = {};
    std::function<int(const Args&)> run = {};
};

bool& verbose();

// Parses argv against the command tree and runs the selected command, returning the exit status.
int dispatch(const Command& root, int argc, char* argv[]);

} // namespace tool
} // namespace lamss

#endif
