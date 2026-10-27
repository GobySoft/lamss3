Here are the detailed plans produced by Claude, referenced by the human-written plans in the "src/doc/plan" directory.

*This index written by Claude*

| Doc | Topic |
|---|---|
| [00-overview](00-overview.md) | Goals, guiding decisions, phases, milestones, schedule, risks |
| [01-source-inventory](01-source-inventory.md) | Survey of `lamss`, `lamss-shared`, `lamss-internal`, `missions-lamss`; what goes to which toolbox |
| [02-repository-layout-and-build](02-repository-layout-and-build.md) | Directory layout, CMake toolbox mechanism, activation file, `build/usr` |
| [03-lamss-tool](03-lamss-tool.md) | The `lamss`/`lm` C++ CLI: bootstrap and subcommands |
| [04-dependencies-ci-packaging](04-dependencies-ci-packaging.md) | Ubuntu 24.04 dependencies, CI matrix, Debian packaging |
| [05-middleware-moos-to-goby3](05-middleware-moos-to-goby3.md) | MOOS to Goby3 migration, CMOOSApp shim, pHelmIvP to IvPico |
| [06-missions-framework](06-missions-framework.md) | Python concept classes, Jinja2 rendering, supervisord output, variants |
| [07-protobuf-split](07-protobuf-split.md) | Splitting `lib_lamss_protobuf` by toolbox; DCCL ID ranges |
| [08-code-style-licensing-maintenance](08-code-style-licensing-maintenance.md) | clang-format, LGPL/GPL, maintainers, docs |
| [09-toolboxes](09-toolboxes.md) | Per-toolbox plans and decisions |
| [10-milestone-mbat-macrura](10-milestone-mbat-macrura.md) | M1 definition, port list, acceptance; M2/M3 sketches |

Figures are Mermaid blocks, rendered by GitHub and most Markdown viewers.
