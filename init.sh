#!/bin/sh
# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Bootstraps a lamss3 checkout: builds the lamss tool into build/usr, links it into ~/.local/bin,
# then has it install the active toolboxes' dependencies. Safe to re-run.

set -eu

usage() {
    cat <<USAGE
Usage: ./init.sh [--yes] [--no-deps]

  --yes      Do not ask before installing packages (apt-get -y)
  --no-deps  Only build and link the tool; install nothing
USAGE
}

assume_yes=0
install_deps=1
for arg in "$@"; do
    case "$arg" in
        --yes | -y) assume_yes=1 ;;
        --no-deps) install_deps=0 ;;
        -h | --help)
            usage
            exit 0
            ;;
        *)
            echo "init.sh: unknown option '$arg'" >&2
            usage >&2
            exit 2
            ;;
    esac
done

root=$(cd "$(dirname "$0")" && pwd -P)
tool="$root/build/usr/bin/lamss"
# from the Ubuntu archive; everything else comes from `lamss dependencies install`
tool_packages="build-essential cmake ninja-build libyaml-cpp-dev gpg"

run() {
    printf '+ %s\n' "$*"
    "$@"
}

as_root() {
    if [ "$(id -u)" -eq 0 ]; then
        run "$@"
    else
        run sudo "$@"
    fi
}

install_tool_packages() {
    missing=""
    for pkg in $tool_packages; do
        dpkg-query -W -f='${Status}' "$pkg" 2>/dev/null | grep -q 'install ok installed' ||
            missing="$missing $pkg"
    done
    [ -z "$missing" ] && return
    as_root apt-get update
    # shellcheck disable=SC2086 # word splitting of the package list is intended
    if [ "$assume_yes" -eq 1 ]; then
        as_root env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends $missing
    else
        as_root apt-get install --no-install-recommends $missing
    fi
}

build_tool() {
    run cmake -S "$root/core/src/bin/lamss" -B "$root/build/lamss-tool" -G Ninja \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX="$root/build/usr"
    run cmake --build "$root/build/lamss-tool"
    run cmake --install "$root/build/lamss-tool"
}

link_tool() {
    bin_dir="$HOME/.local/bin"
    run mkdir -p "$bin_dir"
    if [ -e "$bin_dir/lamss" ] && [ ! -L "$bin_dir/lamss" ]; then
        echo "init.sh: $bin_dir/lamss exists and is not a symlink; leaving it alone" >&2
    else
        run ln -sfn "$tool" "$bin_dir/lamss"
    fi

    if [ -L "$bin_dir/lm" ]; then
        case "$(readlink "$bin_dir/lm")" in
            */build/usr/bin/lamss) run ln -sfn "$tool" "$bin_dir/lm" ;;
            *) echo "lm: $bin_dir/lm is another program; not installing the lm alias" ;;
        esac
    elif command -v lm >/dev/null 2>&1 || [ -e "$bin_dir/lm" ]; then
        echo "lm: '$(command -v lm || echo "$bin_dir/lm")' already exists; use lamss instead of lm"
    else
        run ln -s "$tool" "$bin_dir/lm"
    fi
}

if [ "$install_deps" -eq 1 ]; then
    install_tool_packages
fi
build_tool
link_tool
if [ "$install_deps" -eq 1 ]; then
    if [ "$assume_yes" -eq 1 ]; then
        run "$tool" dependencies install --yes
    else
        run "$tool" dependencies install
    fi
fi

echo
echo "Done. Set up your shell (add to ~/.bashrc to make it permanent):"
# shellcheck disable=SC2016 # printed for the user to run
echo '  eval "$(lamss env)"'
case ":$PATH:" in
    *":$HOME/.local/bin:"*) ;;
    *)
        echo "$HOME/.local/bin is not on your PATH; add it first:"
        # shellcheck disable=SC2016 # printed for the user to run
        echo '  export PATH="$HOME/.local/bin:$PATH"'
        ;;
esac
echo "Then build with: lamss build"
