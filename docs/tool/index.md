# The lamss tool

*This page written by Claude*

`lamss` (alias `lm`) builds lamss3, manages toolboxes and dependencies, sets up shells, and builds this site. It is a small C++17 program that depends only on yaml-cpp, so `init.sh` can build it before anything else is installed.

| Command | Short | Does |
|---|---|---|
| [`lamss build`](build.md) | `lm b` | configure, build and install the active toolboxes |
| [`lamss toolbox`](toolbox.md) | `lm tb` | list, add, remove, activate and deactivate toolboxes |
| [`lamss dependencies`](dependencies.md) | `lm dep` | list and install the packages the active toolboxes need |
| [`lamss env`](env.md) | | load a checkout in your shells, switch between checkouts |
| [`lamss docs`](docs.md) | | build, serve and publish this site |
| [`lamss clean`](clean.md) | | remove build products |

Every command and subcommand takes `--help`; a command that needs a subcommand prints its help when given none. `-v` prints each command the tool runs. The [command reference](../reference/cli.md) collects all the help text.

The tool finds the checkout by walking up from the current directory to the one holding `.lamss/`, so it works from any subdirectory.

The design is in [plan 03](../../core/src/doc/plan/claude/03-lamss-tool.md).
