// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <thread>

#include "command.h"
#include "process.h"
#include "repo.h"

namespace
{
int parse_jobs(const std::string& text)
{
    try
    {
        std::size_t used = 0;
        int jobs = std::stoi(text, &used);
        if (used == text.size() && jobs > 0)
            return jobs;
    }
    catch (const std::exception&)
    {
    }
    throw lamss::tool::UsageError("--jobs must be a positive integer");
}
} // namespace

lamss::tool::Command lamss::tool::build_command()
{
    return {
        "build",
        {"b"},
        "Configure (if needed), build and install the active toolboxes",
        {{"configure", 0, "",
          "Force CMake to reconfigure (automatic when .lamss/toolboxes is newer than "
          "build/CMakeCache.txt)"},
         {"toolbox", 0, "TOOLBOX", "Build and install only this toolbox"},
         {"type",
          0,
          "TYPE",
          "CMAKE_BUILD_TYPE; RelWithDebInfo on first configure, then remembered in build/",
          {"Debug", "Release", "RelWithDebInfo", "MinSizeRel"}},
         {"jobs", 'j', "N", "Parallel build jobs (default: number of processors)"},
         {"test", 0, "", "Run ctest after building"},
         {"define",
          'D',
          "KEY=VALUE",
          "Passed to CMake as -DKEY=VALUE (forces a reconfigure)",
          {},
          true}},
        {},
        {},
        [](const Args& args)
        {
            Repo repo(find_root());
            const auto type = args.value("type");
            const auto defines = args.values("define");
            const auto build_dir = repo.build_dir();
            const auto cache = build_dir / "CMakeCache.txt";
            const bool cached = fs::exists(cache);

            bool configure = args.flag("configure") || !cached || type || !defines.empty();
            if (!configure && fs::exists(repo.toolboxes_file()))
                configure = fs::last_write_time(repo.toolboxes_file()) > fs::last_write_time(cache);

            if (configure)
            {
                Argv cmake{"cmake", "-S", repo.root().string(), "-B", build_dir.string()};
                if (!cached)
                {
                    cmake.insert(cmake.end(), {"-G", "Ninja"});
                }
                if (type)
                    cmake.push_back("-DCMAKE_BUILD_TYPE=" + *type);
                else if (!cached)
                    cmake.push_back("-DCMAKE_BUILD_TYPE=RelWithDebInfo");
                for (const auto& define : defines) cmake.push_back("-D" + define);

                if (int status = run_process(cmake))
                    return status;
            }

            const int jobs =
                args.value("jobs")
                    ? parse_jobs(*args.value("jobs"))
                    : std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
            const auto jobs_str = std::to_string(jobs);

            fs::path test_dir = build_dir;
            if (auto toolbox = args.value("toolbox"))
            {
                const auto& tb = *toolbox;
                validate_toolbox_name(tb);
                if (!repo.toolbox_list().is_active(tb))
                    throw Error("toolbox '" + tb + "' is not active; run `lamss toolbox activate " +
                                tb + "`");
                if (int status = run_process({"cmake", "--build", build_dir.string(), "--target",
                                              "lamss_" + tb + "_all", "-j", jobs_str}))
                    return status;
                test_dir = build_dir / tb;
                if (int status = run_process({"cmake", "--install", test_dir.string()}))
                    return status;
            }
            else
            {
                if (int status = run_process({"cmake", "--build", build_dir.string(), "--target",
                                              "install", "-j", jobs_str}))
                    return status;
            }

            if (args.flag("test"))
            {
                RunOptions in_test_dir;
                in_test_dir.cwd = test_dir;
                return run_process({"ctest", "--output-on-failure", "-j", jobs_str}, in_test_dir);
            }
            return 0;
        }};
}
