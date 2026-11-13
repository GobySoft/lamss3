# Templates

*This page written by Claude*

Each subsystem's configuration files are rendered from [Jinja2](https://jinja.palletsprojects.com/) templates with one dictionary, the **context**. Templates hold loops and conditionals only; anything that needs arithmetic or lookups is computed in Python (`derived()`, see [Concepts](concepts.md#the-mission-dictionary)) and placed in the dictionary. The code is in [`render.py`](../../core/src/python/lamss/mission/render.py).

Examples on this page come from the test fixture checkout [`core/src/test/mission/fixtures/render`](../../core/src/test/mission/fixtures/render): cruise `harbor`, fleet `duo` of a `shore` station and a `buoy`, whose components are `daemon`, `moos` and `app`. Its expected output is in [`fixtures/render_expected/harbor`](../../core/src/test/mission/fixtures/render_expected/harbor).

## What a component renders

```python
class Daemon(Component):
    params = dict(seconds=300)
    ports = ["daemon"]
    processes = [Process("daemon", "sleep {{ component.daemon.seconds }}", "daemon.cfg.j2")]


class Moos(Component):
    ports = ["moosdb"]
    templates = ["last.moos.j2"]
    processes = [Process("moosdb", "sleep 301", startsecs=0)]
```

For each component of a subsystem, in order, the renderer writes:

- each of its `templates`, and
- the `config_template` of each of its active processes,

into the subsystem's build directory, named after the template without `.j2` (`daemon.cfg.j2` → `daemon.cfg`). Two templates writing the same file in one subsystem is an error, as is a template named `context.yaml`, `env.sh` or `supervisord.conf`.

A process's `command` is itself a template string, rendered with the same context: `sleep {{ component.daemon.seconds }}` becomes `sleep 300`.

## Where templates are found

The first match wins, in this order:

1. the cruise class's directory, then those of its base classes: cruise-specific overrides (`cruise/harbor/daemon.cfg.j2` replaces every subsystem's `daemon.cfg`);
2. the component class's directory, then those of its base classes: templates that belong to one component;
3. each active toolbox's `missions/templates/`, the last toolbox in `.lamss/toolboxes` first and `core` last: templates shared by several components.

A template may `{% include %}` or `{% extends %}` any template on the same path.

## The context

Templates for a subsystem see:

| Key | Holds |
|---|---|
| `mode` | `sim` or `run` |
| `variant` | the variant's name, or `null` |
| `instance` | the [instance](output.md#instances) name, e.g. `harbor` or `harbor_quick` |
| `cruise` | the cruise's parameters: `name`, `datum`, `warp`, `port_base`, `port_ranges`, `supervisor`, ... |
| `fleet` | `name` and `platforms`, the list of platform names |
| `environment` | the environment's parameters, or `null` in `run` mode or without one |
| `platform` | this platform's parameters and all its `subsystem`s |
| `subsystem` | this subsystem: `name`, `host`, `ports` and its `component`s |
| `component` | this subsystem's components by name, each with its parameters |
| `platforms` | every *other* platform, by name, for peer and modem tables |
| `paths` | `root` (the checkout), `prefix` (the install prefix), `build`, `run` and `log` (this subsystem's directories), `data` (each toolbox's `missions/data`, by name) and `proto_libs` (the installed `liblamss_<toolbox>_protobuf.so` files) |
| `fragments` | rendered fragments by target file; see [below](#fragments) |

The renderer writes the context, without `fragments`, to `context.yaml` in the subsystem's directory before rendering anything, so you can see exactly what the templates saw. An abbreviated `shore/main/context.yaml`:

```yaml
mode: sim
variant: null
instance: harbor
cruise: {name: harbor, datum: {lat: 42.358, lon: -71.087}, warp: 1, port_base: 24000, port_ranges: {http: 50000, supervisor: 51000}, supervisor: {http_port: null, http_address: 127.0.0.1}}
fleet: {name: duo, platforms: [shore, buoy]}
environment: null
platform:
  name: shore
  type: station
  modem_id: 1
  subsystem:
    main: {name: main, host: localhost, ports: {daemon: 24000, moosdb: 24001}, component: ...}
subsystem: {name: main, host: localhost, ports: {daemon: 24000, moosdb: 24001}, component: ...}
component:
  daemon: {name: daemon, seconds: 300}
  moos: {name: moos}
  app: {name: app, rate: 4, label: 100% app}
paths:
  root: /home/me/lamss3
  prefix: /home/me/lamss3/build/usr
  build: /home/me/lamss3/extra/missions/build/harbor/shore/main
  run: /home/me/lamss3/extra/missions/run/harbor/shore/main
  log: /home/me/lamss3/extra/missions/run/harbor/log/shore/main
  data: {core: /home/me/lamss3/core/missions/data, extra: /home/me/lamss3/extra/missions/data}
  proto_libs: []
platforms:
  buoy: {name: buoy, type: buoy, modem_id: 2, subsystem: ...}
```

`paths.log` goes through the stable `run/<instance>/log` link, so generated files do not change from one launch to the next ([Output](output.md#logs)).

Templates render with `StrictUndefined`: a misspelt key is an error naming the template, the line and the component, not an empty string. An error inside an included template names that template.

```
lamss mission: error: daemon.cfg.j2:9 for platform.shore.subsystem.main.component.daemon: 'dict object' has no attribute 'missing'
```

## Filters

| Filter | Gives | Example |
|---|---|---|
| `pb_string` | a quoted, escaped Protobuf TextFormat string | `{{ platform.name \| pb_string }}` → `"shore"` |
| `latlon` | `lat: <lat> lon: <lon>` from a `{lat, lon}` dictionary | `datum { {{ cruise.datum \| latlon }} }` |
| `utm` | `{zone, hemisphere, easting, northing}` (WGS84, metres) of a `{lat, lon}` dictionary | `{{ (cruise.datum \| utm).zone }}` → `19` |
| `socket_path(name)` | `<directory>/<name>`, or a short path under `$TMPDIR/lamss-<uid>/` when that is too long for a unix socket | `{{ paths.run \| socket_path("gobyd") \| pb_string }}` |

The fixture's shared template, [`fixtures/render/core/missions/templates/daemon.cfg.j2`](../../core/src/test/mission/fixtures/render/core/missions/templates/daemon.cfg.j2):

```jinja
platform: {{ platform.name | pb_string }}
datum { {{ cruise.datum | latlon }} }
utm_zone: {{ (cruise.datum | utm).zone }}
port: {{ subsystem.ports.daemon }}
log_dir: {{ paths.log | pb_string }}
{% for name in platforms | sort %}peer { name: {{ name | pb_string }} modem_id: {{ platforms[name].modem_id }} }
{% endfor %}
```

renders for `shore/main` as

```
platform: "shore"
datum { lat: 42.358 lon: -71.087 }
utm_zone: 19
port: 24000
log_dir: "/home/me/lamss3/extra/missions/run/harbor/log/shore/main"
peer { name: "buoy" modem_id: 2 }
```

## Fragments

Some files are shared by several components: a MOOS community's `last.moos` holds one `ProcessConfig` block per app. One component owns the file and renders it; others contribute **fragments**:

```python
class App(Component):
    requires = [Moos]
    params = dict(rate=4, label="100% app")
    fragments = {"last.moos": ["app.moos.j2"]}
    processes = [Process("app", "sleep 302", autorestart=True)]
```

Fragments are rendered first, in component order, with the context. The owner's template then finds them, as a list of strings, in `fragments["<file>"]`:

```jinja
ServerHost = {{ subsystem.host }}
ServerPort = {{ subsystem.ports.moosdb }}
Community  = {{ platform.name }}_{{ subsystem.name }}
LatOrigin  = {{ cruise.datum.lat }}
LongOrigin = {{ cruise.datum.lon }}
{% for fragment in fragments.get("last.moos", []) %}
{{ fragment }}{% endfor %}
```

A fragment for a file that no component renders goes nowhere; `requires` makes sure the owner is there.

## Ports

A component lists the names of the TCP or UDP ports it needs (`ports = ["moosdb"]`), or maps names to a named range (`ports = {"liaison": "http"}`), and templates find the numbers in `subsystem.ports.<name>`. Ports are allocated when the cruise is resolved:

- subsystem by subsystem, in fleet and platform order, and component by component;
- counting up from the cruise's `port_base` (default `9000`), skipping any port already given;
- a port with a range counts up from `port_ranges.<range>` instead; the default ranges are `http` (50000, for web pages such as goby_liaison, away from MOOS's 9000s) and `supervisor` (51000, for supervisord's own page). A range the cruise does not define is an error;
- a port set in the subsystem's `ports` parameter is used as is: `--set platform.shore.subsystem.main.ports.moosdb=24002`.

`harbor` sets `port_base = 24000`, so `shore/main` gets `daemon: 24000, moosdb: 24001`, `buoy/main` `24002` and `24003`, and `buoy/sensors` `daemon: 24004`. Two components of one subsystem asking for the same name is an error. A fleet of several platforms on one machine therefore needs no hand-kept port table, and a second instance of the same cruise runs beside the first with `--set port_base=...` (and `--set port_ranges.supervisor=...` for its web page).

## Environment variables

Every process of a subsystem, and its [`env.sh`](output.md#envsh), gets:

| Variable | Value |
|---|---|
| `LAMSS_ROOT`, `LAMSS_PREFIX` | the checkout and the install prefix |
| `LAMSS_INSTANCE`, `LAMSS_CRUISE` | the instance and cruise names |
| `LAMSS_PLATFORM`, `LAMSS_SUBSYSTEM` | this subsystem's platform and name |
| `LAMSS_RUN`, `LAMSS_LOG` | `paths.run` and `paths.log` |
| `LAMSS_WARP` | `cruise.warp` |

plus `PATH` and `LD_LIBRARY_PATH` with the prefix's `bin` and `lib` first. A component adds its own by overriding `environment()`:

```python
class Daemon(Component):
    ports = ["daemon"]

    def environment(self, paths):
        return {"DAEMON_PORT": str(self.subsystem.allocated_ports["daemon"])}
```

`paths` is the context's `paths` dictionary, for variables that name files of this subsystem, such as a Goby socket (from the test fixture [`fixtures/goby`](../../core/src/test/mission/fixtures/goby/core/missions/component/gobyd/gobyd.py)):

```python
class Gobyd(Component):
    def environment(self, paths):
        socket = socket_path(Path(paths["run"]), "gobyd")       # from lamss.mission.render
        return {"GOBY_INTERPROCESS": f'platform: "{self.platform.name}" socket_name: "{socket}"'}
```

A variable set to two different values in one subsystem is an error. supervisord cannot pass a value containing a quote, comma or newline, so such a variable is left out of `supervisord.conf` and set only in `env.sh`.

A component can also give the [`goby test`](testing.md) endpoint for its subsystem by overriding `test_endpoint(paths)`.
