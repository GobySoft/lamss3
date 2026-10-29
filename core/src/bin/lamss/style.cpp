// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include "style.h"

namespace lamss
{
namespace tool
{
namespace style
{
namespace
{
std::string wrap(const char* code, const std::string& text, Stream stream)
{
    return enabled(stream) ? std::string("\033[") + code + "m" + text + "\033[0m" : text;
}
} // namespace

bool enabled(Stream stream)
{
    static const bool colour_wanted = []
    {
        const char* term = std::getenv("TERM");
        return !std::getenv("NO_COLOR") && !(term && std::strcmp(term, "dumb") == 0);
    }();
    static const bool out_tty = isatty(STDOUT_FILENO);
    static const bool err_tty = isatty(STDERR_FILENO);
    return colour_wanted && (stream == Stream::out ? out_tty : err_tty);
}

std::string heading(const std::string& text, Stream stream) { return wrap("1", text, stream); }
std::string command(const std::string& text, Stream stream) { return wrap("36", text, stream); }
std::string good(const std::string& text, Stream stream) { return wrap("32", text, stream); }
std::string attention(const std::string& text, Stream stream) { return wrap("33", text, stream); }
std::string error(const std::string& text, Stream stream) { return wrap("1;31", text, stream); }
std::string note(const std::string& text, Stream stream) { return wrap("2", text, stream); }
} // namespace style
} // namespace tool
} // namespace lamss
