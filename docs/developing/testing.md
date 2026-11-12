# Testing and CI

*This page written by Claude*

```sh
lm b --test                  # build, then run every active toolbox's tests
ctest --test-dir build/core -R moos   # some of core's tests, after a build
```

Tests live in each toolbox's `src/test/` and are registered with `lamss_add_test` (C++), `lamss_add_pytest` (Python), or plain `add_test` for scripts. Each toolbox has the same kinds:

| Test | Checks |
|---|---|
| `lamss_tool_*` (core) | the `lamss` tool: toolbox lists, requirements, dependencies, `env`, command parsing |
| `lamss_docs_python` (core) | how `lamss docs` rewrites links and builds the navigation |
| `lamss_<toolbox>_protobuf_dccl` | every DCCL message loads, and its ID is the one allocated in `DCCL_IDS.md` |
| `lamss_core_moos_gateway_plugin_load` | the Goby3–MOOS gateway plugin loads |
| `lamss_<toolbox>_moos_smoke` | MOOSDB, the gateway and the toolbox's MOOS apps start together and pass messages |

## CircleCI

[`.circleci/config.yml`](../../.circleci/config.yml) runs on every push; [`.circleci/README.md`](../../.circleci/README.md) describes each job. In short:

- `format`: `clang-format-18 --dry-run --Werror` on every C++ and proto file.
- `build-all-<release>-<arch>`: `./init.sh --yes`, every in-tree toolbox activated, `lamss build --test -D LAMSS_WERROR=ON`, on 22.04, 24.04 and 26.04, amd64 and arm64.
- `build-<toolbox>-standalone`: each toolbox built alone against an installed `core`, with only its own dependencies.
- `env-switch`: two checkouts and `lamss env switch`.
- `docs`: `lamss docs build --strict`.

Build with `-D LAMSS_WERROR=ON` locally before pushing: the three releases carry gcc 11, 13 and 15, which warn differently.
