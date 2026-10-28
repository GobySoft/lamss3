# lamss3
LAMSS: Autonomous Marine Sensing Systems version 3

## Getting started

*This section written by Claude*

On Ubuntu 22.04, 24.04 or 26.04:

```sh
./init.sh                  # builds the lamss tool, links ~/.local/bin/lamss and lm
eval "$(lamss env)"        # PATH, LD_LIBRARY_PATH, PYTHONPATH, LAMSS_ROOT; add to ~/.bashrc
lm b                       # lamss build: configure, build and install the active toolboxes into build/usr
lm tb activate autonomy    # lamss toolbox activate: add a toolbox (and what it requires) to the build
lm b
```

`lamss --help` lists the commands, and every command takes `--help`. `./init.sh --yes` runs without prompts (CI); `./init.sh --no-deps` installs no packages.
