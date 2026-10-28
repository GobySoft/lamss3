// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <thread>

#include "command.h"
#include "process.h"
#include "repo.h"

lamss::tool::BuildTool::BuildTool()
{
    execute(
        [this]()
        {
            const auto& cfg = app_cfg();
            Repo repo(find_root());
            const auto build_dir = repo.build_dir();
            const auto cache = build_dir / "CMakeCache.txt";
            const bool cached = fs::exists(cache);

            bool configure = cfg.configure() || !cached || cfg.has_type() || cfg.define_size() > 0;
            if (!configure && fs::exists(repo.toolboxes_file()))
                configure = fs::last_write_time(repo.toolboxes_file()) > fs::last_write_time(cache);

            if (configure)
            {
                Argv cmake{"cmake", "-S", repo.root().string(), "-B", build_dir.string()};
                if (!cached)
                {
                    cmake.insert(cmake.end(), {"-G", "Ninja"});
                }
                if (cfg.has_type())
                    cmake.push_back("-DCMAKE_BUILD_TYPE=" +
                                    protobuf::BuildToolConfig::BuildType_Name(cfg.type()));
                else if (!cached)
                    cmake.push_back("-DCMAKE_BUILD_TYPE=RelWithDebInfo");
                for (const auto& define : cfg.define()) cmake.push_back("-D" + define);

                if (int status = run_process(cmake))
                    return status;
            }

            const int jobs =
                cfg.has_jobs() ? cfg.jobs()
                               : std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
            const auto jobs_str = std::to_string(jobs);

            fs::path test_dir = build_dir;
            if (cfg.has_toolbox())
            {
                const auto& tb = cfg.toolbox();
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

            if (cfg.test())
            {
                RunOptions in_test_dir;
                in_test_dir.cwd = test_dir;
                return run_process({"ctest", "--output-on-failure", "-j", jobs_str}, in_test_dir);
            }
            return 0;
        });
}
