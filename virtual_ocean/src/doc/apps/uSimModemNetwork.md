---
title: uSimModemNetwork
toolbox: virtual_ocean
kind: goby-moos-app
source: virtual_ocean/src/bin/uSimModemNetwork/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# uSimModemNetwork

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

<!-- One short paragraph: what the app is for and where it sits in a mission (which apps feed it,
which consume its output). Tag every sentence. -->

## Interfaces

<!-- The tables between the GENERATED markers come from `lamss docs extract <app>`, and
`lamss docs check` (in CI) fails when they drift from the code. Rows are matched by name: the
script keeps "Meaning" and "Notes", and any cell the code does not determine (a refined
"Type / format", for example). Handler is the handling function; Where is the source file. -->

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `LAT_ORIGIN` | MOOS |  | `handle_lat_origin` |  |  |
| `LONG_ORIGIN` | MOOS |  | `handle_lon_origin` |  |  |
| `BELLHOP_RESPONSE` | MOOS |  | `handle_bellhop_response` |  |  |
| `SVP_RESPONSE` | MOOS |  | `handle_svp_response` |  |  |
| `NODE_REPORT` | MOOS |  | `handle_node_report` |  |  |
| `NETSIM_NAV_UPDATE` | MOOS | protobuf | `handle_netsim_nav_update` |  |  |
| `IMPULSE_REQUEST` | MOOS |  | `handle_impulse_request` |  |  |
| `PERFORMANCE_REQUEST` | MOOS |  | `handle_performance_request` |  |  |
| `TRANSMIT_TEST` | MOOS |  | `handle_transmit_test` |  |  |
| `AMBIENT_NL_DEPTH` | MOOS |  | `handle_ambient_nl_depth` |  |  |
| `NOISE_VERT_DIR` | MOOS |  | `handle_noise_vert_dir` |  |  |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `TRANSMIT_REQUEST` | MOOS | string | `SimModemNetwork.cpp` |  |  |
| `NODE_REPORT` | MOOS | string | `SimModemNetwork.cpp` |  |  |
| `IMPULSE_RESPONSE` | MOOS | string | `SimModemNetwork.cpp` |  |  |
| `PERFORMANCE_RESPONSE` | MOOS | string | `SimModemNetwork.cpp` |  |  |
| `BELLHOP_REQUEST` | MOOS |  | `SimModemNetwork.cpp` |  |  |
| `SVP_REQUEST` | MOOS |  | `SimModemNetwork.cpp` |  |  |
| `PERFORMANCE_REQUEST` | MOOS | string | `SimModemNetwork.cpp` |  |  |
<!-- END GENERATED publishes -->

### Configuration

<!-- For Goby-config apps (GobyMOOSApp, Goby3): generated from the config proto (fields, types,
defaults, (goby.field).description) and the `-e` example. For CMOOSApp: generated from the
GetConfigurationParam / m_MissionReader calls. -->

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `common` | GobyMOOSAppConfig |  | no | Configuration common to all Goby MOOS applications |
| `base_dir_name` | string |  | no |  |
| `acomms_frequency` | double | `10000` | no |  |
| `acomms_bandwidth` | double | `2000` | no |  |
| `test_replica` | string | `replica.dat` | no |  |
| `noise_level` | double | `40` | no |  |
| `max_number_bounces` | int32 | `1` | no |  |
| `raytracing_interval` | double | `30` | no |  |
| `raytracing_range_box` | double | `20` | no |  |
| `mseas_svp_updates` | bool | `false` | no |  |
| `mseas_update_interval` | double | `60` | no |  |
| `svp_range_resolution` | double | `200` | no |  |
| `svp_depth_resolution` | double | `20` | no |  |
| `node` | repeated string |  | no |  |
| `array_element` | repeated ArrayElement |  | no |  |
| `depth_performance_prediction` | bool | `false` | no |  |
| `min_depth` | double | `2` | no |  |
| `max_depth` | double | `200` | no |  |
| `num_depths` | int32 | `100` | no |  |
| `bounce_penalty` | int32 | `0` | no |  |
| `multipath_penalty` | int32 | `0` | no |  |
| `optimize` | ModemPerformanceMetric | `SNR` | no |  |
| `num_ff_tabs` | int32 | `21` | no |  |
| `num_fb_tabs` | int32 | `20` | no |  |
| `nbaud` | int32 | `18` | no |  |
| `source_level` | double | `183` | no |  |
| `isotropic_noise` | bool | `false` | no |  |
| `surface_rms_roughness` | double | `0` | no |  |
| `surface_roughness_loss` | double | `0` | no |  |
| `modem_id_lookup_path` | string | `modemidlookup.txt` | no |  |
<!-- END GENERATED config -->

Example (from `<app>.moos` / `<app> -e`):

```
ProcessConfig = <app>
{
}
```

### Files, devices and external programs

<!-- Files read or written (paths, formats), serial ports and sockets, programs run with system()
or similar. "None" if none. -->

## Behaviour

<!-- How inputs become outputs: the main loop, filters (e.g. "ignores its own posts"), state
machine (a mermaid stateDiagram for anything with states), timing (AppTick, timeouts). Tag
every statement. -->

## Running it

<!-- Which mission component launches it, minimal standalone run (MOOSDB + this app), and the
test(s) that exercise it. -->

## Known issues and open questions

<!-- Links to GitHub issues, and questions for the authors/maintainers: things the code does that
look unintended, unused inputs or outputs, magic numbers. Each open question is a candidate
issue. -->

## History

<!-- Origin repository and path, first commit date and authors (from `git log --follow`), and
what changed during the lamss3 port (removed features, renamed variables). -->
