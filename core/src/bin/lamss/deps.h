// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LAMSS_TOOL_DEPS_H
#define LAMSS_TOOL_DEPS_H

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace lamss
{
namespace tool
{
constexpr const char* all_distros_key = "all";

struct AptRepository
{
    std::string name;
    std::string url;
    std::string key;
    std::vector<std::string> components;
    std::filesystem::path toolbox_dir;
};

struct SourceDependency
{
    std::string name;
    std::string script;
    std::filesystem::path toolbox_dir;
};

// One toolbox's dependencies.yml, or the union over several.
struct Dependencies
{
    // "all" and distribution codenames to package names, in first-seen order without duplicates
    std::map<std::string, std::vector<std::string>> apt;
    std::vector<AptRepository> apt_repositories;
    std::vector<SourceDependency> source;
};

// Throws Error with the file name on malformed input.
Dependencies parse_dependencies(const std::string& yaml, const std::filesystem::path& toolbox_dir,
                                const std::string& file_name = "dependencies.yml");

// Repositories and sources are de-duplicated by name; the first definition wins.
void merge_dependencies(Dependencies& into, const Dependencies& from);

// apt.all plus apt.<codename>
std::vector<std::string> apt_packages_for(const Dependencies& deps, const std::string& codename);

std::optional<std::string> parse_os_release_codename(const std::string& os_release);

// One line for /etc/apt/sources.list.d: "<url> <codename>/" for flat repositories (no components).
std::string apt_source_entry(const AptRepository& repo, const std::string& codename,
                             const std::filesystem::path& keyring);

// Whether any sources text mentions this repository's URL (ignoring a trailing slash).
bool sources_mention(const std::string& sources_text, const std::string& url);

} // namespace tool
} // namespace lamss

#endif
