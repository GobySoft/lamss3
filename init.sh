#!/bin/sh
# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Bootstraps a lamss3 checkout: installs core's apt dependencies, builds the lamss tool into
# build/usr and links it into ~/.local/bin. Safe to re-run.

set -eu

usage() {
    cat <<EOF
Usage: ./init.sh [--yes] [--no-deps]

  --yes      Do not ask before installing packages (apt-get -y)
  --no-deps  Skip the apt steps (GobySoft repository and core/dependencies.yml)
EOF
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

gobysoft_keyring=/etc/apt/keyrings/gobysoft.gpg
gobysoft_list=/etc/apt/sources.list.d/gobysoft.list
gobysoft_url=http://packages.gobysoft.org/ubuntu/release/

run() {
    printf '+ %s\n' "$*"
    "$@"
}

sudo_if_needed() {
    if [ "$(id -u)" -eq 0 ]; then
        "$@"
    elif command -v sudo >/dev/null 2>&1; then
        sudo "$@"
    else
        echo "init.sh: need root for '$*' but sudo is not installed" >&2
        exit 1
    fi
}

as_root() {
    if [ "$(id -u)" -eq 0 ]; then
        printf '+ %s\n' "$*"
    else
        printf '+ sudo %s\n' "$*"
    fi
    sudo_if_needed "$@"
}

codename() {
    if [ -r /etc/os-release ]; then
        # shellcheck source=/dev/null
        (. /etc/os-release && echo "${VERSION_CODENAME:-}")
    elif command -v lsb_release >/dev/null 2>&1; then
        lsb_release -cs
    fi
}

# Prints the apt packages for "all" and the given codename from a dependencies.yml that uses the
# subset documented at the top of core/dependencies.yml.
apt_packages() {
    awk -v codename="$2" '
        function emit(item) {
            sub(/#.*/, "", item)
            gsub(/[][ \t"'\'']/, "", item)
            if (item != "") print item
        }
        /^[^ \t#]/ { section = $0; sub(/:.*/, "", section); key = ""; next }
        section != "apt" { next }
        /^[ \t]+[A-Za-z0-9_]+:/ {
            key = $0; sub(/^[ \t]+/, "", key); sub(/:.*/, "", key)
            rest = $0; sub(/^[^:]*:/, "", rest); sub(/#.*/, "", rest)
            if ((key == "all" || key == codename) && rest ~ /\[/) {
                n = split(rest, items, ",")
                for (i = 1; i <= n; i++) emit(items[i])
            }
            next
        }
        /^[ \t]+-/ && (key == "all" || key == codename) {
            item = $0; sub(/^[ \t]+-/, "", item); emit(item)
        }
    ' "$1"
}

confirm() {
    [ "$assume_yes" -eq 1 ] && return 0
    printf '%s [Y/n] ' "$1"
    read -r answer || answer=n
    case "$answer" in
        "" | y | Y | yes) return 0 ;;
        *) return 1 ;;
    esac
}

apt_get() {
    if [ "$assume_yes" -eq 1 ]; then
        as_root env DEBIAN_FRONTEND=noninteractive apt-get "$@"
    else
        as_root apt-get "$@"
    fi
}

install_apt_dependencies() {
    dist=$(codename)
    if [ -z "$dist" ]; then
        echo "init.sh: cannot determine the distribution codename" >&2
        exit 1
    fi

    need_update=0
    if ! grep -qs 'packages\.gobysoft\.org' /etc/apt/sources.list \
        /etc/apt/sources.list.d/*.list /etc/apt/sources.list.d/*.sources; then
        confirm "Add the GobySoft apt repository ($gobysoft_url $dist/)?" || exit 1
        if ! command -v gpg >/dev/null 2>&1; then
            apt_get update
            apt_get install -y --no-install-recommends gnupg
        fi
        keyring_tmp=$(mktemp)
        gpg --dearmor <"$root/cmake/keys/gobysoft.asc" >"$keyring_tmp"
        as_root install -d -m 0755 "$(dirname "$gobysoft_keyring")"
        as_root install -m 0644 "$keyring_tmp" "$gobysoft_keyring"
        rm -f "$keyring_tmp"
        entry="deb [signed-by=$gobysoft_keyring] $gobysoft_url $dist/"
        echo "+ echo '$entry' | tee $gobysoft_list"
        echo "$entry" | sudo_if_needed tee "$gobysoft_list" >/dev/null
        need_update=1
    fi

    packages=$(apt_packages "$root/core/dependencies.yml" "$dist")
    missing=""
    for pkg in $packages; do
        if ! dpkg-query -W -f='${Status}' "$pkg" 2>/dev/null | grep -q 'install ok installed'; then
            missing="$missing $pkg"
        fi
    done

    if [ -z "$missing" ]; then
        echo "core apt dependencies are installed"
        if [ "$need_update" -eq 1 ]; then
            apt_get update
        fi
        return
    fi

    apt_get update
    install_flags="--no-install-recommends"
    if [ "$assume_yes" -eq 1 ]; then
        install_flags="-y $install_flags"
    fi
    # shellcheck disable=SC2086 # word splitting of the flag and package lists is intended
    apt_get install $install_flags $missing
}

build_tool() {
    # a core-only tree, so the tool builds even when an active toolbox is missing or broken
    tool_build="$root/build/lamss-tool"
    run cmake -S "$root/core" -B "$tool_build" -G Ninja \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=OFF
    run cmake --build "$tool_build" --target lamss
    run cmake --install "$tool_build" --component lamss_tool
}

link_tool() {
    bin_dir="$HOME/.local/bin"
    tool="$root/build/usr/bin/lamss"
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

    echo
    echo "Done. Set up your shell (add to ~/.bashrc to make it permanent):"
    # shellcheck disable=SC2016 # printed for the user to run
    echo '  eval "$(lamss env)"'
    case ":$PATH:" in
        *":$bin_dir:"*) ;;
        *)
            echo "$bin_dir is not on your PATH; add it first:"
            # shellcheck disable=SC2016 # printed for the user to run
            echo '  export PATH="$HOME/.local/bin:$PATH"'
            ;;
    esac
    echo "Then build with: lamss build"
}

if [ "$install_deps" -eq 1 ]; then
    install_apt_dependencies
fi
build_tool
link_tool
