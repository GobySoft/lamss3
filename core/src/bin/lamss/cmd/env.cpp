// Copyright 2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>
#include <iostream>

#include "command.h"
#include "process.h"
#include "repo.h"

namespace
{
// Prepends entry, dropping any existing copy so repeated evals do not grow the variable.
std::string prepend_path(const std::string& entry, const char* variable)
{
    std::string result = entry;
    const char* current = std::getenv(variable);
    std::string rest = current ? current : "";
    std::size_t begin = 0;
    while (begin < rest.size())
    {
        auto end = rest.find(':', begin);
        if (end == std::string::npos)
            end = rest.size();
        auto item = rest.substr(begin, end - begin);
        if (!item.empty() && item != entry)
            result += ":" + item;
        begin = end + 1;
    }
    return result;
}

void print_export(const std::string& variable, const std::string& value)
{
    std::cout << "export " << variable << "=" << lamss::tool::format_command({value}) << "\n";
}
} // namespace

lamss::tool::EnvTool::EnvTool()
{
    execute(
        []()
        {
            Repo repo(find_root());
            const auto prefix = repo.prefix();
            print_export("LAMSS_ROOT", repo.root().string());
            print_export("LAMSS_PREFIX", prefix.string());
            print_export("PATH", prepend_path((prefix / "bin").string(), "PATH"));
            print_export("LD_LIBRARY_PATH",
                         prepend_path((prefix / "lib").string(), "LD_LIBRARY_PATH"));
            print_export("PYTHONPATH",
                         prepend_path((prefix / "lib" / "python3" / "dist-packages").string(),
                                      "PYTHONPATH"));
            return 0;
        });
}
