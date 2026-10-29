// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LAMSS_TOOL_STYLE_H
#define LAMSS_TOOL_STYLE_H

#include <string>

// One fixed meaning per style, shared with init.sh; plain text unless writing to a terminal.
namespace lamss
{
namespace tool
{
namespace style
{
enum class Stream
{
    out,
    err
};

bool enabled(Stream stream);

std::string heading(const std::string& text, Stream stream = Stream::out);
std::string command(const std::string& text, Stream stream = Stream::out);
std::string good(const std::string& text, Stream stream = Stream::out);
std::string attention(const std::string& text, Stream stream = Stream::out);
std::string error(const std::string& text, Stream stream = Stream::err);
std::string note(const std::string& text, Stream stream = Stream::out);
} // namespace style
} // namespace tool
} // namespace lamss

#endif
