// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <set>

#include "command.h"
#include "envfile.h"
#include "process.h"
#include "repo.h"
#include "style.h"

namespace
{
// Prepends entry, dropping any existing copy so repeated evals do not grow the variable.
std::string prepend_path(const std::string& entry, const char* variable)
{
    std::string result = entry;
    const char* current = std::getenv(variable);
    std::string rest = current ? current : "";
    std::size_t begin = 0;
    while (begin < rest.size())
    {
        auto end = rest.find(':', begin);
        if (end == std::string::npos)
            end = rest.size();
        auto item = rest.substr(begin, end - begin);
        if (!item.empty() && item != entry)
            result += ":" + item;
        begin = end + 1;
    }
    return result;
}

void print_export(const std::string& variable, const std::string& value)
{
    std::cout << "export " << variable << "=" << lamss::tool::format_command({value}) << "\n";
}
using namespace lamss::tool;

fs::path env_sh_path() { return config_dir() / "env.sh"; }
fs::path checkouts_path() { return config_dir() / "checkouts"; }

std::string read_if_exists(const fs::path& file) { return fs::exists(file) ? read_file(file) : ""; }

std::optional<fs::path> active_root() { return env_sh_root(read_if_exists(env_sh_path())); }

bool is_built(const fs::path& root) { return fs::exists(root / "build" / "usr" / "bin" / "lamss"); }

int run_show(const Args&)
{
    Repo repo(find_root());
    const auto prefix = repo.prefix();
    print_export("LAMSS_ROOT", repo.root().string());
    print_export("LAMSS_PREFIX", prefix.string());
    print_export("PATH", prepend_path((prefix / "bin").string(), "PATH"));
    print_export("LD_LIBRARY_PATH", prepend_path((prefix / "lib").string(), "LD_LIBRARY_PATH"));
    print_export("PYTHONPATH", prepend_path((prefix / "lib" / "python3" / "dist-packages").string(),
                                            "PYTHONPATH"));
    return 0;
}

int run_list(const Args&)
{
    auto checkouts = parse_checkouts(read_if_exists(checkouts_path()));
    const auto active = active_root();
    if (active && std::find(checkouts.begin(), checkouts.end(), *active) == checkouts.end())
        checkouts.push_back(*active);
    if (checkouts.empty())
    {
        std::cout << "No checkouts yet: run ./init.sh, or `lamss env switch` in a checkout."
                  << std::endl;
        return 0;
    }
    for (const auto& root : checkouts)
    {
        std::string state = !fs::is_directory(root / ".lamss") ? "missing"
                            : is_built(root)                   ? "built"
                                                               : "not built";
        const bool is_active = active && *active == root;
        const std::string shown_state = state == "missing" ? style::error(state, style::Stream::out)
                                        : state == "not built" ? style::attention(state)
                                                               : state;
        std::cout << (is_active ? style::good("* ") : "  ") << root.string() << " (" << shown_state
                  << ")" << std::endl;
    }
    return 0;
}

fs::path resolve_checkout(const std::string& target)
{
    if (fs::is_directory(target))
    {
        auto root = find_root_from(fs::canonical(target));
        if (!root)
            throw Error(target + " is not inside a lamss3 checkout");
        return *root;
    }
    std::vector<fs::path> matches;
    for (const auto& root : parse_checkouts(read_if_exists(checkouts_path())))
        if (root.filename() == target)
            matches.push_back(root);
    if (matches.empty())
        throw Error("no checkout '" + target + "'; give a path, or see `lamss env list`");
    if (matches.size() > 1)
    {
        std::string all;
        for (const auto& m : matches) all += "\n  " + m.string();
        throw Error("'" + target + "' matches more than one checkout; give the path:" + all);
    }
    return matches.front();
}

int run_switch(const Args& args)
{
    auto target = args.value("checkout");
    const fs::path root = fs::canonical(target ? resolve_checkout(*target) : find_root());

    fs::create_directories(config_dir());
    auto checkouts = parse_checkouts(read_if_exists(checkouts_path()));
    if (std::find(checkouts.begin(), checkouts.end(), root) == checkouts.end())
    {
        checkouts.push_back(root);
        write_file(checkouts_path(), render_checkouts(checkouts));
    }
    write_file(env_sh_path(), render_env_sh(root, env_sh_path()));

    std::cout << style::good("Active lamss3 checkout: " + root.string()) << std::endl;
    if (!is_built(root))
        std::cout << style::attention("It is not built yet: run ./init.sh in it.") << std::endl;
    std::cout << "New shells use it; to update a shell that did not load " << env_sh_path().string()
              << ", run: . " << format_command({env_sh_path().string()}) << std::endl;
    return 0;
}

std::vector<fs::path> default_rc_files()
{
    const char* home = std::getenv("HOME");
    if (!home || !*home)
        throw Error("HOME is not set; use --rc");
    std::vector<fs::path> files{fs::path(home) / ".bashrc"};
    if (fs::exists(fs::path(home) / ".zshrc"))
        files.push_back(fs::path(home) / ".zshrc");
    return files;
}

int run_hook(const Args& args)
{
    const bool remove = args.flag("remove");
    std::vector<fs::path> files;
    for (const auto& rc : args.values("rc")) files.emplace_back(rc);
    if (files.empty())
        files = default_rc_files();

    for (const auto& rc : files)
    {
        const auto before = read_if_exists(rc);
        const auto after = remove ? remove_hook(before) : add_hook(before);
        if (after == before)
        {
            std::cout << (remove ? "Not in " : "Already in ") << rc.string() << std::endl;
            continue;
        }
        write_file(rc, after);
        std::cout << style::good(std::string(remove ? "Removed the lamss block from "
                                                    : "Added the lamss block to ") +
                                 rc.string())
                  << std::endl;
    }
    return 0;
}
} // namespace

lamss::tool::Command lamss::tool::env_command()
{
    return {
        "env",
        {},
        "Set up shells to use a lamss3 checkout, and switch between checkouts",
        {},
        {},
        {{"show",
          {},
          "Print shell exports for the checkout you are in: eval \"$(lamss env show)\"",
          {},
          {},
          {},
          run_show},
         {"list",
          {},
          "List the checkouts lamss env knows about; * marks the active one",
          {},
          {},
          {},
          run_list},
         {"switch",
          {},
          "Make a checkout the active one for new shells (and this one, with the shell hook)",
          {},
          {{"checkout", "Path, or name from `lamss env list` (default: the checkout you are in)"}},
          {},
          run_switch},
         {"hook",
          {},
          "Add (or --remove) the block in ~/.bashrc and ~/.zshrc that loads the active "
          "checkout",
          {{"remove", 0, "", "Remove the block instead"},
           {"rc", 0, "FILE", "Edit FILE instead of ~/.bashrc (and ~/.zshrc if present)", {}, true}},
          {},
          {},
          run_hook}}};
}
