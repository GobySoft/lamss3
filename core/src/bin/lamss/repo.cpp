// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>

#include "repo.h"

namespace lamss
{
namespace tool
{
namespace
{
const char* whitespace = " \t\r\n";

std::string list_entry(const std::string& line)
{
    if (line.empty() || line[0] == '#')
        return "";
    return trim(line);
}

std::vector<std::string> split_lines(const std::string& text)
{
    std::vector<std::string> lines;
    std::istringstream is(text);
    for (std::string line; std::getline(is, line);) lines.push_back(line);
    return lines;
}

bool contains(const std::vector<std::string>& v, const std::string& s)
{
    return std::find(v.begin(), v.end(), s) != v.end();
}
} // namespace

std::string trim(const std::string& s)
{
    auto begin = s.find_first_not_of(whitespace);
    if (begin == std::string::npos)
        return "";
    auto end = s.find_last_not_of(whitespace);
    return s.substr(begin, end - begin + 1);
}

std::string read_file(const fs::path& file)
{
    std::ifstream in(file, std::ios::binary);
    if (!in)
        throw Error("cannot read " + file.string());
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void write_file(const fs::path& file, const std::string& contents)
{
    fs::path tmp = file;
    tmp += ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        out << contents;
        if (!out.flush())
            throw Error("cannot write " + tmp.string());
    }
    fs::rename(tmp, file);
}

std::optional<fs::path> find_root_from(fs::path start)
{
    std::error_code ec;
    fs::path dir = fs::weakly_canonical(start, ec);
    if (ec)
        dir = start;
    while (true)
    {
        if (fs::is_directory(dir / ".lamss", ec))
            return dir;
        if (!dir.has_parent_path() || dir.parent_path() == dir)
            return std::nullopt;
        dir = dir.parent_path();
    }
}

fs::path find_root()
{
    const char* env_root = std::getenv("LAMSS_ROOT");
    if (env_root && *env_root)
    {
        fs::path root(env_root);
        if (!fs::is_directory(root / ".lamss"))
            throw Error("LAMSS_ROOT=" + root.string() + " has no .lamss/ directory");
        return fs::weakly_canonical(root);
    }

    if (auto root = find_root_from(fs::current_path()))
        return *root;

    std::error_code ec;
    fs::path exe = fs::read_symlink("/proc/self/exe", ec);
    if (!ec)
    {
        if (auto root = find_root_from(exe.parent_path()))
            return *root;
    }

    throw Error("not inside a lamss3 repository (no .lamss/ in the current directory or any "
                "parent); cd into one or set LAMSS_ROOT");
}

ToolboxList::ToolboxList(const std::string& text) : lines_(split_lines(text)) {}

ToolboxList ToolboxList::read(const fs::path& file)
{
    if (!fs::exists(file))
        return ToolboxList();
    return ToolboxList(read_file(file));
}

std::vector<std::string> ToolboxList::active() const
{
    std::vector<std::string> names{core_toolbox};
    for (const auto& line : lines_)
    {
        auto name = list_entry(line);
        if (!name.empty() && !contains(names, name))
            names.push_back(name);
    }
    return names;
}

bool ToolboxList::is_active(const std::string& name) const { return contains(active(), name); }

bool ToolboxList::activate(const std::string& name)
{
    if (is_active(name))
        return false;
    lines_.push_back(name);
    return true;
}

bool ToolboxList::deactivate(const std::string& name)
{
    auto it = std::remove_if(lines_.begin(), lines_.end(),
                             [&](const std::string& line) { return list_entry(line) == name; });
    bool removed = it != lines_.end();
    lines_.erase(it, lines_.end());
    return removed;
}

std::string ToolboxList::str() const
{
    std::string out;
    for (const auto& line : lines_) out += line + "\n";
    return out;
}

std::optional<ToolboxDeclaration> parse_toolbox_declaration(const std::string& cmakelists)
{
    std::string code;
    for (const auto& line : split_lines(cmakelists))
    {
        auto hash = line.find('#');
        code += (hash == std::string::npos ? line : line.substr(0, hash)) + "\n";
    }

    static const std::regex call(R"((^|[^A-Za-z0-9_])lamss_toolbox\s*\(([^)]*)\))",
                                 std::regex::icase);
    std::smatch match;
    if (!std::regex_search(code, match, call))
        return std::nullopt;

    std::istringstream args(match[2].str());
    ToolboxDeclaration decl;
    std::string keyword;
    for (std::string token; args >> token;)
    {
        if (token.size() >= 2 && token.front() == '"' && token.back() == '"')
            token = token.substr(1, token.size() - 2);
        if (token == "NAME" || token == "REQUIRES")
            keyword = token;
        else if (keyword == "NAME" && decl.name.empty())
            decl.name = token;
        else if (keyword == "REQUIRES" && !contains(decl.required, token))
            decl.required.push_back(token);
    }
    return decl;
}

std::vector<std::string> parse_gitmodules_paths(const std::string& gitmodules)
{
    static const std::regex path_line(R"(^\s*path\s*=\s*(.*?)\s*$)");
    std::vector<std::string> paths;
    for (const auto& line : split_lines(gitmodules))
    {
        std::smatch match;
        if (std::regex_match(line, match, path_line))
            paths.push_back(match[1].str());
    }
    return paths;
}

std::vector<Activation> plan_activation(const std::vector<std::string>& names,
                                        const RequiresFunction& requires_of)
{
    std::vector<Activation> plan;
    std::set<std::string> visiting;
    std::function<void(const std::string&, const std::string&)> visit =
        [&](const std::string& name, const std::string& required_by)
    {
        if (name == core_toolbox || visiting.count(name))
            return;
        visiting.insert(name);
        for (const auto& req : requires_of(name)) visit(req, name);
        plan.push_back({name, required_by});
    };
    for (const auto& name : names) visit(name, "");
    return plan;
}

std::vector<std::string> plan_deactivation(const std::vector<std::string>& names,
                                           const std::vector<std::string>& active,
                                           const RequiresFunction& requires_of, bool force)
{
    std::set<std::string> removing(names.begin(), names.end());
    for (bool changed = true; changed;)
    {
        changed = false;
        for (const auto& tb : active)
        {
            if (removing.count(tb))
                continue;
            for (const auto& req : requires_of(tb))
            {
                if (!removing.count(req))
                    continue;
                if (!force)
                    throw Error("'" + tb + "' requires '" + req + "'; deactivate '" + tb +
                                "' too, or use --force");
                removing.insert(tb);
                changed = true;
                break;
            }
        }
    }

    std::vector<std::string> plan;
    for (const auto& name : names)
        if (!contains(plan, name))
            plan.push_back(name);
    for (const auto& tb : active)
        if (removing.count(tb) && !contains(plan, tb))
            plan.push_back(tb);
    return plan;
}

void Repo::write_toolbox_list(const ToolboxList& list) const
{
    write_file(toolboxes_file(), list.str());
}

bool Repo::has_toolbox(const std::string& name) const
{
    return fs::exists(root_ / name / "CMakeLists.txt");
}

std::vector<std::string> Repo::requires_of(const std::string& name) const
{
    auto cmakelists = root_ / name / "CMakeLists.txt";
    if (!fs::exists(cmakelists))
        return {};
    auto decl = parse_toolbox_declaration(read_file(cmakelists));
    return decl ? decl->required : std::vector<std::string>();
}

std::vector<std::string> Repo::submodule_paths() const
{
    auto gitmodules = root_ / ".gitmodules";
    if (!fs::exists(gitmodules))
        return {};
    return parse_gitmodules_paths(read_file(gitmodules));
}

bool Repo::is_submodule(const std::string& name) const { return contains(submodule_paths(), name); }

std::vector<std::string> Repo::known_toolboxes() const
{
    std::set<std::string> names;
    for (const auto& entry : fs::directory_iterator(root_))
    {
        auto name = entry.path().filename().string();
        if (!entry.is_directory() || name.front() == '.' || name == "build")
            continue;
        auto cmakelists = entry.path() / "CMakeLists.txt";
        if (fs::exists(cmakelists) && parse_toolbox_declaration(read_file(cmakelists)))
            names.insert(name);
    }
    for (const auto& path : submodule_paths())
        if (path.find('/') == std::string::npos)
            names.insert(path);
    for (const auto& name : toolbox_list().active()) names.insert(name);

    names.erase(core_toolbox);
    std::vector<std::string> result{core_toolbox};
    result.insert(result.end(), names.begin(), names.end());
    return result;
}

void validate_toolbox_name(const std::string& name)
{
    static const std::regex valid("[A-Za-z0-9_][A-Za-z0-9_.-]*");
    if (!std::regex_match(name, valid))
        throw Error("invalid toolbox name '" + name + "'");
}

} // namespace tool
} // namespace lamss
