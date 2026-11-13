# Mission components

*This page written by Claude*

`core` provides the components every platform needs: interprocess communication, logging and health monitoring, plus `view_message` for the MOOS apps. They live in [`core/missions/component/`](../../../missions/component), with the shared templates in [`core/missions/templates/`](../../../missions/templates). See [Concepts](../../../../docs/missions/concepts.md) and [Templates](../../../../docs/missions/templates.md) for how components work in general.

| Component | Processes | Requires | Port |
|---|---|---|---|
| [`interprocess`](#interprocess) | `gobyd`; with `moos`: `MOOSDB`, `goby_moos_gateway` | | `moosdb` |
| [`health`](#health) | `goby_coroner` | `interprocess` | |
| [`logging`](#logging) | `goby_logger` | `interprocess` | |
| [`view_message`](#view_message) | `pViewMessage` | `interprocess` with `moos` | |
| [`node_report`](#node_report) | `pNodeReporter` | `interprocess` with `moos` | |
| [`acomms`](#acomms) (`udp`) | `pAcommsHandler` | `interprocess` with `moos` | `acomms` |
| [`moos_link`](#moos_link) | `pMOOSLink` | `interprocess` with `moos` | |

## interprocess

[`interprocess.py`](../../../missions/component/interprocess/interprocess.py): one `gobyd` per subsystem and, while lamss still has MOOS apps, a `MOOSDB` bridged to it by `lamss_goby_moos_gateway` with the lamss [gateway plugin](../libraries/moos_gateway_plugin.md). MOOS app components put their `ProcessConfig` blocks into this component's `last.moos` as [fragments](../../../../docs/missions/templates.md#fragments).

| Parameter | Default | Meaning |
|---|---|---|
| `moos` | `true` | run `MOOSDB` and the gateway and render `last.moos`; `false` leaves only `gobyd` |
| `verbosity` | `WARN` | terminal verbosity of the Goby apps: `QUIET`, `WARN`, `VERBOSE`, `DEBUG1`, `DEBUG2` or `DEBUG3` |

| Process | Command | Configuration |
|---|---|---|
| `gobyd` | `gobyd gobyd.pb.cfg` | `gobyd.pb.cfg.j2` |
| `MOOSDB` | `MOOSDB last.moos` | `last.moos.j2` (MOOS only) |
| `goby_moos_gateway` | `lamss_goby_moos_gateway goby_moos_gateway.pb.cfg` | `goby_moos_gateway.pb.cfg.j2` (MOOS only) |

- **Community and platform:** the MOOS community is the platform's name on its main subsystem and `<platform>_<subsystem>` on the others (`community` in the context). The Goby platform is the platform's name.
- **Socket:** each subsystem's `gobyd` listens on `gobyd` in the subsystem's run directory (or a [shorter path](../../../../docs/missions/templates.md#filters) when that is too long), so several platforms run side by side on one machine.
- **Environment:** `GOBY_INTERPROCESS` is set to the same platform and socket for the subsystem's processes, so a Goby app started from [`env.sh`](../../../../docs/missions/output.md#envsh) joins the right `gobyd` without a configuration file.
- **Tests:** it gives the subsystem's [`goby test` endpoint](../../../../docs/missions/testing.md#endpoints-from-components), named `<platform>_<subsystem>`.
- `MOOSDB` listens on the subsystem's `moosdb` port, which the gateway's configuration uses too.

`--set platform.<p>.subsystem.<s>.component.interprocess.moos=false` turns MOOS off, but only where no MOOS app is listed: `view_message` refuses to run without it.

## health

[`health.py`](../../../missions/component/health/health.py): `goby_coroner`, which asks the subsystem's Goby apps for their health and reports it.

| Parameter | Default | Meaning |
|---|---|---|
| `request_interval` | `5` | seconds between health requests |
| `response_timeout` | `3` | seconds an app has to answer |

The coroner expects (`expected_name`) every process of the subsystem created with `Process(..., health=True)`, and adds any other app that answers. The list is `component.health.expected` in the context; for the `example` topside it is `gobyd`, `goby_moos_gateway`, `goby_coroner` and `goby_logger`. A component whose process answers Goby health requests should set `health=True` on it.

## logging

[`logging.py`](../../../missions/component/logging/logging.py): `goby_logger`, writing the subsystem's interprocess messages to its log directory (`paths.log`, inside the launch's [log directory](../../../../docs/missions/output.md#logs)).

| Parameter | Default | Meaning |
|---|---|---|
| `group_regex` | `.*` | groups to log |
| `type_regex` | `.*` | types to log |

It loads every installed `liblamss_<toolbox>_protobuf.so`, so lamss messages are logged with their types.

## view_message

[`view_message.py`](../../../missions/component/view_message/view_message.py): [`pViewMessage`](../apps/pViewMessage.md), a MOOS app, configured by a `pViewMessage.moos.j2` fragment in `last.moos`.

| Parameter | Default | Meaning |
|---|---|---|
| `generate_view_objects` | `true` | `generate_view_objects` in its `ProcessConfig` |
| `destination` | `1` | `destination` in its `ProcessConfig` |

It requires `interprocess` with `moos: true`; otherwise resolving the mission fails with `... view_message is a MOOS app: needs interprocess.moos`.

## Shared templates

The Goby apps' configurations all start with [`app.pb.cfg.j2`](../../../missions/templates/app.pb.cfg.j2):

```jinja
app {
  glog_config { tty_verbosity: {{ component.interprocess.verbosity }} }
{%- if cruise.warp != 1 %}
  simulation { time { use_sim_time: true warp_factor: {{ cruise.warp }} } }
{%- endif %}
}
interprocess {
  platform: {{ platform.name | pb_string }}
  socket_name: {{ paths.run | socket_path("gobyd") | pb_string }}
}
```

so every Goby app of a subsystem shares its verbosity and `gobyd`, and runs on simulated time when `warp` is not 1. A component for another Goby app includes it the same way (`{% include "app.pb.cfg.j2" %}`), which needs `interprocess` in the subsystem. `last.moos.j2` sets `MOOSTimeWarp` from `warp` and `LatOrigin`/`LongOrigin` from the cruise's `datum`.

| Template | Renders | For |
|---|---|---|
| `app.pb.cfg.j2` | the `app` and `interprocess` blocks | included by the others |
| `gobyd.pb.cfg.j2` | `gobyd.pb.cfg` | `interprocess` |
| `goby_moos_gateway.pb.cfg.j2` | `goby_moos_gateway.pb.cfg`: the MOOSDB port, the lamss plugin's libraries | `interprocess` |
| `last.moos.j2` | `last.moos`, with the fragments | `interprocess` |
| `goby_coroner.pb.cfg.j2` | `goby_coroner.pb.cfg` | `health` |
| `goby_logger.pb.cfg.j2` | `goby_logger.pb.cfg` | `logging` |
| `pViewMessage.moos.j2` | a `last.moos` fragment | `view_message` |

## Choosing files and processes from parameters

`interprocess` runs `MOOSDB` and the gateway, and renders `last.moos`, only when `moos` is true. A component does this by overriding, besides `active_processes()`:

- `active_templates()`: the templates to render (default: `templates`);
- `active_fragments()`: the fragments to contribute (default: `fragments`).

```python
class Interprocess(Component):
    params = dict(moos=True, verbosity="WARN")
    ports = ["moosdb"]

    def active_processes(self):
        processes = [Process("gobyd", "gobyd gobyd.pb.cfg", "gobyd.pb.cfg.j2", health=True)]
        if self.params["moos"]:
            processes += [Process("MOOSDB", "MOOSDB last.moos"),
                          Process("goby_moos_gateway", "lamss_goby_moos_gateway goby_moos_gateway.pb.cfg",
                                  "goby_moos_gateway.pb.cfg.j2", health=True)]
        return processes

    def active_templates(self):
        return ["last.moos.j2"] if self.params["moos"] else []
```

The `moosdb` port is allocated whether or not MOOS runs.

## node_report

*This section written by Claude*

[`node_report.py`](../../../missions/component/node_report/node_report.py): MOOS-IvP's `pNodeReporter`, which posts the platform's `NODE_REPORT` (named after the MOOS community) from its `NAV_*` variables. pMarineViewer draws it, and the gateway plugin publishes it to Goby as `lamss.core.LegacyNodeReport` on `lamss::moos::NODE_REPORT`. `platform_type` is the platform's `type`. It is a stand-in until the `lamss_node_report` Goby app of [plan 09](../plan/claude/09-toolboxes.md) exists.

## acomms

*This section written by Claude*

[`acomms.py`](../../../missions/component/acomms/acomms.py): `pAcommsHandler`, configured from the cruise so that every platform agrees on the message set and the MAC. It runs from its own `acomms.moos`, rendered by [`acomms.moos.j2`](../../../missions/component/acomms/acomms.moos.j2).

| Cruise parameter | Meaning |
|---|---|
| `acomms.messages` | names of message templates `acomms/<name>.pb.cfg.j2`, found in any active toolbox's `missions/templates/`. Each defines the macros `translator()` (its `translator_entry`) and `queue()` (its `message_entry`), and sees the platform, so a topside and a vehicle can queue a message differently |
| `acomms.slots` | the `MAC_FIXED_DECENTRALIZED` cycle: a list of `src`, `dest` (default -1), `rate` (default 0) and `seconds` |

- The platform's `modem_id` is required.
- `modemidlookup.txt` maps every platform's `modem_id` to its name and type, for the translator's `name2modem_id` and `modem_id2name`.
- `pAcommsHandler` sets `VEHICLE_NAME` and `VEHICLE_TYPE`, which `pInitializer` used to set.
- `LIAISON_COMMANDER_OUT` is its multiplex variable. A protobuf message published on the gateway plugin's `lamss::goby::pAcommsHandler_multiplex` group is therefore queued for sending, as goby_liaison's commander does.

| Implementation | Driver | Notes |
|---|---|---|
| [`udp`](../../../missions/component/acomms/udp/udp.py) | `DRIVER_UDP` | listens on the subsystem's `acomms` port and sends to every other platform's (`ip`, default `127.0.0.1`) |

`core` provides the `lamss_status` message ([`templates/acomms/lamss_status.pb.cfg.j2`](../../../missions/templates/acomms/lamss_status.pb.cfg.j2)): the `missions-lamss` `LAMSS_STATUS` entry, sent every 5 s on demand. Where it is received it posts `LAMSS_STATUS_IN`, `NODE_REPORT`, `<NAME>_NAV_*` and the rest the old topside read. The publishes of the thermocline fields that `lamss.core.LAMSS_STATUS` no longer has are dropped.

## moos_link

*This section written by Claude*

[`moos_link.py`](../../../missions/component/moos_link/moos_link.py): [pMOOSLink](../apps/pMOOSLink.md) copies MOOS variables between this subsystem's MOOSDB and the one of another subsystem of the same platform (`remote`, default `main`). It is a stand-in until those subsystems share a Goby interprocess bus.

| Parameter | Default | Meaning |
|---|---|---|
| `remote` | `main` | the other subsystem |
| `inputs` | `[]` | variables pulled from the remote (`in =`) |
| `outputs` | `[]` | variables pushed to the remote (`out =`); `LOCAL:REMOTE` renames |
