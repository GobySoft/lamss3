// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LAMSS_TOOL_CMD_COMMAND_H
#define LAMSS_TOOL_CMD_COMMAND_H

#include "cli.h"

namespace lamss
{
namespace tool
{
Command build_command();
Command toolbox_command();
Command dependencies_command();
Command env_command();
Command clean_command();
} // namespace tool
} // namespace lamss

#endif
