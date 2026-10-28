// Copyright 2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <iostream>
#include <set>

#include "command.h"
#include "repo.h"

namespace
{
namespace fs = std::filesystem;

bool is_ancestor(const fs::path& ancestor, const fs::path& path)
{
    auto mismatch = std::mismatch(ancestor.begin(), ancestor.end(), path.begin(), path.end());
    return mismatch.first == ancestor.end();
}

void collect(const fs::path& dir, const std::set<fs::path>& keep, std::vector<fs::path>& remove)
{
    std::vector<fs::path> entries;
    for (const auto& entry : fs::directory_iterator(dir)) entries.push_back(entry.path());
    std::sort(entries.begin(), entries.end());

    for (const auto& path : entries)
    {
        if (keep.count(path))
            continue;
        bool holds_kept = fs::is_directory(fs::symlink_status(path)) &&
                          std::any_of(keep.begin(), keep.end(),
                                      [&](const fs::path& k) { return is_ancestor(path, k); });
        if (holds_kept)
            collect(path, keep, remove);
        else
            remove.push_back(path);
    }
}
} // namespace

lamss::tool::CleanTool::CleanTool()
{
    execute(
        [this]()
        {
            const auto& cfg = app_cfg();
            Repo repo(find_root());
            const auto build_dir = repo.build_dir();

            std::set<fs::path> keep;
            if (!cfg.all())
            {
                keep.insert(repo.prefix() / "bin" / "lamss");
                keep.insert(build_dir / "lamss-tool");
                std::error_code ec;
                auto exe = fs::read_symlink("/proc/self/exe", ec);
                if (!ec)
                    keep.insert(exe);
            }

            std::vector<fs::path> remove;
            if (fs::exists(build_dir))
            {
                if (cfg.all())
                    remove.push_back(build_dir);
                else
                    collect(build_dir, keep, remove);
            }

            if (remove.empty())
            {
                std::cout << "Nothing to clean." << std::endl;
                return 0;
            }

            std::cout << "Removing:" << std::endl;
            for (const auto& path : remove)
                std::cout << "  " << path.lexically_relative(repo.root()).string() << std::endl;

            if (!cfg.yes())
            {
                std::cout << "Proceed? [y/N] " << std::flush;
                std::string answer;
                std::getline(std::cin, answer);
                answer = trim(answer);
                if (answer != "y" && answer != "Y" && answer != "yes")
                {
                    std::cout << "Aborted." << std::endl;
                    return 1;
                }
            }

            for (const auto& path : remove) fs::remove_all(path);

            if (cfg.all())
                std::cout << "Removed the lamss tool too; run ./init.sh --no-deps to rebuild it."
                          << std::endl;
            return 0;
        });
}
