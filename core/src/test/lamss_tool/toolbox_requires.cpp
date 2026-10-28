// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests parsing lamss_toolbox(NAME ... REQUIRES ...) from a toolbox CMakeLists.txt and the
// activation / deactivation closures computed from it.

#include <map>

#include "check.h"
#include "repo.h"

using namespace lamss::tool;
using Names = std::vector<std::string>;

void test_parse()
{
    auto decl =
        parse_toolbox_declaration("cmake_minimum_required(VERSION 3.22...4.2)\n"
                                  "project(lamss_sonar CXX)\n"
                                  "lamss_toolbox(NAME sonar REQUIRES core virtual_ocean)\n");
    CHECK(decl);
    CHECK(decl->name == "sonar");
    CHECK((decl->required == Names{"core", "virtual_ocean"}));

    decl = parse_toolbox_declaration("lamss_toolbox(NAME core)\n");
    CHECK(decl && decl->name == "core" && decl->required.empty());

    CHECK(!parse_toolbox_declaration("project(foo)\n"));
    CHECK(!parse_toolbox_declaration("# lamss_toolbox(NAME commented REQUIRES x)\n"));
    CHECK(!parse_toolbox_declaration("my_lamss_toolbox(NAME other REQUIRES x)\n"));
}

void test_parse_multiline()
{
    auto decl = parse_toolbox_declaration("include(LamssToolbox)\n"
                                          "LAMSS_TOOLBOX(\n"
                                          "  REQUIRES\n"
                                          "    autonomy   # navigation\n"
                                          "    \"virtual_ocean\"\n"
                                          "    autonomy\n"
                                          "  NAME mit\n"
                                          ")\n"
                                          "lamss_toolbox_export()\n");
    CHECK(decl);
    CHECK(decl->name == "mit");
    CHECK((decl->required == Names{"autonomy", "virtual_ocean"}));
}

void test_gitmodules()
{
    auto paths = parse_gitmodules_paths("[submodule \"mit\"]\n"
                                        "\tpath = mit\n"
                                        "\turl = git@github.com:GobySoft/lamss-mit.git\n"
                                        "[submodule \"hydroman/upstream\"]\n"
                                        "\tpath = hydroman/upstream\n"
                                        "\turl = https://example.com/hydroman.git\n");
    CHECK((paths == Names{"mit", "hydroman/upstream"}));
}

const std::map<std::string, Names> graph{{"core", {}},
                                         {"autonomy", {"core"}},
                                         {"virtual_ocean", {"core"}},
                                         {"sonar", {"core", "virtual_ocean"}},
                                         {"mit", {"autonomy", "virtual_ocean"}},
                                         {"icex", {"autonomy", "mit"}}};

Names requires_of(const std::string& name)
{
    auto it = graph.find(name);
    return it == graph.end() ? Names{} : it->second;
}

void test_activation()
{
    auto plan = plan_activation({"icex"}, requires_of);
    Names names;
    for (const auto& step : plan) names.push_back(step.name);
    CHECK((names == Names{"autonomy", "virtual_ocean", "mit", "icex"}));
    CHECK(plan[0].required_by == "icex");
    CHECK(plan[1].required_by == "mit");
    CHECK(plan[2].required_by == "icex");
    CHECK(plan[3].required_by.empty());

    CHECK(plan_activation({"core"}, requires_of).empty());

    auto cyclic = [](const std::string& name) { return name == "a" ? Names{"b"} : Names{"a"}; };
    CHECK(plan_activation({"a"}, cyclic).size() == 2);
}

void test_deactivation()
{
    const Names active{"core", "autonomy", "virtual_ocean", "sonar", "mit"};

    CHECK((plan_deactivation({"sonar"}, active, requires_of, false) == Names{"sonar"}));

    bool threw = false;
    try
    {
        plan_deactivation({"virtual_ocean"}, active, requires_of, false);
    }
    catch (const Error&)
    {
        threw = true;
    }
    CHECK(threw);

    CHECK((plan_deactivation({"virtual_ocean"}, active, requires_of, true) ==
           Names{"virtual_ocean", "sonar", "mit"}));
    CHECK((plan_deactivation({"virtual_ocean", "sonar", "mit"}, active, requires_of, false) ==
           Names{"virtual_ocean", "sonar", "mit"}));
}

int main()
{
    test_parse();
    test_parse_multiline();
    test_gitmodules();
    test_activation();
    test_deactivation();
    return 0;
}
