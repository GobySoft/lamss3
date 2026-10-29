// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>
#include <sstream>

#include "envfile.h"
#include "process.h"
#include "repo.h"

namespace lamss
{
namespace tool
{
namespace
{
const std::string root_marker = "# lamss-root: ";
const std::string hook_begin = "# >>> lamss >>>";
const std::string hook_end = "# <<< lamss <<<";

std::string quote(const fs::path& path) { return format_command({path.string()}); }
} // namespace

fs::path config_dir()
{
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg)
        return fs::path(xdg) / "lamss";
    const char* home = std::getenv("HOME");
    if (!home || !*home)
        throw Error("HOME is not set");
    return fs::path(home) / ".config" / "lamss";
}

std::string render_env_sh(const fs::path& root, const fs::path& env_sh_path)
{
    const auto prefix = root / "build" / "usr";
    std::ostringstream out;
    out << "# Written by `lamss env switch`; changes are overwritten.\n"
        << root_marker << root.string() << "\n"
        << R"(
# drop the previously active checkout's entries so switching does not accumulate them
_lamss_drop() {
    _lamss_out=
    _lamss_rest=$1:
    while [ -n "$_lamss_rest" ]; do
        _lamss_item=${_lamss_rest%%:*}
        _lamss_rest=${_lamss_rest#*:}
        case $_lamss_item in
            "" | "$LAMSS_PREFIX" | "$LAMSS_PREFIX"/*) ;;
            *) _lamss_out=${_lamss_out:+$_lamss_out:}$_lamss_item ;;
        esac
    done
    printf '%s' "$_lamss_out"
}
if [ -n "${LAMSS_PREFIX:-}" ]; then
    PATH=$(_lamss_drop "$PATH")
    LD_LIBRARY_PATH=$(_lamss_drop "${LD_LIBRARY_PATH:-}")
    PYTHONPATH=$(_lamss_drop "${PYTHONPATH:-}")
fi
unset -f _lamss_drop
unset _lamss_out _lamss_rest _lamss_item

)"
        << "export LAMSS_ROOT=" << quote(root) << "\n"
        << "export LAMSS_PREFIX=" << quote(prefix) << "\n"
        << R"(export PATH="$LAMSS_PREFIX/bin${PATH:+:$PATH}"
export LD_LIBRARY_PATH="$LAMSS_PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export PYTHONPATH="$LAMSS_PREFIX/lib/python3/dist-packages${PYTHONPATH:+:$PYTHONPATH}"

# reload this file after `lamss env switch` so the current shell follows
lamss() {
    command lamss "$@" || return
    if [ "${1:-}" = env ] && [ "${2:-}" = switch ]; then
        . )"
        << quote(env_sh_path) << R"(
    fi
}

case $(command -v lm 2>/dev/null) in
    "" | *"alias lm="*) alias lm=lamss ;;
esac
)";
    return out.str();
}

std::optional<fs::path> env_sh_root(const std::string& env_sh)
{
    std::istringstream lines(env_sh);
    for (std::string line; std::getline(lines, line);)
    {
        if (line.compare(0, root_marker.size(), root_marker) == 0)
            return fs::path(line.substr(root_marker.size()));
    }
    return std::nullopt;
}

bool has_hook(const std::string& rc) { return rc.find(hook_begin) != std::string::npos; }

std::string add_hook(const std::string& rc)
{
    if (has_hook(rc))
        return rc;
    std::string out = rc;
    if (!out.empty() && out.back() != '\n')
        out += '\n';
    if (!out.empty())
        out += '\n';
    out += hook_begin + "\n" +
           R"(_lamss_env="${XDG_CONFIG_HOME:-$HOME/.config}/lamss/env.sh"
[ -f "$_lamss_env" ] && . "$_lamss_env"
unset _lamss_env
)" + hook_end +
           "\n";
    return out;
}

std::string remove_hook(const std::string& rc)
{
    auto begin = rc.find(hook_begin);
    if (begin == std::string::npos)
        return rc;
    auto end = rc.find(hook_end, begin);
    if (end == std::string::npos)
        throw Error("found \"" + hook_begin + "\" without \"" + hook_end + "\"; edit it by hand");
    end += hook_end.size();
    if (end < rc.size() && rc[end] == '\n')
        ++end;
    // also drop the blank line add_hook put before the block
    if (begin >= 2 && rc[begin - 1] == '\n' && rc[begin - 2] == '\n')
        --begin;
    return rc.substr(0, begin) + rc.substr(end);
}

std::vector<fs::path> parse_checkouts(const std::string& text)
{
    std::vector<fs::path> checkouts;
    std::istringstream lines(text);
    for (std::string line; std::getline(lines, line);)
    {
        line = trim(line);
        if (!line.empty() && line[0] != '#')
            checkouts.emplace_back(line);
    }
    return checkouts;
}

std::string render_checkouts(const std::vector<fs::path>& checkouts)
{
    std::string out = "# lamss3 checkouts known to `lamss env`, one per line\n";
    for (const auto& checkout : checkouts) out += checkout.string() + "\n";
    return out;
}

} // namespace tool
} // namespace lamss
