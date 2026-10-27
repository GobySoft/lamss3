# Dependencies, CI and packaging

*This document written by Claude*

## Target platforms

| Platform | Role |
|---|---|
| Ubuntu 24.04 (noble), amd64 | Primary: developer machines, topside, CI |
| Ubuntu 24.04, arm64 | Vehicle payload computers (Jetson, PC/104 replacements); CI cross-check |
| Ubuntu 26.04 (resolute) | Secondary from Phase 5; Goby3 3.5+ already supports it |
| Ubuntu 22.04 (jammy) | Not supported by lamss3; the old repos remain the 22.04 option |

CMake range `3.22...4.2` lets 22.04's CMake configure the tree if someone tries, but no CI runs on it.

## Dependency sources

```mermaid
flowchart LR
  apt[Ubuntu archive<br/>boost, protobuf, gsl, netcdf, hdf5, armadillo, fltk, pcap, python3-*] --> lamss3
  goby[packages.gobysoft.org<br/>goby3, dccl4, netsim, acousticstoolbox, moos10, moos-ivp] --> lamss3
  src[Source builds<br/>IvPico, HydroMAN 3 submodule, DM7820 kernel module] --> lamss3
```

The old `DEPENDENCIES` Makefile pulled MOOS and MOOS-IvP from `ppa:tes/lamss-moos10` and `packages.gobysoft.org`. netsim 1.3.0's CI confirms that `libnetsim-dev`/`netsim-apps` and their Goby3/DCCL dependencies are built for noble. netsim also `find_package(MOOS 10)`s, so `libmoos10-dev` for noble exists on the same repository. `libmoos-ivp-dev`/`moos-ivp-apps` for noble still need checking **first thing in Phase 0** and fall back to a `cmake/BuildMOOSIvP.cmake` `ExternalProject` from the `moos-ivp` 24.8.1 release if not.

## `DEPENDENCIES` files

Format defined in [03](03-lamss-tool.md#lamss-dependencies-listinstall--lm-dep). Expected contents, derived from the old `DEPENDENCIES` and the survey ([01](01-source-inventory.md#dependencies-to-carry-forward)):

| Toolbox | apt packages | Source |
|---|---|---|
| core | `build-essential cmake ninja-build git libgoby3-dev libgoby3-moos-dev goby3-apps goby3-moos libdccl4-dev dccl4-compiler libprotobuf-dev protobuf-compiler libboost-all-dev libmoos10-dev moosdb10 moos10-essential libmoos-ivp-dev moos-ivp-apps nlohmann-json3-dev libnetcdf-dev libnetcdf-c++4-dev libgsl-dev python3-jinja2 python3-yaml supervisor` | — |
| autonomy | `libnetsim-dev` | — |
| hydroman | `libpcap-dev` + whatever `upstream/` needs (copied into this file when the wrapper is written) | HydroMAN 3 submodule |
| sonar | `libgsl-dev libhdf5-dev libfltk1.3-dev libhidapi-dev libusb-1.0-0-dev libnetsim-dev` | DM7820 DKMS |
| virtual_ocean | `libnetsim-dev netsim-apps acousticstoolbox libnetcdf-dev python3-netcdf4` | — |
| icex | `libboost-python-dev libopencv-core-dev libgoby3-gui-dev` | — |
| mit | `moos-ivp-gui` (pMarineViewer) `python3-pandas python3-numpy python3-scipy` | — |

Notes:

- Python packages come from apt only. The old `pip install -U bokeh websockets pymoos ...` is replaced; `pymoos` is not needed once the mission framework does not talk MOOS. Where a tool genuinely needs a pip package it runs in a venv created by `lamss dependencies install --python-venv`.
- `apt-key adv` is dead on 24.04; the GobySoft repo is added with a keyring file under `/etc/apt/keyrings/` and a `signed-by=` sources entry.
- `DEBIAN_FRONTEND=noninteractive` and `--no-install-recommends` for CI.

## CI

CircleCI, copying the pattern of [netsim](https://github.com/GobySoft/netsim)'s `.circleci/config.yml`: it already builds Debian packages for jammy, noble and bookworm on amd64, arm64 and armhf inside the `gobysoft/goby3-ubuntu-build-base:24.04.2` images and uploads to `packages.gobysoft.org` (`continuous/` for untagged builds, `release/` for tags). Reusing those images and that upload path means lamss3 packages land next to goby3, dccl and netsim with no new infrastructure. GitHub Actions would work equally well but would need the images and upload step recreated.

```mermaid
flowchart LR
  push[push / PR] --> fmt[clang-format check]
  push --> matrix
  subgraph matrix [build matrix: noble amd64, noble arm64, resolute amd64]
    each[each toolbox standalone<br/>find_package of installed upstream toolboxes]
    all[all in-tree toolboxes together]
  end
  matrix --> test[ctest + mission smoke sim]
  test --> pkg[debian packages: nightly + tags]
  pkg --> apt[GobySoft apt repo or GitHub releases]
```

Jobs:

| Job | Does |
|---|---|
| `format` | `clang-format --dry-run --Werror` over changed files |
| `build-<tb>-standalone` | Container with deps from `<tb>/DEPENDENCIES` + installed `.deb`s of required toolboxes from the previous stage; `cmake -S <tb>`; proves the toolbox is self-contained |
| `build-all` | `lm dep install && lm b --test` with every in-tree toolbox active |
| `mission-smoke` | `lm m sim mbat --variant smoke --execute` for 5 simulated minutes under `warp`; asserts supervisord shows all processes `RUNNING` and the mission manager reached `DEPLOY` |
| `package` | `cpack -G DEB` per toolbox; `lintian` |
| `docs` | Renders the mermaid figures and the Markdown docs into a static site (mkdocs-material) |

Submodule toolboxes (`mit`, `icex`, `attic`) run their own CI in their own repositories, triggered on `lamss3` tags, and are **not** part of the `lamss3` required checks. `attic` CI is allowed to be red. The `hydroman` wrapper builds in `lamss3` CI only if the CI runner has read access to the HydroMAN repository (a deploy key); otherwise the job is skipped, not failed.

Containers: the `gobysoft/goby3-ubuntu-build-*` images already carry Goby3's build dependencies; lamss3 adds a thin layer per distro (`gobysoft/lamss3-build-base:<codename>`) that installs `core/DEPENDENCIES`, rebuilt weekly so dependency drift shows up in the image build and not in every PR. netsim's CI still uses `apt-key adv`; lamss3 should use the keyring/`signed-by` form and offer that change back to netsim.

## Debian packaging (Phase 5)

- One source package per toolbox, `lamss3-<tb>`, producing `lamss3-<tb>` (apps + data), `liblamss3-<tb>-dev` (headers, CMake config, proto files) and `python3-lamss3-<tb>` where applicable.
- Generated with `cpack` from the same `install()` rules that fill `build/usr`, so packaging cannot drift from the developer build.
- `lamss3-core` ships the `lamss` tool and `bootstrap.sh` is unnecessary on packaged systems.
- Kernel module for DURIP as `rtd-dm7820-dkms`.
- Versioning: `lamss3` uses `MAJOR.MINOR.PATCH` git tags; toolboxes share the version (one tag releases all in-tree toolboxes). Submodule toolboxes declare the `lamss3` version they were tested against in their `lamss_toolbox()` call.

## Developer containers

`core/docker/Dockerfile` (replacing the old VNC/apache/mysql image) builds the CI image plus a user; `lamss docker` is deliberately **not** a subcommand: `docker run -v $PWD:/lamss3 ghcr.io/gobysoft/lamss3-ci:noble` is enough.

## Open questions for the human plan

- CircleCI + `packages.gobysoft.org` is assumed because netsim proves the path works for noble on three architectures; confirm GobySoft is willing to host lamss3 packages there.
- arm64/armhf builds in netsim's CI are cross-builds (`sbuild`, `nocheck`), which is fast enough for PRs; tests run on amd64 only. lamss3 follows that.
