---
title: pLamssAEL
toolbox: sonar
kind: goby-moos-app
source: sonar/src/bin/pLamssAEL/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# pLamssAEL

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

pLamssAEL ("array element localization") computes where the acoustic array is and how it is
oriented: position, depth, heading, pitch and roll **[code]**. For a hull-mounted ("fixed") array
it offsets the vehicle's navigation along the heading; for a towed array it uses the vehicle's
own position from the time it was where the array is now; for a moored array it uses a
configured position **[code]**. It is the first app of the passive sonar chain **[guess]**: it
reads `NAV_DATA` (published by the frontseat interface **[guess]**), and its `AEL_PITCH` feeds
[pLamssCBF](pLamssCBF.md), its `AEL_NAV` feeds [pLamssBearingTracker](pLamssBearingTracker.md),
and its `AEL_HEADING` feeds [pLamssTrackManager](pLamssTrackManager.md) and the tracking
behaviors (`BHV_MaintainBearing`, `BHV_WiggleTail`) **[code]**. In simulation, virtual_ocean's
`uSimTowedArray` publishes the same `AEL_*` variables itself **[code]**.

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `NAV_DATA` | MOOS | string: serialized `goby.middleware.frontseat.protobuf.NodeStatus` | `handle_nav_data` | vehicle navigation **[code]** | uses `local_fix` x/y, `global_fix.depth`, `pose` heading/pitch/roll (degrees) and `speed.over_ground` **[code]**; for a mooring it replaces the configured position **[code]** |
| `LAT_ORIGIN` | MOOS | double | `handle_lat_origin` | new local-grid origin latitude **[code]** | NaN ignored; geodesy re-initialised in `loop()` **[code]** |
| `LONG_ORIGIN` | MOOS | double | `handle_lon_origin` | new local-grid origin longitude **[code]** | NaN ignored **[code]** |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `cfg_.ael_preample() + "_NAV"` (built at run time) | MOOS | string: serialized `lamss.sonar.ArrayNavigation` | `LamssAEL.cpp` | array position (`ael_x`, `ael_y`, `ael_z` = -depth), heading (degrees), pitch and roll (radians) and time **[code]** | `AEL_NAV` by default; every app tick once `filter_length` NAV_DATA messages have arrived (or at once for a mooring) **[code]**; read by pLamssBearingTracker **[code]** |
| `cfg_.ael_preample() + "_PITCH"` (built at run time) | MOOS | double | `LamssAEL.cpp` | filtered array pitch, radians **[code]** | `AEL_PITCH` by default; read by pLamssCBF **[code]** |
| `cfg_.ael_preample() + "_HEADING"` (built at run time) | MOOS | double | `LamssAEL.cpp` | filtered array heading, degrees 0–360 **[code]** | `AEL_HEADING` by default; read by pLamssTrackManager and tracking behaviors **[code]** |
| `cfg_.ael_preample() + "_X"` (built at run time) | MOOS | double | `LamssAEL.cpp` | array x on the local grid, m **[code]** | `AEL_X` by default; no consumer in lamss3 **[code]** |
| `cfg_.ael_preample() + "_Y"` (built at run time) | MOOS | double | `LamssAEL.cpp` | array y on the local grid, m **[code]** | `AEL_Y` by default; no consumer in lamss3 **[code]** |
<!-- END GENERATED publishes -->

### Configuration

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `common` | GobyMOOSAppConfig |  | no | Configuration common to all Goby MOOS applications |
| `array_type` | ArrayTypeEnum | `ARRAY_TYPE_FIXED` | no | `ARRAY_TYPE_FIXED` (offset along heading) or `ARRAY_TYPE_TOWED` (vehicle position from `-array_center_offset / speed` seconds ago) **[code]** |
| `array_center_offset` | double | `0` | no | distance of the array centre from the navigation reference along the vehicle axis, m; negative is aft **[code]** (example: -38.8 m for the towed array) |
| `filter_length` | int32 | `15` | no | length of the moving average on heading, pitch and roll; also the number of NAV_DATA messages before the first output **[code]** |
| `is_mooring` | bool | `false` | no | use the `deploy_*` position instead of waiting for NAV_DATA **[code]** |
| `deploy_x` | double | `0` | no | mooring x on the local grid **[code]**; one of `deploy_x`/`deploy_lon` (oneof) |
| `deploy_lon` | double | `0` | no | mooring longitude, used when `deploy_x`/`deploy_y` are not both set **[code]** |
| `deploy_y` | double | `0` | no | mooring y on the local grid **[code]**; one of `deploy_y`/`deploy_lat` (oneof) |
| `deploy_lat` | double | `0` | no | mooring latitude **[code]** |
| `deploy_depth` | double | `0` | no | mooring depth, m **[code]** |
| `deploy_heading` | double | `0` | no | mooring heading **[code]**; not used in the output, see open questions |
| `ael_preample` | string | `AEL` | no | prefix of every published variable **[code]** ("preamble" **[guess]**) |
<!-- END GENERATED config -->

Example (from `pLamssAEL.moos`):

```
ProcessConfig = pLamssAEL
{
  common { app_tick: 2 comm_tick: 5 }

  array_type: ARRAY_TYPE_TOWED
  array_center_offset: -38.8
  ael_preample: "AEL"
  filter_length: 40
  is_mooring: false
}
```

### Files, devices and external programs

None **[code]**.

## Behaviour

- Every NAV_DATA message replaces the last navigation and feeds three moving averages of
  `filter_length` samples: heading (degrees, `CAngleAvg`), pitch and roll (converted to radians,
  `CRadianAvg`) **[code]**.
- Output starts once `filter_length` NAV_DATA messages have arrived, or immediately for a mooring
  with a position configured **[code]**. After that it publishes every app tick, even if NAV_DATA
  stops **[code]**.
- **Fixed array, or towed array at zero speed:** array x/y = vehicle x/y +
  `array_center_offset · cos(pitch)` along the heading; heading and pitch are the filtered values
  **[code]**.
- **Towed array, moving:** each tick the current navigation is appended to a history; entries
  older than `-array_center_offset / speed_over_ground` seconds are pruned, and the oldest one left
  is used as the array position, heading and pitch **[code]**. This assumes the array follows the
  vehicle's track **[guess]**.
- **Mooring:** the position comes from `deploy_x`/`deploy_y` (converted to lat/lon) or from
  `deploy_lat`/`deploy_lon` (converted to x/y), and depth from `deploy_depth` **[code]**. The
  position is recomputed when the origin changes **[code]**. The smoke test configures a mooring
  at x=100, y=200, depth 50 and sees `ael_x: 100 ael_y: 200 ael_z: -50` in `AEL_NAV` **[test]**.
- `LAT_ORIGIN`/`LONG_ORIGIN` re-initialise the geodesy in the next `loop()` **[code]**.
- `slow_loop()` runs every 10 s and does nothing **[code]**.

## Running it

- Not launched by any mission component yet **[code]**.
- Standalone: `MOOSDB` and `pLamssAEL pLamssAEL.moos` in `sonar/src/bin/pLamssAEL/`. It needs
  NAV_DATA (a serialized `NodeStatus`) to publish anything unless `is_mooring` is set **[code]**.
- Tested by `lamss_sonar_moos_smoke`: started from its example, and as a moored array feeding
  pLamssCBF and pLamssBearingTracker, where `AEL_NAV` is checked **[test]**.

## Known issues and open questions

- [GobySoft/lamss3#32](https://github.com/GobySoft/lamss3/issues/32): `last_slow_loop_t_` is never
  initialised.
- For a mooring, `deploy_heading` is stored in the navigation but `computeAEL()` takes heading,
  pitch and roll from the moving averages, which no NAV_DATA has fed: the published heading is
  always 0. Should a mooring publish `deploy_heading`?
- In the towed branch the history entries never get a roll value, so `ael_roll` in `AEL_NAV` is
  uninitialised. Should it be the filtered roll, as in the fixed branch?
- The towed branch only uses navigation it has stored itself, one entry per app tick, so the
  look-back resolution is the app tick. Is that sufficient, or should NAV_DATA times be used?
- `AEL_X` and `AEL_Y` have no reader in lamss3. Are they for logging or for a topside display?
- The angle conversion uses `pi = 3.14159` for pitch and roll and `M_PI` elsewhere.
- `ael_preample`: is "preamble" meant? Renaming would change the configuration.

## History

- From GobySoft/lamss-shared `src/moos/pLamssAEL`; first commit 2015-12-21 (Henrik Schmidt,
  "new pLamssAEL GobyMOOS App for array element location calculation"); 17 commits by Henrik
  Schmidt, Toby Schneider and Supun Randeni before the port, the last on 2021-06-08 (dynamic UTM
  handling).
- lamss3 port (GobySoft/lamss3#21–#23): clang-format; builds against `lamss::sonar_protobuf`
  (`ArrayNavigation` is now `lamss.sonar.ArrayNavigation`) and core's henrik_util; config proto
  in `package lamss.sonar`, proto2; warning fixes with no change of behaviour; example `.moos` and
  smoke test added.
