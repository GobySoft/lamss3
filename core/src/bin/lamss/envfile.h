// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LAMSS_TOOL_ENVFILE_H
#define LAMSS_TOOL_ENVFILE_H

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace lamss
{
namespace tool
{
// $XDG_CONFIG_HOME/lamss, or ~/.config/lamss
std::filesystem::path config_dir();

// The script shells source at startup to use the active checkout.
std::string render_env_sh(const std::filesystem::path& root,
                          const std::filesystem::path& env_sh_path);
std::optional<std::filesystem::path> env_sh_root(const std::string& env_sh);

bool has_hook(const std::string& rc);
std::string add_hook(const std::string& rc);
std::string remove_hook(const std::string& rc);

std::vector<std::filesystem::path> parse_checkouts(const std::string& text);
std::string render_checkouts(const std::vector<std::filesystem::path>& checkouts);

} // namespace tool
} // namespace lamss

#endif
