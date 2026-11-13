# Tutorial: the example cruise

*This page written by Claude*

This walks through `core`'s [`example` cruise](../../core/src/doc/missions/example.md): look at it, generate it, run it, test it, then add a variant and a component. It needs a built checkout with `core` (`lm b`), which installs `supervisord`, Goby and MOOS through `lm dep install`.

## 1. Look

```
$ lm m list
example  core
$ lm m list example
note: platform.topside.subsystem.main: optional component 'viewer' left out (no active toolbox defines it)
example (core): fleet example_fleet, environment synthetic
  topside/main: interprocess, health, logging, view_message
  variants: -
```

The cruise is three small classes:

```python
# core/missions/cruise/example/example.py
class Example(Cruise):
    fleet = "example_fleet"
    environment = "synthetic"
    params = dict(datum=dict(lat=42.358, lon=-71.087))

# core/missions/fleet/example_fleet/example_fleet.py
class ExampleFleet(Fleet):
    platforms = [Topside]

# core/missions/platform/topside/topside.py
class Topside(Platform):
    params = dict(type="topside")
    components = [Interprocess, Health, Logging, ViewMessage, optional("viewer")]
```

The note says that `viewer`, from the `gui` toolbox, is not active; the mission runs without it.

`show` prints the resolved mission, and `--explain` where a value comes from:

```
$ lm m show example
mode: sim
variant: null
cruise:
  name: example
  datum:
    lat: 42.358
    lon: -71.087
  warp: 1
  port_base: 9000
  ...
platform:
  topside:
    ...
    subsystem:
      main:
        ...
        ports:
          moosdb: 9000
        component:
          interprocess:
            name: interprocess
            moos: true
            verbosity: WARN
            community: topside
          health:
            ...
            expected:
            - gobyd
            - goby_moos_gateway
            - goby_coroner
            - goby_logger
          ...
$ lm m show example --set warp=10 --explain warp
warp = 10
  cruise    Cruise.params  warp = 1
  set       --set warp     warp = 10  <- wins
```

## 2. Generate

```
$ lm m generate example
generated /home/me/lamss3/core/missions/build/example
$ ls core/missions/build/example/topside/main
context.yaml  env.sh  goby_coroner.pb.cfg  goby_logger.pb.cfg  goby_moos_gateway.pb.cfg
gobyd.pb.cfg  last.moos  supervisord.conf
```

Each file is a rendered [template](templates.md); `context.yaml` is what the templates saw. `gobyd.pb.cfg`:

```
app {
  glog_config { tty_verbosity: WARN }
}
interprocess {
  platform: "topside"
  socket_name: "/home/me/lamss3/core/missions/run/example/topside/main/gobyd"
}
```

`last.moos` holds the MOOS community, with `pViewMessage`'s block added by the `view_message` component as a [fragment](templates.md#fragments). Try `lm m generate example --set warp=10` and look again: `gobyd.pb.cfg` gains `simulation { time { use_sim_time: true warp_factor: 10 } }` and `last.moos` has `MOOSTimeWarp = 10`. The [output page](output.md) describes `supervisord.conf` and `env.sh`.

## 3. Run

```
$ lm m sim example
generated /home/me/lamss3/core/missions/build/example
example: 6 processes RUNNING
  web:     http://127.0.0.1:51000/
  logs:    /home/me/lamss3/core/missions/log/example/2026-11-13T14-00-05Z
  status:  lamss mission status example
  stop:    lamss mission stop example
$ lm m status
example (/home/me/lamss3/core/missions/run/example) http://127.0.0.1:51000/:
  topside_main:MOOSDB                      RUNNING   pid 41207, uptime 0:00:09
  topside_main:goby_coroner                RUNNING   pid 41210, uptime 0:00:08
  ...
$ lm m tail -f goby_coroner
```

The web page shows the same and can stop and restart single processes. Every process's output is in the logs directory, under `topside/main/`, and `goby_logger`'s log of the Goby messages beside it.

## 4. Test

The cruise has a `goby test` file, [`test/example.yml`](../../core/missions/cruise/example/test/example.yml): it waits for the Goby apps to report healthy, then sends a node report from Goby into MOOS and back.

```sh
lm m test example
```

This runs a second instance, `example_test`, on free ports, so it works while `example` is running; it starts and stops that instance itself and exits 0 when the test passed. [Testing](testing.md) explains test files.

```sh
lm m stop
```

## 5. Add a variant

A variant changes the cruise only when asked for. Create `core/missions/cruise/example/example_fast.py` (or the same path under `missions/` in your own toolbox, which extends `core`'s tree):

```python
from lamss.mission import variant

from missions.cruise.example import Example


@variant("fast")
class ExampleFast(Example):
    params = dict(warp=10)
```

```
$ lm m list
example  core          fast
$ lm m show example fast --explain warp
warp = 10
  cruise    Cruise.params       warp = 1
  variant   ExampleFast.params  warp = 10  <- wins
$ lm m sim example fast
```

The variant's instance is `example_fast`, with its own directories, so it can run beside `example` (give it other ports with `--set port_base=...`). No rebuild is needed: missions are read from the checkout. [Variants](variants.md) covers combining them.

## 6. Add a component

Goby's `goby_opencpn_interface` serves the vehicles' positions as AIS to OpenCPN. A component runs it. Create `core/missions/component/opencpn/opencpn.py`:

```python
from lamss.mission import Component, Process

from missions.component.interprocess import Interprocess


class Opencpn(Component):
    requires = [Interprocess]
    params = dict(mmsi_start=988700000)
    ports = ["ais"]
    processes = [Process("goby_opencpn_interface", "goby_opencpn_interface opencpn.pb.cfg",
                         "opencpn.pb.cfg.j2")]
```

and its template beside it, `core/missions/component/opencpn/opencpn.pb.cfg.j2`, which starts with the shared Goby `app` and `interprocess` blocks:

```jinja
{% include "app.pb.cfg.j2" %}
ais_server { bind_port: {{ subsystem.ports.ais }} }
mmsi_start: {{ component.opencpn.mmsi_start }}
```

- `requires` makes a subsystem without `interprocess` an error.
- `ports = ["ais"]` gets a port [allocated](templates.md#ports), which the template reads as `subsystem.ports.ais`.
- `params` can be overridden like any other: `--set platform.topside.subsystem.main.component.opencpn.mmsi_start=...`.

Add it to the topside with a platform variant, `core/missions/platform/topside/topside_opencpn.py`:

```python
from lamss.mission import variant

from missions.component.opencpn import Opencpn
from missions.platform.topside import Topside


@variant("opencpn")
class TopsideOpencpn(Topside):
    components = Topside.components + [Opencpn]
```

```
$ lm m list example
...
  variants: fast, opencpn
$ lm m generate example opencpn
$ cat core/missions/build/example_opencpn/topside/main/opencpn.pb.cfg
app {
  glog_config { tty_verbosity: WARN }
}
interprocess {
  platform: "topside"
  socket_name: "/home/me/lamss3/core/missions/run/example_opencpn/topside/main/gobyd"
}
ais_server { bind_port: 9001 }
mmsi_start: 988700000
$ lm m test example opencpn
```

To make it part of every topside instead, add `Opencpn` to `Topside.components`; to make it optional on a platform whose toolbox might not have it, list `optional("opencpn")`. The [core components](../../core/src/doc/missions/components.md) are fuller examples, including components that choose their processes and files from their parameters.
