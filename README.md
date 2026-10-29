# lamss3
LAMSS: Autonomous Marine Sensing Systems version 3

## Getting started

*This section written by Claude*

On Ubuntu 22.04, 24.04 or 26.04:

```sh
./init.sh                  # shows and asks before each step: builds the lamss tool, makes this the
                           # active checkout, loads it from ~/.bashrc, installs dependencies (via sudo)
. ~/.config/lamss/env.sh   # or open a new shell
lm b                       # lamss build: configure, build and install the active toolboxes into build/usr
lm tb activate autonomy    # lamss toolbox activate: add a toolbox (and what it requires) to the build
lm dep install             # lamss dependencies install: its packages
lm b
```

`lamss --help` lists the commands, and every command takes `--help`. `./init.sh --yes` runs every step without asking (CI); `./init.sh --no-deps` installs no packages; `lm dep install` installs what newly activated toolboxes need.

With more than one checkout, run `./init.sh` in each; `lamss env list` shows them and `lamss env switch <name or path>` changes which one your shells use.
