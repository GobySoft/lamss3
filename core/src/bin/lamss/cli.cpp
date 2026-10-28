// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <filesystem>
#include <iostream>

#include "cli.h"

namespace lamss
{
namespace tool
{
namespace
{
const Option help_option{"help", 'h', "", "Show this help"};
const Option verbose_option{"verbose", 'v', "", "Print each command before running it"};

std::string join(const std::vector<std::string>& items, const std::string& separator)
{
    std::string out;
    for (const auto& item : items) out += (out.empty() ? "" : separator) + item;
    return out;
}

void print_table(const std::vector<std::pair<std::string, std::string>>& rows)
{
    std::size_t width = 0;
    for (const auto& row : rows) width = std::max(width, row.first.size());
    for (const auto& [left, right] : rows)
        std::cout << "  " << left << std::string(width - left.size() + 3, ' ') << right << "\n";
}

std::string option_label(const Option& o)
{
    std::string label = o.short_name ? std::string("-") + o.short_name + ", " : "    ";
    label += "--" + o.name;
    if (!o.value_name.empty())
        label += " " + o.value_name;
    return label;
}

std::string option_help(const Option& o)
{
    std::string help = o.help;
    if (!o.choices.empty())
        help += " (" + join(o.choices, ", ") + ")";
    if (o.repeated)
        help += " [repeatable]";
    return help;
}

void print_help(const Command& cmd, const std::string& path)
{
    std::cout << "Usage: " << path;
    if (!cmd.subcommands.empty())
        std::cout << " <command> [options]";
    else
    {
        if (!cmd.options.empty())
            std::cout << " [options]";
        for (const auto& p : cmd.positionals)
            std::cout << " " << (p.required ? "<" : "[") << p.name << (p.variadic ? "..." : "")
                      << (p.required ? ">" : "]");
    }
    std::cout << "\n\n" << cmd.summary << "\n";

    if (!cmd.subcommands.empty())
    {
        std::vector<std::pair<std::string, std::string>> rows;
        for (const auto& sub : cmd.subcommands)
        {
            std::vector<std::string> names{sub.name};
            names.insert(names.end(), sub.aliases.begin(), sub.aliases.end());
            rows.emplace_back(join(names, ", "), sub.summary);
        }
        std::cout << "\nCommands:\n";
        print_table(rows);
    }

    if (!cmd.positionals.empty())
    {
        std::vector<std::pair<std::string, std::string>> rows;
        for (const auto& p : cmd.positionals) rows.emplace_back(p.name, p.help);
        std::cout << "\nArguments:\n";
        print_table(rows);
    }

    std::vector<std::pair<std::string, std::string>> rows;
    for (const auto& o : cmd.options) rows.emplace_back(option_label(o), option_help(o));
    for (const auto& o : {help_option, verbose_option})
        rows.emplace_back(option_label(o), option_help(o));
    std::cout << "\nOptions:\n";
    print_table(rows);

    if (!cmd.subcommands.empty())
        std::cout << "\nRun \"" << path << " <command> --help\" for help on a command.\n";
}

const Command* find_subcommand(const Command& cmd, const std::string& name)
{
    for (const auto& sub : cmd.subcommands)
    {
        if (sub.name == name ||
            std::find(sub.aliases.begin(), sub.aliases.end(), name) != sub.aliases.end())
            return &sub;
    }
    return nullptr;
}

const Option* find_long(const Command& cmd, const std::string& name)
{
    for (const auto& o : cmd.options)
        if (o.name == name)
            return &o;
    return nullptr;
}

const Option* find_short(const Command& cmd, char name)
{
    for (const auto& o : cmd.options)
        if (o.short_name == name)
            return &o;
    return nullptr;
}

void set_option(Args& args, const Option& o, const std::string& value)
{
    if (!o.choices.empty() &&
        std::find(o.choices.begin(), o.choices.end(), value) == o.choices.end())
        throw UsageError("--" + o.name + " must be one of: " + join(o.choices, ", "));
    args.add(o.name, value);
}

// Returns false when help was requested.
bool parse_leaf(const Command& cmd, const std::vector<std::string>& tokens, std::size_t i,
                Args& args)
{
    std::vector<std::string> positionals;
    bool options_done = false;
    for (; i < tokens.size(); ++i)
    {
        const auto& token = tokens[i];
        if (options_done || token.size() < 2 || token[0] != '-')
        {
            positionals.push_back(token);
            continue;
        }
        if (token == "--")
        {
            options_done = true;
            continue;
        }
        if (token == "-h" || token == "--help")
            return false;
        if (token == "-v" || token == "--verbose")
        {
            verbose() = true;
            continue;
        }

        const Option* option = nullptr;
        std::optional<std::string> inline_value;
        if (token[1] == '-')
        {
            auto eq = token.find('=');
            auto name = token.substr(2, eq == std::string::npos ? std::string::npos : eq - 2);
            option = find_long(cmd, name);
            if (eq != std::string::npos)
                inline_value = token.substr(eq + 1);
        }
        else
        {
            option = find_short(cmd, token[1]);
            if (token.size() > 2)
                inline_value = token.substr(2);
        }
        if (!option)
            throw UsageError("unknown option '" + token + "'");

        if (option->value_name.empty())
        {
            if (inline_value)
                throw UsageError("--" + option->name + " does not take a value");
            args.add(option->name, "");
        }
        else if (inline_value)
            set_option(args, *option, *inline_value);
        else if (i + 1 < tokens.size())
            set_option(args, *option, tokens[++i]);
        else
            throw UsageError("--" + option->name + " needs a value");
    }

    std::size_t next = 0;
    for (const auto& p : cmd.positionals)
    {
        if (next >= positionals.size())
        {
            if (p.required)
                throw UsageError("missing <" + p.name + ">");
            break;
        }
        if (p.variadic)
            while (next < positionals.size()) args.add(p.name, positionals[next++]);
        else
            args.add(p.name, positionals[next++]);
    }
    if (next < positionals.size())
        throw UsageError("unexpected argument '" + positionals[next] + "'");
    return true;
}
} // namespace

std::optional<std::string> Args::value(const std::string& name) const
{
    auto it = values_.find(name);
    if (it == values_.end() || it->second.empty())
        return std::nullopt;
    return it->second.back();
}

std::vector<std::string> Args::values(const std::string& name) const
{
    auto it = values_.find(name);
    return it == values_.end() ? std::vector<std::string>{} : it->second;
}

bool& verbose()
{
    static bool on = false;
    return on;
}

int dispatch(const Command& root, int argc, char* argv[])
{
    std::vector<std::string> tokens(argv + 1, argv + argc);
    const Command* cmd = &root;
    std::string path = std::filesystem::path(argv[0]).filename().string();

    try
    {
        std::size_t i = 0;
        while (!cmd->subcommands.empty())
        {
            if (i == tokens.size())
            {
                print_help(*cmd, path);
                return 0;
            }
            const auto& token = tokens[i++];
            if (token == "-h" || token == "--help")
            {
                print_help(*cmd, path);
                return 0;
            }
            if (token == "-v" || token == "--verbose")
            {
                verbose() = true;
                continue;
            }
            if (token == "help")
            {
                for (; i < tokens.size(); ++i)
                {
                    const Command* sub = find_subcommand(*cmd, tokens[i]);
                    if (!sub)
                        throw UsageError("unknown command '" + tokens[i] + "'");
                    cmd = sub;
                    path += " " + sub->name;
                }
                print_help(*cmd, path);
                return 0;
            }
            if (!token.empty() && token[0] == '-')
                throw UsageError("unknown option '" + token + "'");
            const Command* sub = find_subcommand(*cmd, token);
            if (!sub)
                throw UsageError("unknown command '" + token + "'");
            cmd = sub;
            path += " " + sub->name;
        }

        Args args;
        if (!parse_leaf(*cmd, tokens, i, args))
        {
            print_help(*cmd, path);
            return 0;
        }
        return cmd->run(args);
    }
    catch (const UsageError& e)
    {
        std::cerr << path << ": error: " << e.what() << "\nRun \"" << path << " --help\" for usage."
                  << std::endl;
        return usage_error;
    }
    catch (const std::exception& e)
    {
        std::cerr << "lamss: error: " << e.what() << std::endl;
        return 1;
    }
}

} // namespace tool
} // namespace lamss
