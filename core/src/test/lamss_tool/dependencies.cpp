// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests dependencies.yml parsing, merging across toolboxes, distribution filtering and the apt
// source entries written for apt_repositories.

#include "check.h"
#include "deps.h"
#include "repo.h"

using namespace lamss::tool;
using Names = std::vector<std::string>;

const char* core_yml = R"(# core
apt:
  all:
    - cmake
    - libgoby3-dev  # comment
    - cmake
  jammy: [libnetcdf-cxx-legacy-dev]
  noble:
    - libnetcdf-c++4-dev
apt_repositories:
  - name: gobysoft
    url: http://packages.gobysoft.org/ubuntu/release/
    key: cmake/keys/gobysoft.asc
source:
  - name: rtd-dm7820-dkms
    script: src/sh/lamss_install_dm7820.sh
)";

const char* sonar_yml = R"(
apt:
  all: [libgoby3-dev, libfftw3-dev]
  noble: [libnetcdf-c++4-dev, libsonar-noble]
apt_repositories:
  - name: gobysoft
    url: http://elsewhere.example/
    key: other.asc
  - name: vendor
    url: https://vendor.example/apt
    key: keys/vendor.gpg
    components: [main, contrib]
source:
  - name: rtd-dm7820-dkms
    script: duplicate.sh
)";

bool throws(const std::string& yaml)
{
    try
    {
        parse_dependencies(yaml, "/tb", "tb/dependencies.yml");
    }
    catch (const Error& e)
    {
        CHECK(std::string(e.what()).find("tb/dependencies.yml") != std::string::npos);
        return true;
    }
    return false;
}

void test_parse()
{
    auto deps = parse_dependencies(core_yml, "/repo/core");
    CHECK((deps.apt.at("all") == Names{"cmake", "libgoby3-dev"}));
    CHECK((deps.apt.at("jammy") == Names{"libnetcdf-cxx-legacy-dev"}));
    CHECK(deps.apt_repositories.size() == 1);
    CHECK(deps.apt_repositories[0].key == "cmake/keys/gobysoft.asc");
    CHECK(deps.apt_repositories[0].toolbox_dir == "/repo/core");
    CHECK(deps.apt_repositories[0].components.empty());
    CHECK(deps.source.size() == 1 && deps.source[0].script == "src/sh/lamss_install_dm7820.sh");

    auto empty = parse_dependencies("# nothing\napt:\n  all: []\n", "/repo/autonomy");
    CHECK(empty.apt.at("all").empty());
    CHECK(parse_dependencies("", "/repo/x").apt.empty());
}

void test_malformed()
{
    CHECK(throws("apt: [a, b]\n"));
    CHECK(throws("apt:\n  all: pkg\n"));
    CHECK(throws("apt:\n  all:\n    - [nested]\n"));
    CHECK(throws("packages:\n  all: []\n"));
    CHECK(throws("apt_repositories:\n  - name: x\n    url: y\n"));
    CHECK(throws("source:\n  - name: x\n    scrpt: y.sh\n"));
    CHECK(throws("apt: {all: [a\n"));
    CHECK(throws("- a\n- b\n"));
}

void test_merge_and_filter()
{
    Dependencies merged;
    merge_dependencies(merged, parse_dependencies(core_yml, "/repo/core"));
    merge_dependencies(merged, parse_dependencies(sonar_yml, "/repo/sonar"));

    CHECK((merged.apt.at("all") == Names{"cmake", "libgoby3-dev", "libfftw3-dev"}));
    CHECK((merged.apt.at("noble") == Names{"libnetcdf-c++4-dev", "libsonar-noble"}));

    CHECK((apt_packages_for(merged, "noble") ==
           Names{"cmake", "libgoby3-dev", "libfftw3-dev", "libnetcdf-c++4-dev", "libsonar-noble"}));
    CHECK((apt_packages_for(merged, "jammy") ==
           Names{"cmake", "libgoby3-dev", "libfftw3-dev", "libnetcdf-cxx-legacy-dev"}));
    CHECK((apt_packages_for(merged, "resolute") == Names{"cmake", "libgoby3-dev", "libfftw3-dev"}));

    CHECK(merged.apt_repositories.size() == 2);
    CHECK(merged.apt_repositories[0].url == "http://packages.gobysoft.org/ubuntu/release/");
    CHECK(merged.apt_repositories[1].toolbox_dir == "/repo/sonar");
    CHECK(merged.source.size() == 1 && merged.source[0].toolbox_dir == "/repo/core");
}

void test_apt_sources()
{
    Dependencies merged;
    merge_dependencies(merged, parse_dependencies(sonar_yml, "/repo/sonar"));
    auto gobysoft = parse_dependencies(core_yml, "/repo/core").apt_repositories[0];

    CHECK(apt_source_entry(gobysoft, "noble", "/etc/apt/keyrings/gobysoft.gpg") ==
          "deb [signed-by=/etc/apt/keyrings/gobysoft.gpg] "
          "http://packages.gobysoft.org/ubuntu/release/ noble/");
    CHECK(apt_source_entry(merged.apt_repositories[1], "jammy", "/k/vendor.gpg") ==
          "deb [signed-by=/k/vendor.gpg] https://vendor.example/apt jammy main contrib");

    const std::string sources = "# deb http://vendor.example/apt jammy main\n"
                                "deb [signed-by=/etc/apt/keyrings/gobysoft.gpg] "
                                "http://packages.gobysoft.org/ubuntu/release noble/\n";
    CHECK(sources_mention(sources, "http://packages.gobysoft.org/ubuntu/release/"));
    CHECK(!sources_mention(sources, "http://vendor.example/apt"));
}

void test_os_release()
{
    CHECK(parse_os_release_codename("NAME=\"Ubuntu\"\nVERSION_CODENAME=noble\nID=ubuntu\n") ==
          std::string("noble"));
    CHECK(parse_os_release_codename("VERSION_CODENAME=\"jammy\"\n") == std::string("jammy"));
    CHECK(!parse_os_release_codename("NAME=Arch\nVERSION_CODENAME=\n"));
}

int main()
{
    test_parse();
    test_malformed();
    test_merge_and_filter();
    test_apt_sources();
    test_os_release();
    return 0;
}
