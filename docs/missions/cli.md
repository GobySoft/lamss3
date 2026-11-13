# lamss mission

*This page written by Claude*

`lamss mission` (`lm m`) lists, generates, starts, watches, tests and stops missions.

```sh
lm m list                              # cruises and the variants that apply to each
lm m show harbor quick                 # the resolved mission
lm m sim harbor                        # generate and start every platform on this machine
lm m status
lm m tail -f shore_main:daemon
lm m stop
lm m test pair                         # the cruise's goby test files against a fresh instance
```

It runs `python3 -m lamss.mission` from the install prefix (`build/usr`) with that prefix's `bin`, `lib` and Python directories first on `PATH`, `LD_LIBRARY_PATH` and `PYTHONPATH`, and `LAMSS_ROOT` and `LAMSS_PREFIX` set, passing every argument through; so build first (`lm b`). The mission classes and templates are read from the checkout, not the prefix, so an edit to them needs no rebuild. The checkout is found as for the other commands, from the current directory upward.

Every subcommand takes `--help`. Errors print `lamss mission: error: <what is wrong>` and exit with status 1. Output is coloured on a terminal, unless `NO_COLOR` is set.

The examples use the test fixture checkout [`core/src/test/mission/fixtures/render`](../../core/src/test/mission/fixtures/render) (cruise `harbor`, whose processes are `sleep`); those of `test` use [`fixtures/goby`](../../core/src/test/mission/fixtures/goby) (cruise `pair`: two platforms, each running `gobyd`). To try them there, copy it first, since generating writes into it, and run the package directly:

```sh
cp -r core/src/test/mission/fixtures/render /tmp/harbor
LAMSS_ROOT=/tmp/harbor PYTHONPATH=$PWD/core/src/python python3 -m lamss.mission list
```

## Common arguments

| Argument | Meaning |
|---|---|
| `cruise` | a cruise's name, as `list` shows it |
| `variant` | optional: one [variant](variants.md) name |
| `--set KEY=VALUE` | override a parameter (repeatable); see [Parameters](parameters.md#overriding-a-parameter) |
| `--platform P` | only this platform (repeatable) |
| `--subsystem S` | only subsystems of this name (repeatable) |
| `--instance NAME` | name of the output and run directories, default `<cruise>[_<variant>]` ([Output](output.md#instances)) |

Notes about optional components left out (`note: ... optional component 'viewer' left out`) go to stderr.

## list

```
lamss mission list [cruise]
```

Without a cruise, one line per cruise: its name, its toolbox and the variants that apply to it. With one, its fleet and environment, each platform's subsystems with their components, and its variants.

```
$ lm m list
harbor  extra         quick
$ lm m list harbor
harbor (extra): fleet duo
  shore/main: daemon, moos, app
  buoy/main: daemon, moos, app
  buoy/sensors: daemon
  variants: quick
```

## show

```
lamss mission show cruise [variant] [--set KEY=VALUE]... [--mode sim|run] [--explain KEY]...
```

Prints the resolved mission dictionary as YAML (as `mission.yaml` holds it). `--mode run` resolves it for run time. `--explain KEY` prints, instead, every layer that sets `KEY` and which wins ([Parameters](parameters.md#--explain)):

```
$ lm m show harbor --explain port_base
port_base = 24000
  cruise    Cruise.params  port_base = 9000
  cruise    Harbor.params  port_base = 24000  <- wins
```

## generate

```
lamss mission generate cruise [variant] [--set KEY=VALUE]... [--mode sim|run]
                       [--platform P]... [--subsystem S]... [--instance NAME] [--explain KEY]...
```

Writes `missions/build/<instance>/` ([Output](output.md)) and starts nothing. `--mode run` needs `--platform`. With `--explain`, it explains instead of generating, as `show` does.

```sh
lm m generate harbor quick                                 # build/harbor_quick/
lm m generate harbor --platform buoy --subsystem sensors   # only buoy/sensors
lm m generate harbor --mode run --platform buoy            # build/harbor/run/supervisord.conf
```

## sim

```
lamss mission sim cruise [variant] [--set KEY=VALUE]... [--platform P]... [--subsystem S]...
                  [--instance NAME] [--generate] [--execute] [--foreground] [--wait SECONDS]
```

Generates the mission in `sim` mode and starts the selected subsystems (by default every subsystem of every platform) under one supervisord on this machine, then waits until every process is `RUNNING` ([Output](output.md#starting)).

| Option | Effect |
|---|---|
| `--generate` | only generate, as `generate` does |
| `--execute` | start what was generated before, without generating |
| `--foreground` | keep supervisord in the foreground until Ctrl-C, SIGINT or SIGTERM |
| `--wait SECONDS` | how long processes may take to reach `RUNNING` (default 60) |

```sh
lm m sim harbor --set warp=10
lm m sim harbor quick --set port_base=25000 --set port_ranges.supervisor=51001   # beside a running harbor
```

## run

```
lamss mission run cruise [variant] --platform P [--subsystem S]... (options as for sim)
```

The same in `run` mode, for a vehicle or topside computer: no environment, `run`-mode subsystems and components, and only the named platform's subsystems. `--platform` is required.

```sh
lm m run harbor --platform buoy --subsystem main
```

## test

```
lamss mission test [options] cruise [variant] [-- goby test run options]
```

Generates the mission as its own instance, `<cruise>[_<variant>]_test`, on a free block of ports, and runs each of the cruise's `goby test` files against it, starting and stopping the mission around each file ([Testing](testing.md)). Exits with the worst `goby test` status: 0 when every test passed, 1 when one failed, 2 when one could not run.

| Option | Effect |
|---|---|
| `--test-file FILE` | run this file instead of the cruise's `test/` directory (repeatable) |
| `--wait SECONDS` | how long processes may take to reach `RUNNING` (default 60) |
| `--goby GOBY` | the `goby` tool (default: `goby` on `PATH`) |
| `--set port_base=N`, `--set port_ranges.<range>=N` | use these ports instead of free blocks |
| `-- ...` | everything after `--` goes to `goby test run` |

`--set`, `--platform`, `--subsystem` and `--instance` work as for `sim`.

```sh
lm m test pair
lm m test pair --test-file ~/checks/crossed.yml --instance crossed
lm m test pair -- -o JSON --output_file report.json
```

## status

```
lamss mission status [instance]
```

Lists every process of each running instance (or of the one named) with its state, and the instance's [web page](output.md#supervisords-web-page). Exits 0 when every process is `RUNNING`, 3 otherwise or when none is running.

```
$ lm m status
harbor (/home/me/lamss3/extra/missions/run/harbor) http://127.0.0.1:51000/:
  buoy_main:app                            RUNNING   pid 4127, uptime 0:00:12
  ...
```

## tail

```
lamss mission tail [program] [-f] [--instance NAME]
```

Prints a process's output; `-f` follows it. A program is named `<platform>_<subsystem>:<process>` (`shore_main:daemon`), or by its process name alone when that is unique (`gobyd`). Without a program, the last 40 lines of supervisord's own log. `--instance` is needed when several instances are running.

## stop

```
lamss mission stop [instance]
```

Stops supervisord and every process of the instance; the instance is needed when several are running.
