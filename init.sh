#!/bin/sh
# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Sets up a lamss3 checkout: builds the lamss tool, makes this the active checkout, loads it in new
# shells and installs the dependencies. Shows each step and asks first; safe to re-run.

set -eu

usage() {
    cat <<USAGE
Usage: ./init.sh [--yes] [--no-deps]

  --yes      Run every step without asking
  --no-deps  Install no packages (skip the apt steps)
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
tool_packages="build-essential cmake ninja-build libyaml-cpp-dev gpg"
env_sh="${XDG_CONFIG_HOME:-$HOME/.config}/lamss/env.sh"

sudo=""
[ "$(id -u)" -eq 0 ] || sudo="sudo "

# Same meanings as the lamss tool's output: bold heading, cyan command, green done, yellow needs
# attention, red error, dim note. Plain text unless writing to a terminal.
if [ -t 1 ] && [ -z "${NO_COLOR:-}" ] && [ "${TERM:-}" != dumb ]; then
    esc=$(printf '\033')
    heading="${esc}[1m" command="${esc}[36m" good="${esc}[32m" attention="${esc}[33m"
    error="${esc}[1;31m" note="${esc}[2m" plain="${esc}[0m"
else
    heading="" command="" good="" attention="" error="" note="" plain=""
fi

fail() {
    printf 'init.sh: %serror:%s %s\n' "$error" "$plain" "$1" >&2
    exit 1
}

step_number=0
step_count=3
[ "$install_deps" -eq 1 ] && step_count=5

step_title() {
    step_number=$((step_number + 1))
    printf '\n%sStep %d/%d  %s%s\n' "$heading" "$step_number" "$step_count" "$1" "$plain"
}

# Prints the remaining arguments as the step's details and asks; returns 1 to skip the step.
step() {
    step_title "$1"
    shift
    for line in "$@"; do
        case "$line" in
            "("*) printf '          %s%s%s\n' "$note" "$line" "$plain" ;;
            *) printf '          %s%s%s\n' "$command" "$line" "$plain" ;;
        esac
    done
    [ "$assume_yes" -eq 1 ] && return 0
    while :; do
        printf '          %sProceed? [Y/n/q]%s ' "$heading" "$plain"
        if ! read -r answer; then
            echo
            fail "no answer; use --yes to run without asking"
        fi
        case "$answer" in
            "" | y | Y | yes) return 0 ;;
            n | N | no)
                printf '          %sSkipped.%s\n' "$attention" "$plain"
                return 1
                ;;
            q | Q | quit)
                printf '%sStopped.%s\n' "$attention" "$plain"
                exit 1
                ;;
        esac
    done
}

already_done() {
    step_title "$1"
    printf '          %sAlready done.%s\n' "$good" "$plain"
}

run() {
    printf '%s+ %s%s\n' "$command" "$*" "$plain"
    "$@"
}

# shellcheck disable=SC1091 # os-release is not part of this repository
distro=$(. /etc/os-release && echo "$PRETTY_NAME")
echo "lamss3 setup for $root ($distro)"
[ "$assume_yes" -eq 1 ] || echo "Each step shows what it will run and asks first; --yes accepts them all."

if [ "$install_deps" -eq 1 ]; then
    missing=""
    for pkg in $tool_packages; do
        dpkg-query -W -f='${Status}' "$pkg" 2>/dev/null | grep -q 'install ok installed' ||
            missing="$missing $pkg"
    done
    title="Install the packages that build the lamss tool (Ubuntu archive)"
    if [ -z "$missing" ]; then
        already_done "$title"
    elif step "$title" "${sudo}apt-get update" "${sudo}apt-get install -y --no-install-recommends$missing"; then
        # shellcheck disable=SC2086 # word splitting of the command prefix and package list is intended
        run $sudo apt-get update
        # shellcheck disable=SC2086
        run $sudo env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends $missing
    fi
fi

build_dir="$root/build/lamss-tool"
if step "Build the lamss tool into build/usr" \
    "cmake -S core/src/bin/lamss -B build/lamss-tool -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo" \
    "cmake --build build/lamss-tool" \
    "cmake --install build/lamss-tool --prefix build/usr"; then
    run cmake -S "$root/core/src/bin/lamss" -B "$build_dir" -G Ninja \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX="$root/build/usr"
    run cmake --build "$build_dir"
    run cmake --install "$build_dir"
fi

if [ ! -x "$tool" ]; then
    fail "$tool was not built; the remaining steps need it"
fi

title="Make this the active lamss3 checkout (writes $env_sh)"
if grep -qsxF "# lamss-root: $root" "$env_sh"; then
    already_done "$title"
else
    previous=$(sed -n 's/^# lamss-root: //p' "$env_sh" 2>/dev/null || true)
    set -- "lamss env switch $root"
    [ -n "$previous" ] && set -- "$@" "(the active checkout is $previous)"
    if step "$title" "$@"; then
        run "$tool" env switch "$root"
    fi
fi

title="Load the active checkout in new shells"
hooked=1
# shellcheck disable=SC2088 # displayed, not expanded
rc_names="~/.bashrc"
grep -qsF '# >>> lamss >>>' "$HOME/.bashrc" || hooked=0
if [ -f "$HOME/.zshrc" ]; then
    rc_names="$rc_names and ~/.zshrc"
    grep -qsF '# >>> lamss >>>' "$HOME/.zshrc" || hooked=0
fi
if [ "$hooked" -eq 1 ]; then
    already_done "$title"
elif step "$title" "lamss env hook" "(adds a 5-line block to $rc_names)"; then
    hooked=1
    run "$tool" env hook
else
    hooked=0
fi

if [ "$install_deps" -eq 1 ] &&
    step "Install the active toolboxes' dependencies" "lamss dependencies install"; then
    if [ "$assume_yes" -eq 1 ]; then
        run "$tool" dependencies install --yes
    else
        run "$tool" dependencies install
    fi
fi

echo
printf '%sDone.%s\n' "$heading$good" "$plain"
if [ "${LAMSS_ROOT:-}" != "$root" ]; then
    if [ "$hooked" -eq 1 ]; then
        echo "Open a new shell to use lamss3, or load it into this one now:"
    else
        echo "New shells will not load lamss3 by themselves (run \`lamss env hook\` to change that); load it with:"
    fi
    printf '  %s. %s%s\n' "$command" "$env_sh" "$plain"
fi
printf 'Then build with: %slamss build%s (or %slm b%s)\n' "$command" "$plain" "$command" "$plain"
