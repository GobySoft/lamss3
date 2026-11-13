# Getting started

*This page written by Claude*

lamss3 builds on Ubuntu 22.04 (jammy), 24.04 (noble) and 26.04 (resolute), on amd64 and arm64.

## Install and build

```sh
git clone https://github.com/GobySoft/lamss3.git
cd lamss3
./init.sh
```

`init.sh` shows each step and asks before running it:

1. installs the few packages the `lamss` tool needs (a C++ compiler, CMake, Ninja, yaml-cpp);
2. builds the `lamss` tool into `build/usr`;
3. makes this checkout the active one (`lamss env switch`);
4. adds a block to `~/.bashrc` (and `~/.zshrc`) that loads the active checkout in every new shell (`lamss env hook`);
5. installs the active toolboxes' dependencies with `sudo apt-get` (`lamss dependencies install`).

`./init.sh --yes` runs every step without asking; `./init.sh --no-deps` installs no packages.

Then load the checkout in the current shell and build:

```sh
. ~/.config/lamss/env.sh   # or open a new shell
lm b                       # lamss build: configure, build and install into build/usr
```

`lm` is a shell alias for `lamss`, defined when no other `lm` exists. Every command takes `--help`.

## Add toolboxes

A fresh clone builds only `core`. To add more:

```sh
lm tb activate sonar       # also activates autonomy, which sonar requires
lm dep install             # their packages
lm b
```

See [toolboxes](tool/toolbox.md) for submodule toolboxes and deactivating.

## Several checkouts

Run `./init.sh` in each. `lamss env list` shows them, and `lamss env switch <name or path>` changes the one your shells use. See [env](tool/env.md).

## Next

- Run the tests: `lm b --test`.
- Read the [lamss tool guide](tool/index.md).
- Build these docs locally: `lamss docs serve` ([docs](tool/docs.md)).
