// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <regex>
#include <sstream>

#include <yaml-cpp/yaml.h>

#include "deps.h"
#include "repo.h"

namespace lamss
{
namespace tool
{
namespace
{
void append_unique(std::vector<std::string>& to, const std::string& value)
{
    if (std::find(to.begin(), to.end(), value) == to.end())
        to.push_back(value);
}

class Parser
{
  public:
    explicit Parser(std::string file_name) : file_name_(std::move(file_name)) {}

    [[noreturn]] void fail(const std::string& what) const { throw Error(file_name_ + ": " + what); }

    std::string scalar(const YAML::Node& node, const std::string& where) const
    {
        if (!node || !node.IsScalar() || node.Scalar().empty())
            fail(where + " must be a non-empty string");
        return node.Scalar();
    }

    std::vector<std::string> scalars(const YAML::Node& node, const std::string& where) const
    {
        std::vector<std::string> values;
        if (!node || node.IsNull())
            return values;
        if (!node.IsSequence())
            fail(where + " must be a list");
        for (const auto& item : node) values.push_back(scalar(item, where + " entry"));
        return values;
    }

    void check_keys(const YAML::Node& map, const std::vector<std::string>& allowed,
                    const std::string& where) const
    {
        for (const auto& entry : map)
        {
            auto key = entry.first.as<std::string>();
            if (std::find(allowed.begin(), allowed.end(), key) == allowed.end())
                fail("unknown key '" + key + "' in " + where);
        }
    }

  private:
    std::string file_name_;
};
} // namespace

Dependencies parse_dependencies(const std::string& yaml, const std::filesystem::path& toolbox_dir,
                                const std::string& file_name)
{
    Parser parser(file_name);
    YAML::Node root;
    try
    {
        root = YAML::Load(yaml);
    }
    catch (const YAML::Exception& e)
    {
        parser.fail(e.what());
    }

    Dependencies deps;
    if (!root || root.IsNull())
        return deps;
    if (!root.IsMap())
        parser.fail("expected a mapping at the top level");
    parser.check_keys(root, {"apt", "apt_repositories", "source"}, "the top level");

    if (const auto apt = root["apt"])
    {
        if (!apt.IsMap())
            parser.fail("apt must map 'all' or a distribution codename to a list of packages");
        for (const auto& entry : apt)
        {
            auto distro = entry.first.as<std::string>();
            auto& packages = deps.apt[distro];
            for (const auto& pkg : parser.scalars(entry.second, "apt." + distro))
                append_unique(packages, pkg);
        }
    }

    if (const auto repos = root["apt_repositories"])
    {
        if (!repos.IsSequence())
            parser.fail("apt_repositories must be a list");
        for (const auto& item : repos)
        {
            if (!item.IsMap())
                parser.fail("each apt_repositories entry must be a mapping");
            parser.check_keys(item, {"name", "url", "key", "components"}, "apt_repositories");
            AptRepository repo;
            repo.name = parser.scalar(item["name"], "apt_repositories.name");
            repo.url = parser.scalar(item["url"], "apt_repositories.url");
            repo.key = parser.scalar(item["key"], "apt_repositories.key");
            repo.components = parser.scalars(item["components"], "apt_repositories.components");
            repo.toolbox_dir = toolbox_dir;
            deps.apt_repositories.push_back(repo);
        }
    }

    if (const auto sources = root["source"])
    {
        if (!sources.IsSequence())
            parser.fail("source must be a list");
        for (const auto& item : sources)
        {
            if (!item.IsMap())
                parser.fail("each source entry must be a mapping");
            parser.check_keys(item, {"name", "script"}, "source");
            SourceDependency source;
            source.name = parser.scalar(item["name"], "source.name");
            source.script = parser.scalar(item["script"], "source.script");
            source.toolbox_dir = toolbox_dir;
            deps.source.push_back(source);
        }
    }

    return deps;
}

void merge_dependencies(Dependencies& into, const Dependencies& from)
{
    for (const auto& [distro, packages] : from.apt)
    {
        auto& merged = into.apt[distro];
        for (const auto& pkg : packages) append_unique(merged, pkg);
    }

    for (const auto& repo : from.apt_repositories)
    {
        auto same_name = [&](const AptRepository& r) { return r.name == repo.name; };
        if (std::none_of(into.apt_repositories.begin(), into.apt_repositories.end(), same_name))
            into.apt_repositories.push_back(repo);
    }

    for (const auto& source : from.source)
    {
        auto same_name = [&](const SourceDependency& s) { return s.name == source.name; };
        if (std::none_of(into.source.begin(), into.source.end(), same_name))
            into.source.push_back(source);
    }
}

std::vector<std::string> apt_packages_for(const Dependencies& deps, const std::string& codename)
{
    std::vector<std::string> packages;
    for (const auto& distro : {std::string(all_distros_key), codename})
    {
        auto it = deps.apt.find(distro);
        if (it == deps.apt.end())
            continue;
        for (const auto& pkg : it->second) append_unique(packages, pkg);
    }
    return packages;
}

std::optional<std::string> parse_os_release_codename(const std::string& os_release)
{
    static const std::regex line(R"(^VERSION_CODENAME=["']?([^"'\s]*)["']?\s*$)");
    std::istringstream is(os_release);
    for (std::string text; std::getline(is, text);)
    {
        std::smatch match;
        if (std::regex_match(text, match, line) && match[1].length() > 0)
            return match[1].str();
    }
    return std::nullopt;
}

std::string apt_source_entry(const AptRepository& repo, const std::string& codename,
                             const std::filesystem::path& keyring)
{
    std::string entry = "deb [signed-by=" + keyring.string() + "] " + repo.url + " ";
    if (repo.components.empty())
        return entry + codename + "/";
    entry += codename;
    for (const auto& component : repo.components) entry += " " + component;
    return entry;
}

bool sources_mention(const std::string& sources_text, const std::string& url)
{
    std::string bare = url;
    while (!bare.empty() && bare.back() == '/') bare.pop_back();
    std::istringstream is(sources_text);
    for (std::string line; std::getline(is, line);)
    {
        auto text = trim(line);
        if (text.empty() || text[0] == '#')
            continue;
        if (text.find(bare) != std::string::npos)
            return true;
    }
    return false;
}

} // namespace tool
} // namespace lamss
