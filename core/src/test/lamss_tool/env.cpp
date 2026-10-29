// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests the files behind `lamss env`: the shell rc hook is added once and removed cleanly, the
// generated env.sh records its checkout, quotes awkward paths, and the checkouts list round-trips.

#include <cstdlib>

#include "check.h"
#include "envfile.h"

using namespace lamss::tool;
namespace fs = std::filesystem;

void test_hook_add_remove()
{
    const std::string rc = "alias ll='ls -l'\nexport EDITOR=vi";
    CHECK(!has_hook(rc));

    auto with = add_hook(rc);
    CHECK(has_hook(with));
    CHECK(with.rfind(rc + "\n\n", 0) == 0);
    CHECK(add_hook(with) == with);

    // the original ended without a newline; removal restores it with one
    CHECK(remove_hook(with) == rc + "\n");
    CHECK(remove_hook(rc) == rc);
}

void test_hook_in_middle_and_empty()
{
    CHECK(remove_hook(add_hook("")) == "");

    auto with = add_hook("a\n") + "b\n";
    CHECK(remove_hook(with) == "a\nb\n");
}

void test_hook_unterminated()
{
    bool threw = false;
    try
    {
        remove_hook("# >>> lamss >>>\nstuff\n");
    }
    catch (const std::exception&)
    {
        threw = true;
    }
    CHECK(threw);
}

void test_env_sh()
{
    auto sh = render_env_sh("/home/me/lamss3", "/home/me/.config/lamss/env.sh");
    CHECK(env_sh_root(sh) == fs::path("/home/me/lamss3"));
    CHECK(sh.find("export LAMSS_PREFIX=/home/me/lamss3/build/usr\n") != std::string::npos);
    CHECK(sh.find(". /home/me/.config/lamss/env.sh\n") != std::string::npos);

    auto quoted = render_env_sh("/data/my lamss's", "/c/env.sh");
    CHECK(env_sh_root(quoted) == fs::path("/data/my lamss's"));
    CHECK(quoted.find("export LAMSS_ROOT='/data/my lamss'\\''s'\n") != std::string::npos);

    CHECK(!env_sh_root("export FOO=1\n"));
}

void test_checkouts_round_trip()
{
    std::vector<fs::path> checkouts{"/a/lamss3", "/b/lamss3"};
    CHECK(parse_checkouts(render_checkouts(checkouts)) == checkouts);
    CHECK(parse_checkouts("# comment\n\n  /c/x  \n") == std::vector<fs::path>{"/c/x"});
}

void test_config_dir()
{
    setenv("XDG_CONFIG_HOME", "/xdg", 1);
    CHECK(config_dir() == fs::path("/xdg/lamss"));
    unsetenv("XDG_CONFIG_HOME");
    setenv("HOME", "/home/me", 1);
    CHECK(config_dir() == fs::path("/home/me/.config/lamss"));
}

int main()
{
    test_hook_add_remove();
    test_hook_in_middle_and_empty();
    test_hook_unterminated();
    test_env_sh();
    test_checkouts_round_trip();
    test_config_dir();
    return 0;
}
