// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests editing of .lamss/toolboxes: comments and unrelated lines survive, core is implicit and
// never written, duplicates are tolerated on read and never created.

#include <unistd.h>

#include "check.h"
#include "repo.h"

using lamss::tool::ToolboxList;
using Names = std::vector<std::string>;

void test_empty()
{
    ToolboxList list;
    CHECK(list.active() == Names{"core"});
    CHECK(list.is_active("core"));
    CHECK(list.str().empty());
}

void test_comments_preserved()
{
    const std::string text = "# Active toolboxes\n"
                             "# hand-editable\n"
                             "\n"
                             "autonomy\n"
                             "# sonar\n"
                             "  virtual_ocean  \n";
    ToolboxList list(text);
    CHECK((list.active() == Names{"core", "autonomy", "virtual_ocean"}));
    CHECK(!list.is_active("sonar"));
    CHECK(list.str() == text);

    CHECK(list.activate("sonar"));
    CHECK(list.str() == text + "sonar\n");

    CHECK(list.deactivate("autonomy"));
    CHECK(list.str() == "# Active toolboxes\n"
                        "# hand-editable\n"
                        "\n"
                        "# sonar\n"
                        "  virtual_ocean  \n"
                        "sonar\n");
    CHECK((list.active() == Names{"core", "virtual_ocean", "sonar"}));
}

void test_missing_final_newline()
{
    ToolboxList list("# comment\nautonomy");
    CHECK(list.activate("sonar"));
    CHECK(list.str() == "# comment\nautonomy\nsonar\n");
}

void test_core_implicit()
{
    ToolboxList list("core\nautonomy\n");
    CHECK((list.active() == Names{"core", "autonomy"}));
    CHECK(!list.activate("core"));
    CHECK(list.str() == "core\nautonomy\n");

    ToolboxList fresh;
    CHECK(!fresh.activate("core"));
    CHECK(fresh.str().empty());
}

void test_duplicates()
{
    ToolboxList list("autonomy\nsonar\nautonomy\n");
    CHECK((list.active() == Names{"core", "autonomy", "sonar"}));
    CHECK(!list.activate("autonomy"));
    CHECK(!list.activate("sonar"));
    CHECK(list.deactivate("autonomy"));
    CHECK(list.str() == "sonar\n");
    CHECK(!list.deactivate("autonomy"));
}

void test_file_round_trip()
{
    namespace fs = std::filesystem;
    auto dir = fs::temp_directory_path() / ("lamss_tool_test_" + std::to_string(getpid()));
    fs::create_directories(dir / ".lamss");
    auto file = dir / ".lamss" / "toolboxes";

    CHECK(ToolboxList::read(file).active() == Names{"core"});

    lamss::tool::write_file(file, "# keep me\nautonomy\n");
    lamss::tool::Repo repo(dir);
    auto list = repo.toolbox_list();
    CHECK(list.activate("sonar"));
    repo.write_toolbox_list(list);
    CHECK(lamss::tool::read_file(file) == "# keep me\nautonomy\nsonar\n");
    CHECK(!fs::exists(dir / ".lamss" / "toolboxes.tmp"));

    CHECK(lamss::tool::find_root_from(dir / ".lamss") == fs::canonical(dir));
    fs::remove_all(dir);
}

int main()
{
    test_empty();
    test_comments_preserved();
    test_missing_final_newline();
    test_core_implicit();
    test_duplicates();
    test_file_round_trip();
    return 0;
}
