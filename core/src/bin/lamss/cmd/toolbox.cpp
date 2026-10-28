// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <iomanip>
#include <iostream>

#include "command.h"
#include "process.h"
#include "repo.h"

namespace
{
using namespace lamss::tool;

RequiresFunction requires_in(const Repo& repo)
{
    return [&repo](const std::string& name) { return repo.requires_of(name); };
}

std::string git_describe(const fs::path& dir)
{
    RunOptions options;
    options.discard_stderr = true;
    auto result = capture_output(
        {"git", "-C", dir.string(), "describe", "--always", "--dirty", "--tags"}, options);
    auto described = trim(result.out);
    return result.status == 0 && !described.empty() ? described : "-";
}

int activate(const Repo& repo, const std::vector<std::string>& names)
{
    std::vector<std::string> requested;
    for (const auto& name : names)
    {
        validate_toolbox_name(name);
        if (name == core_toolbox)
            std::cout << "core is always active" << std::endl;
        else if (!repo.has_toolbox(name))
            throw Error("no toolbox '" + name + "' (no " + (repo.root() / name).string() +
                        "/CMakeLists.txt); run `lamss toolbox add` to add it");
        else
            requested.push_back(name);
    }

    auto plan = plan_activation(requested, requires_in(repo));
    for (const auto& step : plan)
    {
        if (!repo.has_toolbox(step.name))
            throw Error("'" + step.required_by + "' requires '" + step.name +
                        "', which is not present; run `lamss toolbox add " + step.name + "`");
    }

    auto list = repo.toolbox_list();
    bool changed = false;
    for (const auto& step : plan)
    {
        if (list.activate(step.name))
        {
            changed = true;
            std::cout << "activated " << step.name;
            if (!step.required_by.empty())
                std::cout << " (note: required by " << step.required_by << ")";
            std::cout << std::endl;
        }
        else if (step.required_by.empty())
        {
            std::cout << step.name << " is already active" << std::endl;
        }
    }
    if (changed)
        repo.write_toolbox_list(list);
    return 0;
}

void deactivate(const Repo& repo, const std::vector<std::string>& names, bool force)
{
    auto list = repo.toolbox_list();
    auto active = list.active();

    std::vector<std::string> requested;
    for (const auto& name : names)
    {
        validate_toolbox_name(name);
        if (name == core_toolbox)
            throw Error("core is always active and cannot be deactivated");
        if (list.is_active(name))
            requested.push_back(name);
        else
            std::cout << name << " is not active" << std::endl;
    }

    auto plan = plan_deactivation(requested, active, requires_in(repo), force);
    for (const auto& name : plan)
    {
        list.deactivate(name);
        std::cout << "deactivated " << name;
        if (std::find(requested.begin(), requested.end(), name) == requested.end())
            std::cout << " (note: it requires a deactivated toolbox)";
        std::cout << std::endl;
    }
    if (!plan.empty())
        repo.write_toolbox_list(list);
}

bool strip_suffix(std::string& s, const std::string& suffix)
{
    if (s.size() <= suffix.size() ||
        s.compare(s.size() - suffix.size(), suffix.size(), suffix) != 0)
        return false;
    s.erase(s.size() - suffix.size());
    return true;
}

// Same rule as `git clone`: the last path component without a trailing "/.git" or ".git".
std::string url_basename(std::string url)
{
    while (strip_suffix(url, "/")) {}
    strip_suffix(url, "/.git");
    while (strip_suffix(url, "/")) {}
    auto slash = url.find_last_of("/:");
    auto name = slash == std::string::npos ? url : url.substr(slash + 1);
    strip_suffix(name, ".git");
    return name;
}

int run_list(const Args&)
{
    Repo repo(find_root());
    auto list = repo.toolbox_list();
    const auto root_describe = git_describe(repo.root());

    const int name_width = 16, kind_width = 11, active_width = 8;
    std::cout << std::left << std::setw(name_width) << "NAME" << std::setw(kind_width) << "KIND"
              << std::setw(active_width) << "ACTIVE" << std::setw(name_width) << "PATH" << "VERSION"
              << std::endl;

    for (const auto& name : repo.known_toolboxes())
    {
        const bool present = repo.has_toolbox(name);
        const bool submodule = repo.is_submodule(name);
        std::string kind = !present ? "missing" : submodule ? "submodule" : "in-tree";
        std::string version = !present    ? "-"
                              : submodule ? git_describe(repo.root() / name)
                                          : root_describe;
        std::cout << std::setw(name_width) << name << std::setw(kind_width) << kind
                  << std::setw(active_width) << (list.is_active(name) ? "yes" : "no")
                  << std::setw(name_width) << name + "/" << version << std::endl;
    }
    return 0;
}

int run_add(const Args& args)
{
    Repo repo(find_root());
    const auto root = repo.root().string();
    const auto source = *args.value("source");
    const auto given_name = args.value("name");

    if (source.find_first_of("/:") == std::string::npos)
    {
        validate_toolbox_name(source);
        if (given_name && *given_name != source)
            throw Error("a name can only be given with a git URL");
        auto submodules = repo.submodule_paths();
        bool has_submodules =
            std::any_of(submodules.begin(), submodules.end(), [&](const std::string& path)
                        { return path == source || path.rfind(source + "/", 0) == 0; });
        if (has_submodules)
        {
            if (int status = run_process({"git", "-C", root, "submodule", "update", "--init",
                                          "--recursive", "--", source}))
                return status;
        }
        return activate(repo, {source});
    }

    const auto name = given_name ? *given_name : url_basename(source);
    validate_toolbox_name(name);
    if (fs::exists(repo.root() / name))
        throw Error((repo.root() / name).string() + " already exists");
    if (int status = run_process({"git", "-C", root, "submodule", "add", "--", source, name}))
        return status;
    if (!repo.has_toolbox(name))
        throw Error("added submodule '" + name +
                    "', but it has no CMakeLists.txt so it was not activated");
    return activate(repo, {name});
}

int run_remove(const Args& args)
{
    Repo repo(find_root());
    const auto name = *args.value("name");
    validate_toolbox_name(name);
    if (!repo.is_submodule(name))
        throw Error("'" + name + "' is not a submodule toolbox; use `lamss toolbox " +
                    "deactivate " + name + "` for in-tree toolboxes");

    deactivate(repo, {name}, args.flag("force"));

    const auto root = repo.root().string();
    if (int status = run_process({"git", "-C", root, "submodule", "deinit", "-f", "--", name}))
        return status;
    return run_process({"git", "-C", root, "rm", "-f", "--", name});
}

int run_activate(const Args& args) { return activate(Repo(find_root()), args.values("name")); }

int run_deactivate(const Args& args)
{
    deactivate(Repo(find_root()), args.values("name"), args.flag("force"));
    return 0;
}
} // namespace

lamss::tool::Command lamss::tool::toolbox_command()
{
    const Positional names{"name", "Toolboxes", true, true};
    return {
        "toolbox",
        {"tb"},
        "List, add, remove, activate and deactivate toolboxes",
        {},
        {},
        {{"list",
          {},
          "List known toolboxes: kind, active, path and git describe",
          {},
          {},
          {},
          run_list},
         {"add",
          {},
          "Add an external toolbox as a git submodule (or an in-tree one by name) and "
          "activate it",
          {},
          {{"source", "Git URL of an external toolbox, or the name of an in-tree toolbox", true},
           {"name", "Directory (and toolbox) name for a git URL (default: the repository name)"}},
          {},
          run_add},
         {"remove",
          {},
          "Deactivate an external toolbox and remove its submodule",
          {{"force", 0, "", "Also deactivate active toolboxes that require it"}},
          {{"name", "Submodule toolbox to remove", true}},
          {},
          run_remove},
         {"activate",
          {},
          "Activate toolboxes and the toolboxes they require",
          {},
          {names},
          {},
          run_activate},
         {"deactivate",
          {},
          "Deactivate toolboxes",
          {{"force", 0, "", "Also deactivate active toolboxes that require these"}},
          {names},
          {},
          run_deactivate}}};
}
