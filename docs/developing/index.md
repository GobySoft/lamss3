# Developing lamss3

*This page written by Claude*

- [Build system](build-system.md): how toolboxes become one CMake build, and the CMake helpers.
- [Porting old code](porting.md): bringing an app or library over from the old repositories with its history.
- [Testing and CI](testing.md): the tests, and what CircleCI runs.
- [Documentation](documentation.md): writing pages, and inferred pages for ported code.

## Conventions

- **Branches and pull requests:** `<bug|feature|minor>/<short-name>`, titled `<type> / <description>`. See [AGENTS.md](../../AGENTS.md) for the rules agents follow.
- **Formatting:** [`.clang-format`](../../.clang-format), checked in CI with `clang-format-18`.
- **Licences:** new files are `Copyright <year> GobySoft, LLC`; files ported from the old repositories keep `Copyright <first year>-<year> Massachusetts Institute of Technology`. Every source file has an SPDX line: `GPL-3.0-or-later` for applications and anything that links MOOS-IvP, `LGPL-3.0-or-later` for other libraries. See [plan 08](../../core/src/doc/plan/claude/08-code-style-licensing-maintenance.md).
- **DCCL IDs** are allocated in [DCCL_IDS.md](../../core/src/lib/protobuf/lamss/DCCL_IDS.md); a test checks every toolbox's messages against it.
