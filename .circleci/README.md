# CircleCI

*This document written by Claude*

`config.yml` runs one workflow, `ci`, on every push (and on pull requests, once enabled below). The plan behind it is in [04-dependencies-ci-packaging](../core/src/doc/plan/claude/04-dependencies-ci-packaging.md#ci).

## Jobs

| Job | Image | Does | Proves |
|---|---|---|---|
| `format` | `ubuntu:noble`, small | `clang-format-18 --dry-run --Werror` on every tracked `*.cpp *.cc *.h *.hpp *.proto` | The tree matches `.clang-format`. Passes trivially when there are no such files |
| `build-all-<codename>-<arch>` | `ubuntu:<codename>` for jammy, noble, resolute; `large` (amd64) and `arm.large` (native arm64) | `./init.sh --yes`, then (with `~/.config/lamss/env.sh` loaded) `lamss toolbox activate autonomy sonar virtual_ocean gui mit`, `lamss dependencies install --yes`, `lamss build --jobs 4 --test -D LAMSS_WERROR=ON` (4 jobs: in the Docker executor `nproc` reports the host's CPUs, and more parallel compiles than the resource class has CPUs exhausted its 8 GB) | `git clone && ./init.sh && lamss build` works on a clean system, with every in-tree toolbox built together, on each supported distribution and architecture |
| `build-<tb>-standalone` | `ubuntu:noble`, large | `./init.sh --yes`, `lamss toolbox activate <tb>`, `lamss dependencies install --yes` (so only `core` and `<tb>` dependencies are present), then plain CMake: build and install `core` into `build/usr`, then `cmake -S <tb>` against it, install and `ctest`. `<tb>` is `core`, `autonomy`, `sonar`, `virtual_ocean`, `gui` and `mit` | Each toolbox builds alone from its own directory, finding upstream toolboxes only through their installed CMake package config, with nothing but its declared dependencies installed |
| `docs` | `ubuntu:noble`, large | `./init.sh --yes`, then `lamss docs build --strict` and `lamss docs check`; the site is stored as the job's artifact | The documentation site builds with no broken links or missing pages, its generated reference (messages, commands) builds from this commit, and every inferred page's interface tables match the code. Publishing is a GitHub Actions workflow, [`.github/workflows/docs.yml`](../.github/workflows/docs.yml), on `main` |
| `mission` | `ubuntu:noble`, large | `./init.sh --yes`, `lamss build`, then `lamss mission sim example`, `status`, `test` and `stop`, and checks no mission process is left | The mission command works end to end from the `lamss` tool on a clean system: generation, supervisord, `goby test` and shutdown (ctest in the `build-*` jobs runs the same cruise's test through `lamss_add_mission_test`) |
| `env-switch` | `ubuntu:noble`, large | `init.sh` in this checkout and in a second clone, then `lamss env list` and `lamss env switch lamss3` | The active checkout follows `lamss env switch` in the running shell, the previous checkout's entries leave `PATH`/`LD_LIBRARY_PATH`/`PYTHONPATH`, and `init.sh` added the `~/.bashrc` hook |

Jobs call `lamss` rather than `lm`: `lm` is a shell alias, which non-interactive shells do not expand.

Standalone builds use `build/standalone/<tb>` so they never share a build tree with `lamss build`.

`clang-format` is pinned to 18, the version in noble, so the result does not change with the image. To reproduce locally: `git ls-files -z -- '*.cpp' '*.cc' '*.h' '*.hpp' '*.proto' | xargs -0 -r clang-format-18 -i`.

There is no packaging or upload yet (Phase 5), and no cache.

## One-time setup in CircleCI

1. In CircleCI, open the GobySoft organisation, **Projects**, and **Set Up Project** for `lamss3`, using the existing `.circleci/config.yml`.
2. **Project Settings → Advanced**: turn on **Build forked pull requests** if pull requests from forks should run. Pull requests from branches of `GobySoft/lamss3` run anyway, as pushes.
3. The arm64 jobs use the Docker `arm.large` resource class. Check that the organisation's plan offers it (**Organization Settings → Plan**); if not, change `resource_class: arm.large` in the `ci` workflow to one the plan does offer, such as `arm.medium`.
4. Once the jobs have run once, make `format`, `docs`, `env-switch`, `mission`, `build-all-*` and `build-*-standalone` required status checks on the `3.y` development branch in GitHub.

## GitHub Pages

*This section written by Claude*

The documentation is published by GitHub Actions, not CircleCI: [`.github/workflows/docs.yml`](../.github/workflows/docs.yml) builds the site on every push to `main` (or when run by hand from the **Actions** tab) and deploys it with `actions/deploy-pages`, so there is no `gh-pages` branch and no deploy key. Once, in GitHub: **Settings → Pages → Build and deployment → Source: GitHub Actions**, and **Settings → Actions → General** allows Actions to run.
