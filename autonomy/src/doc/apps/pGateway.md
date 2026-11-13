---
title: pGateway
toolbox: autonomy
kind: moos-app
source: autonomy/src/bin/pGateway/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# pGateway

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

pGateway publishes a fixed position, zero speed and heading, and a node report of type `ship`,
every iteration **[code]**. It stands in for a stationary node, such as a moored gateway buoy or a
ship holding station, so that other MOOS apps see its navigation as if it came from a real
navigation source **[guess]**. The M1 plan runs it on the topside for ship navigation, and the
lamss3 source inventory calls it a "fixed gateway/buoy node" **[guess]**.

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `VEHICLE_NAME` | MOOS | string | `Gateway.cpp`, `OnNewMail` | name written into `NAME=` of the node report **[code]** | empty until received, so the report starts with `NAME=,` **[code]** |
| `LAT_ORIGIN` | MOOS | double | `OnNewMail` | new origin latitude; re-initialises the geodesy **[code]** | never registered, so never received **[code]**; see open questions |
| `LONG_ORIGIN` | MOOS | double | `OnNewMail` | new origin longitude **[code]** | never registered, so never received **[code]** |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `NAV_X` | MOOS | double | `Gateway.cpp` | deploy position, local x (m) **[code]** | constant, every iteration **[code]** |
| `NAV_Y` | MOOS | double | `Gateway.cpp` | deploy position, local y (m) **[code]** | constant **[code]** |
| `NAV_HEADING` | MOOS | double | `Gateway.cpp` | always 0 **[code]** | posted twice per iteration **[code]** |
| `NAV_SPEED` | MOOS | double | `Gateway.cpp` | always 0 **[code]** |  |
| `NAV_DEPTH` | MOOS | double | `Gateway.cpp` | `deploy_depth` **[code]** | checked by `lamss_autonomy_moos_smoke` **[test]** |
| `NAV_LAT` | MOOS | double | `Gateway.cpp` | deploy latitude **[code]** |  |
| `NAV_LONG` | MOOS | double | `Gateway.cpp` | deploy longitude **[code]** |  |
| `NODE_REPORT_NMEA_SHIP` | MOOS | string: `NAME=…,TYPE=ship,UTC_TIME=…,X=…,Y=…,LAT=…,LON=…,SPD=…,HDG=…,DEPTH=…` | `Gateway.cpp` | node report for the fixed "ship" **[code]**; read by a topside display or the acomms status chain **[guess]** | `TYPE=ship` checked by `lamss_autonomy_moos_smoke` **[test]**; the same variable is the `out_status_var` of core iSerialNMEA **[code]** |
<!-- END GENERATED publishes -->

### Configuration

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `LatOrigin` | double |  | yes | global (mission file); start-up fails without it **[code]** |
| `LongOrigin` | double |  | yes | global (mission file); start-up fails without it **[code]** |
| `deploy_x` | double | `0` | no | deploy position, local x (m) **[code]**; converted to lat/lon at start-up **[code]** |
| `deploy_y` | double | `0` | no | deploy position, local y (m) **[code]** |
| `deploy_longitude` | double |  | no | deploy longitude; overrides `deploy_x`/`deploy_y` **[code]** |
| `deploy_latitude` | double |  | no | deploy latitude; overrides `deploy_x`/`deploy_y` **[code]** |
| `deploy_depth` | double |  | yes | depth posted as `NAV_DEPTH` (m) **[code]**; start-up fails without it **[code]** |
<!-- END GENERATED config -->

Example (from `pGateway.moos`):

```
ProcessConfig = pGateway
{
  AppTick    = 4
  CommsTick  = 4

  // May be specified in local grid or latitude and longitude
  // deploy_x = 2000
  // deploy_y = 2000
  deploy_latitude = 42.36
  deploy_longitude = -71.08
  deploy_depth = 84.7
}
```

### Files, devices and external programs

None **[code]**.

## Behaviour

- Start-up **[code]**:
    - reads the global `LatOrigin` and `LongOrigin` and initialises the local grid; fails if either is missing;
    - takes the deploy position from `deploy_x`/`deploy_y` (converted to latitude/longitude), then lets `deploy_latitude`/`deploy_longitude` override it;
    - converts the final latitude/longitude back to local x/y, so the posted `NAV_X`/`NAV_Y` always match `NAV_LAT`/`NAV_LONG`;
    - fails if `deploy_depth` is missing.
- Each iteration posts the same `NAV_*` values and one `NODE_REPORT_NMEA_SHIP` string **[code]**. Heading and speed are always 0 **[code]**. The `UTC_TIME` field is the current MOOS time **[code]**.
- The rate is the `AppTick` (4 Hz in the example) **[code]**.
- The mail handler can re-initialise the local grid from `LAT_ORIGIN`/`LONG_ORIGIN` **[code]**, but those variables are not registered **[code]**. The deploy x/y are not recomputed after a re-initialisation either **[code]**.

## Running it

- No mission component launches it yet.
- Standalone, from `autonomy/src/bin/pGateway/`: `MOOSDB pGateway.moos &` then `pGateway pGateway.moos` (the default mission file is `Mission.moos`; the process name is always `pGateway`) **[code]**.
- Tested by `lamss_autonomy_moos_smoke` (`autonomy/src/test/moos_smoke/moos_smoke.sh`): it checks that pGateway connects and posts `NODE_REPORT_NMEA_SHIP` with `TYPE=ship` and `NAV_DEPTH` **[test]**.

## Known issues and open questions

- `LAT_ORIGIN`/`LONG_ORIGIN` are handled in `OnNewMail` but never registered, so the 2021 "dynamic UTM" change has no effect **[code]**. Should they be registered? If so:
    - a message carrying only one of them re-initialises the grid with 0 for the other;
    - the deploy x/y would also need recomputing from the deploy latitude/longitude.
- With neither `deploy_x`/`deploy_y` nor `deploy_latitude`/`deploy_longitude` set, or with only one of `deploy_latitude`/`deploy_longitude`, the start-up conversion reads an uninitialised latitude or longitude **[code]**. Should one of the two forms be required?
- `NAV_HEADING` is posted twice per iteration **[code]**.
- The node report has `NAME=` empty until `VEHICLE_NAME` is received **[code]**. Is `VEHICLE_NAME` always set on the topside?
- Members `deploy_spd`, `deploy_hdg`, `depth_trigger`, `dest_addr` and `start_time` are unused or written but never read **[code]**. Leftovers from an earlier version? **[guess]**
- The start-up error message says "Geodesy Init inside pTargetSim failed" **[code]**, which suggests the app began as a copy of pTargetSim **[guess]**.

## History

- From GobySoft/lamss `src/moos/pGateway/` (imported at lamss 3186a63) **[code]**. First commit 2011-04-26 (Toby Schneider), when the MOOS processes used in missions-lamss were added. 9 commits before the port, by Toby Schneider and Henrik Schmidt; the last, in 2021-06, added `LAT_ORIGIN`/`LONG_ORIGIN` handling. In 2017 `NODE_REPORT_NMEA_SHIP` was added.
- lamss3 Phase 2 port (GobySoft/lamss3#21):
    - builds as a plain CMOOSApp with `CMOOSGeodesy` from goby_moos;
    - `pGateway.moos` gained the `LatOrigin`/`LongOrigin` globals it needs;
    - formatted with clang-format;
    - the origin variables in the start-up and mail handlers are now initialised to 0 (a gcc warning).
