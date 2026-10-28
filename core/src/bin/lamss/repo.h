// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LAMSS_TOOL_REPO_H
#define LAMSS_TOOL_REPO_H

#include <filesystem>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace lamss
{
namespace tool
{
namespace fs = std::filesystem;

class Error : public std::runtime_error
{
  public:
    using std::runtime_error::runtime_error;
};

constexpr const char* core_toolbox = "core";

std::string trim(const std::string& s);
std::string read_file(const fs::path& file);
void write_file(const fs::path& file, const std::string& contents);

std::optional<fs::path> find_root_from(fs::path start);
// LAMSS_ROOT, then the current directory, then the directory of the running binary
fs::path find_root();

// Line-preserving editor for .lamss/toolboxes; core is implicit and never written.
class ToolboxList
{
  public:
    explicit ToolboxList(const std::string& text = "");
    static ToolboxList read(const fs::path& file);

    std::vector<std::string> active() const;
    bool is_active(const std::string& name) const;
    bool activate(const std::string& name);
    bool deactivate(const std::string& name);
    std::string str() const;

  private:
    std::vector<std::string> lines_;
};

struct ToolboxDeclaration
{
    std::string name;
    std::vector<std::string> required;
};

// Parses the lamss_toolbox(NAME <tb> REQUIRES <tb>...) call from a toolbox's CMakeLists.txt.
std::optional<ToolboxDeclaration> parse_toolbox_declaration(const std::string& cmakelists);

std::vector<std::string> parse_gitmodules_paths(const std::string& gitmodules);

using RequiresFunction = std::function<std::vector<std::string>(const std::string&)>;

struct Activation
{
    std::string name;
    std::string required_by;
};

// Requested toolboxes plus everything they transitively require (except core), requirements first.
std::vector<Activation> plan_activation(const std::vector<std::string>& names,
                                        const RequiresFunction& requires_of);

// Requested toolboxes plus, with force, every active toolbox that transitively requires one.
// Throws without force if an active toolbox requires one of them.
std::vector<std::string> plan_deactivation(const std::vector<std::string>& names,
                                           const std::vector<std::string>& active,
                                           const RequiresFunction& requires_of, bool force);

class Repo
{
  public:
    explicit Repo(fs::path root) : root_(std::move(root)) {}

    const fs::path& root() const { return root_; }
    fs::path toolboxes_file() const { return root_ / ".lamss" / "toolboxes"; }
    fs::path build_dir() const { return root_ / "build"; }
    fs::path prefix() const { return build_dir() / "usr"; }

    ToolboxList toolbox_list() const { return ToolboxList::read(toolboxes_file()); }
    void write_toolbox_list(const ToolboxList& list) const;

    bool has_toolbox(const std::string& name) const;
    std::vector<std::string> requires_of(const std::string& name) const;
    std::vector<std::string> submodule_paths() const;
    bool is_submodule(const std::string& name) const;
    // core first, then every in-tree, submodule or listed toolbox, sorted
    std::vector<std::string> known_toolboxes() const;

  private:
    fs::path root_;
};

void validate_toolbox_name(const std::string& name);

} // namespace tool
} // namespace lamss

#endif
