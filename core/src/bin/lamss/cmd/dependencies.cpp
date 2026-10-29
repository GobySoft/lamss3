// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>
#include <iostream>
#include <set>
#include <sstream>
#include <unistd.h>

#include <yaml-cpp/yaml.h>

#include "command.h"
#include "deps.h"
#include "process.h"
#include "repo.h"
#include "style.h"

namespace
{
using namespace lamss::tool;

const fs::path apt_etc = "/etc/apt";

std::string detect_codename()
{
    const fs::path os_release = "/etc/os-release";
    if (fs::exists(os_release))
    {
        if (auto codename = parse_os_release_codename(read_file(os_release)))
            return *codename;
    }
    RunOptions quiet;
    quiet.discard_stderr = true;
    auto lsb = capture_output({"lsb_release", "-cs"}, quiet);
    auto codename = trim(lsb.out);
    if (lsb.status != 0 || codename.empty())
        throw Error("cannot determine the distribution codename (no VERSION_CODENAME in "
                    "/etc/os-release and lsb_release failed); use --distro");
    return codename;
}

template <typename Names>
std::vector<std::string> selected_toolboxes(const Repo& repo, const Names& requested)
{
    if (requested.empty())
        return repo.toolbox_list().active();

    std::vector<std::string> names(requested.begin(), requested.end());
    for (const auto& name : names)
    {
        validate_toolbox_name(name);
        if (!repo.has_toolbox(name))
            throw Error("no toolbox '" + name + "'");
    }
    std::vector<std::string> toolboxes{core_toolbox};
    auto requires_of = [&repo](const std::string& name) { return repo.requires_of(name); };
    for (const auto& step : plan_activation(names, requires_of)) toolboxes.push_back(step.name);
    return toolboxes;
}

Dependencies load_dependencies(const Repo& repo, const std::vector<std::string>& toolboxes)
{
    Dependencies merged;
    for (const auto& tb : toolboxes)
    {
        auto file = repo.root() / tb / "dependencies.yml";
        if (!fs::exists(file))
            continue;
        merge_dependencies(merged,
                           parse_dependencies(read_file(file), repo.root() / tb,
                                              (fs::path(tb) / "dependencies.yml").string()));
    }
    return merged;
}

// Paths in dependencies.yml are relative to the toolbox, or failing that to the repository root.
fs::path resolve(const Repo& repo, const fs::path& toolbox_dir, const std::string& path)
{
    for (const auto& candidate : {toolbox_dir / path, repo.root() / path})
        if (fs::exists(candidate))
            return candidate;
    throw Error("cannot find '" + path + "' in " + toolbox_dir.string() + " or " +
                repo.root().string());
}

// For listing: a missing file is shown rather than failing the whole list.
std::string display_path(const Repo& repo, const fs::path& toolbox_dir, const std::string& path)
{
    try
    {
        return resolve(repo, toolbox_dir, path).lexically_relative(repo.root()).string();
    }
    catch (const Error&)
    {
        return (toolbox_dir / path).lexically_relative(repo.root()).string() + " (missing)";
    }
}

std::vector<std::string> sorted_distros(const std::map<std::string, std::vector<std::string>>& apt)
{
    std::vector<std::string> distros;
    if (apt.count(all_distros_key))
        distros.push_back(all_distros_key);
    for (const auto& entry : apt)
        if (entry.first != all_distros_key)
            distros.push_back(entry.first);
    return distros;
}

void print_yaml(const Repo& repo, const Dependencies& deps,
                const std::map<std::string, std::vector<std::string>>& apt)
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "apt" << YAML::Value << YAML::BeginMap;
    for (const auto& [distro, packages] : apt)
        out << YAML::Key << distro << YAML::Value << packages;
    out << YAML::EndMap;

    if (!deps.apt_repositories.empty())
    {
        out << YAML::Key << "apt_repositories" << YAML::Value << YAML::BeginSeq;
        for (const auto& r : deps.apt_repositories)
        {
            out << YAML::BeginMap << YAML::Key << "name" << YAML::Value << r.name;
            out << YAML::Key << "url" << YAML::Value << r.url;
            out << YAML::Key << "key" << YAML::Value << display_path(repo, r.toolbox_dir, r.key);
            if (!r.components.empty())
                out << YAML::Key << "components" << YAML::Value << r.components;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
    }

    if (!deps.source.empty())
    {
        out << YAML::Key << "source" << YAML::Value << YAML::BeginSeq;
        for (const auto& s : deps.source)
            out << YAML::BeginMap << YAML::Key << "name" << YAML::Value << s.name << YAML::Key
                << "script" << YAML::Value << display_path(repo, s.toolbox_dir, s.script)
                << YAML::EndMap;
        out << YAML::EndSeq;
    }
    out << YAML::EndMap;
    std::cout << out.c_str() << std::endl;
}

std::set<std::string> installed_packages(const std::vector<std::string>& packages)
{
    std::set<std::string> installed;
    if (packages.empty())
        return installed;
    Argv query{"dpkg-query", "-W", "-f=${Package} ${Status}\\n"};
    query.insert(query.end(), packages.begin(), packages.end());
    RunOptions quiet;
    quiet.discard_stderr = true;
    std::istringstream lines(capture_output(query, quiet).out);
    for (std::string line; std::getline(lines, line);)
    {
        auto space = line.find(' ');
        if (space != std::string::npos && line.find("install ok installed") != std::string::npos)
            installed.insert(line.substr(0, space));
    }
    return installed;
}

std::string apt_sources_text()
{
    std::string text;
    auto add = [&text](const fs::path& file)
    {
        try
        {
            text += read_file(file) + "\n";
        }
        catch (const Error&)
        {
        }
    };
    if (fs::exists(apt_etc / "sources.list"))
        add(apt_etc / "sources.list");
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(apt_etc / "sources.list.d", ec))
    {
        auto ext = entry.path().extension();
        if (ext == ".list" || ext == ".sources")
            add(entry.path());
    }
    return text;
}

class Installer
{
  public:
    explicit Installer(bool dry_run) : dry_run_(dry_run) {}
    ~Installer()
    {
        std::error_code ec;
        if (!tmp_dir_.empty())
            fs::remove_all(tmp_dir_, ec);
    }

    Argv as_root(Argv command) const
    {
        if (geteuid() != 0)
            command.insert(command.begin(), "sudo");
        return command;
    }

    int step(const Argv& command, const RunOptions& options = {}) const
    {
        std::cout << style::command("+ " + format_command(command)) << std::endl;
        return dry_run_ ? 0 : run_process(command, options);
    }

    fs::path tmp_dir()
    {
        if (dry_run_)
            return "/tmp/lamss-dependencies";
        if (tmp_dir_.empty())
        {
            std::string pattern = (fs::temp_directory_path() / "lamss-XXXXXX").string();
            if (!mkdtemp(pattern.data()))
                throw Error("cannot create a temporary directory");
            tmp_dir_ = pattern;
        }
        return tmp_dir_;
    }

    void write(const fs::path& file, const std::string& contents) const
    {
        std::cout << style::command("+ echo " + format_command({trim(contents)}) + " > " +
                                    file.string())
                  << std::endl;
        if (!dry_run_)
            write_file(file, contents);
    }

  private:
    bool dry_run_;
    fs::path tmp_dir_;
};

int run_list(const Args& args)
{
    Repo repo(find_root());
    auto deps = load_dependencies(repo, selected_toolboxes(repo, args.values("toolbox")));

    std::map<std::string, std::vector<std::string>> apt;
    if (args.flag("all-distros"))
        apt = deps.apt;
    else
    {
        auto distro = args.value("distro");
        auto codename = distro ? *distro : detect_codename();
        apt[codename] = apt_packages_for(deps, codename);
    }

    if (args.value("format") == std::optional<std::string>("yaml"))
    {
        print_yaml(repo, deps, apt);
        return 0;
    }

    for (const auto& distro : sorted_distros(apt))
    {
        if (args.flag("all-distros"))
            std::cout << "# " << distro << std::endl;
        for (const auto& pkg : apt[distro]) std::cout << pkg << std::endl;
    }
    if (!deps.source.empty())
    {
        std::cerr << "Source dependencies (installed by `lamss dependencies install`):"
                  << std::endl;
        for (const auto& s : deps.source)
            std::cerr << "  " << s.name << ": " << display_path(repo, s.toolbox_dir, s.script)
                      << std::endl;
    }
    return 0;
}

int run_install(const Args& args)
{
    Repo repo(find_root());
    auto deps = load_dependencies(repo, selected_toolboxes(repo, args.values("toolbox")));
    auto codename = detect_codename();
    Installer installer(args.flag("dry-run"));

    bool repository_added = false;
    const auto sources = apt_sources_text();
    const auto keyrings = apt_etc / "keyrings";
    for (const auto& r : deps.apt_repositories)
    {
        if (sources_mention(sources, r.url))
            continue;
        auto key = resolve(repo, r.toolbox_dir, r.key);
        auto keyring = keyrings / (r.name + ".gpg");
        auto key_file = key;
        if (key.extension() == ".asc")
        {
            key_file = installer.tmp_dir() / (r.name + ".gpg");
            if (int status = installer.step(
                    {"gpg", "--dearmor", "--yes", "-o", key_file.string(), key.string()}))
                return status;
        }
        auto list = installer.tmp_dir() / (r.name + ".list");
        installer.write(list, apt_source_entry(r, codename, keyring) + "\n");

        for (const auto& command :
             {installer.as_root({"install", "-d", "-m", "0755", keyrings.string()}),
              installer.as_root({"install", "-m", "0644", key_file.string(), keyring.string()}),
              installer.as_root({"install", "-m", "0644", list.string(),
                                 (apt_etc / "sources.list.d" / (r.name + ".list")).string()})})
        {
            if (int status = installer.step(command))
                return status;
        }
        repository_added = true;
    }

    auto packages = apt_packages_for(deps, codename);
    auto installed = installed_packages(packages);
    std::vector<std::string> missing;
    for (const auto& pkg : packages)
        if (!installed.count(pkg))
            missing.push_back(pkg);

    if (repository_added || !missing.empty())
    {
        if (int status = installer.step(installer.as_root({"apt-get", "update"})))
            return status;
    }
    if (missing.empty())
    {
        std::cout << style::good("All " + std::to_string(packages.size()) + " apt packages for " +
                                 codename + " are installed.")
                  << std::endl;
    }
    else
    {
        Argv install{"apt-get", "install", "--no-install-recommends"};
        if (args.flag("yes"))
        {
            install.insert(install.begin(), {"env", "DEBIAN_FRONTEND=noninteractive"});
            install.push_back("-y");
        }
        install.insert(install.end(), missing.begin(), missing.end());
        if (int status = installer.step(installer.as_root(install)))
            return status;
    }

    if (!args.flag("apt-only"))
    {
        RunOptions at_root;
        at_root.cwd = repo.root();
        for (const auto& s : deps.source)
        {
            auto script = resolve(repo, s.toolbox_dir, s.script);
            if (int status = installer.step({script.string()}, at_root))
                return status;
        }
    }
    return 0;
}
} // namespace

lamss::tool::Command lamss::tool::dependencies_command()
{
    const Option toolbox{"toolbox",
                         0,
                         "TOOLBOX",
                         "Only this toolbox and the toolboxes it requires (default: the active "
                         "toolboxes)",
                         {},
                         true};
    return {"dependencies",
            {"dep"},
            "List and install the system packages the active toolboxes need",
            {},
            {},
            {{"list",
              {},
              "List the dependencies from the toolboxes' dependencies.yml files",
              {toolbox,
               {"all-distros", 0, "",
                "List the apt packages for every distribution, not just this one"},
               {"format",
                0,
                "FORMAT",
                "apt: apt packages one per line on stdout (source dependencies on stderr); "
                "yaml: the merged dependencies.yml",
                {"apt", "yaml"}},
               {"distro", 0, "CODENAME",
                "Distribution codename (default: this system's, from /etc/os-release)"}},
              {},
              {},
              run_list},
             {"install",
              {},
              "Add apt repositories, install apt packages and run source install scripts",
              {toolbox,
               {"apt-only", 0, "", "Do not run the source install scripts"},
               {"dry-run", 0, "", "Print the commands without running them"},
               {"yes", 'y', "", "Do not ask: pass -y to apt-get"}},
              {},
              {},
              run_install}}};
}
