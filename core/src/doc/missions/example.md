# The example cruise

*This page written by Claude*

`example` is the smallest runnable mission: one [topside](platforms.md) with the core [components](components.md), in a synthetic environment. It needs only `core` (and uses `gui`'s [viewer](../../../../gui/src/doc/missions/viewer.md) when that is active), so it is what CI runs and where the [tutorial](../../../../docs/missions/tutorial.md) starts.

| Class | File | Holds |
|---|---|---|
| `Example(Cruise)` | [`cruise/example/example.py`](../../../missions/cruise/example/example.py) | fleet `example_fleet`, environment `synthetic`, `datum` 42.358 N, 71.087 W |
| `ExampleFleet(Fleet)` | [`fleet/example_fleet/example_fleet.py`](../../../missions/fleet/example_fleet/example_fleet.py) | one `Topside` |
| `Synthetic(Environment)` | [`environment/synthetic/synthetic.py`](../../../missions/environment/synthetic/synthetic.py) | see below |

```
$ lm m list example
note: platform.topside.subsystem.main: optional component 'viewer' left out (no active toolbox defines it)
example (core): fleet example_fleet, environment synthetic
  topside/main: interprocess, health, logging, view_message
  variants: -
```

In simulation the topside's `main` subsystem runs six processes (seven with `gui`):

| Process | Priority | From |
|---|---|---|
| `gobyd` | 10 | `interprocess` |
| `MOOSDB` | 11 | `interprocess` |
| `goby_moos_gateway` | 12 | `interprocess` |
| `goby_coroner` | 20 | `health` |
| `goby_logger` | 30 | `logging` |
| `pViewMessage` | 40 | `view_message` |
| `goby_liaison` | 50 | `viewer`, with `gui` |

With the default ports, `MOOSDB` listens on 9000 (from `port_base`), `goby_liaison` on 50000 when the gui toolbox is active (from `port_ranges.http`), and supervisord's web page on 51000 (from `port_ranges.supervisor`).

## The synthetic environment

A flat bottom in isovelocity water, needing no data files: a base for test cruises.

| Parameter | Value |
|---|---|
| `bathymetry` | `{kind: flat, depth: 100.0}` |
| `sound_speed` | `1500.0` |
| `currents` | `null` |
| `ice` | `null` |

It has no simulator components; its parameters are in the context as `environment.*` for those that will use them.

## The test

[`cruise/example/test/example.yml`](../../../missions/cruise/example/test/example.yml) is a [`goby test`](../../../../docs/missions/testing.md) file. It checks, through the `topside_main` endpoint, that

1. `gobyd`, `goby_logger`, `goby_coroner` and `goby_moos_gateway` report healthy within 30 s, and
2. a `lamss.core.LegacyNodeReport` published on the Goby group `lamss::moos::NODE_REPORT::out` comes back on `lamss::moos::NODE_REPORT` with the same name, `x` and `y`: it crossed the gateway into MOOS and back, through the lamss [gateway plugin](../libraries/moos_gateway_plugin.md).

```sh
lm m test example
```

It runs in three places:

- **`lamss mission test example`**, by hand.
- **ctest**, as `lamss_core_mission_example`, registered with `lamss_add_mission_test` in [`core/src/test/mission_example/CMakeLists.txt`](../../test/mission_example/CMakeLists.txt) when `supervisord`, `gobyd` and `MOOSDB` are installed; so in every CI job that runs `lamss build --test`.
- **the `mission` CircleCI job** ([`.circleci/config.yml`](../../../../.circleci/config.yml)), which builds with the `lamss` tool on a clean system and runs

  ```sh
  lamss mission list
  lamss mission sim example --set port_base=29000
  lamss mission status example
  lamss mission test example          # instance example_test, beside the running example
  lamss mission stop example
  ```

  then fails if any mission process is left running, and prints the processes' logs when a step fails.

The engine's tests of the real trees are in [`core/src/test/mission/test_core_missions.py`](../../test/mission/test_core_missions.py): `example` with and without `gui`, its rendered files, that turning MOOS off is refused while `view_message` is listed, and that `warp` reaches the Goby apps and `last.moos`.
