---
title: iNetSimManager
toolbox: virtual_ocean
kind: goby-moos-app
source: virtual_ocean/src/bin/iNetSimManager/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# iNetSimManager

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

iNetSimManager forwards the vehicle's navigation and its acoustic modem receive statistics over
TCP to a `netsim_manager` server **[code]**. It sends nothing to MOOS **[code]**. It is the vehicle
side of the netsim acoustic network simulator: the server needs each vehicle's position to model
propagation between modems, and the receive statistics to compare simulated with real receptions
**[guess]**. Each vehicle is identified to the server by the TCP port of its (simulated) modem,
not by a modem ID **[code]**.

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `IFS_DATA_IN` | MOOS | protobuf | `handle_frontseat_data` | front-seat data; its `node_status` is sent as navigation **[code]** | only when `nav_source: IFRONTSEAT` (default); messages without `node_status` are ignored **[code]** |
| `cfg_.node_report_var()` (built at run time) | MOOS |  | `handle_node_report` | node report, sent as navigation **[code]** | `NODE_REPORT` by default; only when `nav_source: NODE_REPORT`; string `NAME=...,LAT=...,LON=...,DEPTH=...,SPD=...,HDG=...,UTC_TIME=...` (`DEP` and `TIME` also accepted) **[code]** |
| `ACOMMS_MODEM_RECEIVE` | MOOS | protobuf | `handle_modem_receive` | modem receptions; the first Micro-Modem receive statistic is forwarded **[code]** | `goby.acomms.protobuf.ModemTransmission` with the micromodem `transmission` extension **[code]** |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
<!-- END GENERATED publishes -->

None: the app publishes nothing to MOOS **[code]**.

### Configuration

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `common` | GobyMOOSAppConfig |  | no | Configuration common to all Goby MOOS applications |
| `netsim_manager_ip_addr` | string |  | yes | address of the `netsim_manager` server **[code]** |
| `netsim_manager_tcp_port` | uint32 | `61999` | no | its TCP port **[code]** |
| `reconnect_interval` | uint32 | `10` | no | seconds between connection checks; reconnects if disconnected **[code]** |
| `modem_tcp_port` | uint32 |  | yes | identifies this vehicle in every request (`nav.modem_tcp_port`, `stats.modem_tcp_port`) **[code]**; the TCP port of this vehicle's simulated modem on the netsim side **[guess]** |
| `nav_source` | NavigationSource | `IFRONTSEAT` | no | where navigation comes from: `IFRONTSEAT` (`IFS_DATA_IN`) or `NODE_REPORT` (`node_report_var`) **[code]** |
| `node_report_var` | string | `NODE_REPORT` | no | MOOS variable read when `nav_source: NODE_REPORT` **[code]** |
| `node_report_name` | string |  | no | `NAME` to accept in node reports (case-insensitive); defaults to the MOOS community **[code]** |
| `modem_id_mapping` | repeated ModemIdMapping |  | no | `{modem_id, modem_tcp_port}` pairs mapping the source modem ID of a reception to the sender's TCP port **[code]**; receptions from an unmapped ID are dropped with a warning **[code]** |
<!-- END GENERATED config -->

There is no example `.moos` file in the source directory; `iNetSimManager -e` prints one
(checked by `lamss_virtual_ocean_moos_smoke` **[test]**). The smoke test uses:

```
ProcessConfig = iNetSimManager
{
  netsim_manager_ip_addr: "<address>"
  netsim_manager_tcp_port: <port>
  modem_tcp_port: <port>
  nav_source: NODE_REPORT
}
```

### Files, devices and external programs

- A TCP client connection to `netsim_manager_ip_addr:netsim_manager_tcp_port` (netsim's
  `tcp_client`), carrying `netsim.protobuf.NetSimManagerRequest` out and
  `netsim.protobuf.NetSimManagerResponse` in **[code]**.
- Built only when netsim is installed (`NETSIM_FOUND`) **[code]**.

## Behaviour

- Connects at start-up and checks the connection every `reconnect_interval` seconds, reconnecting
  if it has dropped **[code]**. `loop()` (each app tick) polls the asio context, which runs the
  TCP reads and writes **[code]**.
- Navigation, `IFRONTSEAT`: each `IFS_DATA_IN` with a `node_status` becomes one request with a
  `nav` entry: time, latitude, longitude, depth, speed over ground, heading **[code]**.
- Navigation, `NODE_REPORT`: only reports whose `NAME` matches `node_report_name` (or the
  community) are used **[code]**. `DEP` is accepted for `DEPTH` and `TIME` for `UTC_TIME`
  **[code]**. A report missing any field is dropped with a warning **[code]**.
- Receptions: the first `receive_stat` of each `ACOMMS_MODEM_RECEIVE` becomes a `stats` entry with
  this vehicle's port, the sender's port (from `modem_id_mapping`), `packet_success` (mode
  `RECEIVE_GOOD`) and the full Micro-Modem statistics **[code]**.
- Every request carries an `id` that counts up from 0 **[code]**.
- Requests are sent only while connected; otherwise they are dropped without a message
  **[code]**. Nothing is queued **[code]**.
- Responses from the server are only logged (at `DEBUG1`) **[code]**.

## Running it

- No mission component launches it yet.
- Needs a running `netsim_manager` (part of netsim, not in lamss3) **[code]**. Without one it starts,
  warns every `reconnect_interval` seconds that it is not connected, and sends nothing **[code]**.
- Standalone: `MOOSDB` and `iNetSimManager <file>.moos` with the block above.
- Tested by `lamss_virtual_ocean_moos_smoke`, only when netsim is found: `-e` prints a
  configuration and the app connects to the MOOSDB **[test]**. The test starts no
  `netsim_manager`, so the TCP side is not exercised.

## Known issues and open questions

- `loop()` catches `boost::system::error_code`, but asio throws `boost::system::system_error`
  **[code]**, so the catch never matches and an error in `poll()` would end the app **[guess]**.
  Should it catch `boost::system::system_error` (or `std::exception`)?
- Requests made while disconnected are dropped silently. Is that intended (stale navigation is
  useless), or should reconnects resend the latest position?
- Only the first `receive_stat` of a reception is forwarded. Can a reception carry more than one
  (e.g. multiple frames), and should those be sent too?
- The `NODE_REPORT` path takes `SPD` and `HDG` as given, while the front-seat path uses speed over
  ground. Does netsim expect over-ground speed?
- The 2019 change to read `NAV_DATA` instead of `IFS_DATA_IN` was reverted the same year. Is
  `IFS_DATA_IN` still the right default for current vehicles?

## History

- From GobySoft/lamss-shared `src/moos/iNetSimManager` (imported with its history at `043f556`).
  First commit 2018-01-31 (Toby Schneider); 17 commits before the port, 16 by Toby Schneider and
  one by Supun Randeni. It began as a copy of pCommsStateTracker, whose history was not imported.
- Main changes in the old history: `NODE_REPORT` as a navigation source (2018), modem ID mapping
  (2019), `DEP`/`TIME` accepted (2019), Goby3 and netsim namespace updates (2019–2020).
- lamss3 port (Phase 2, GobySoft/lamss3#21–#23): clang-format, config proto in `package lamss.virtual_ocean`,
  MIT/GPL-3.0-or-later headers, and built only where netsim is installed.
