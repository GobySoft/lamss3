---
title: uSimTargets
toolbox: virtual_ocean
kind: goby-moos-app
source: virtual_ocean/src/bin/uSimTargets/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# uSimTargets

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

uSimTargets keeps a list of simulated acoustic targets, each moving on a straight line at constant
speed and heading, and publishes their state for the sonar simulators and their position as
contacts **[code]**. Targets are created, changed and removed with `LAMSS_SIMULATE_TARGET` messages
(`TGT_STATE_IN`) or one `auto_target` from the configuration **[code]**. uSimPassiveSonar reads
`TARGET_SIM_LIST` and the `TGT_<n>_SIM_STATE` variables to synthesise the targets' signals
**[code]**. `LAMSS_SIMULATE_TARGET` is a DCCL message (ID 5133, 32 bytes) **[code]**, so targets can
be injected from the topside over the acoustic link **[guess]**. The `TGT_<n>_SIM_NAV_*` variables
are "for use by IvPContactBehavior" (code comment) and `NODE_REPORT` shows the targets in
pMarineViewer **[guess]**.

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `LAT_ORIGIN` | MOOS | double | `handle_lat_origin` | new local-grid origin latitude **[code]** | NaN ignored; targets are moved to the new grid in `loop()` **[code]** |
| `LONG_ORIGIN` | MOOS | double | `handle_lon_origin` | new local-grid origin longitude **[code]** | NaN ignored |
| `TGT_STATE_IN` | MOOS | string: protobuf `lamss.virtual_ocean.LAMSS_SIMULATE_TARGET` (`parse_for_moos`) | `handle_tgt_state` | create or update target `tgt_num` (> 0), cancel target `-tgt_num` (< 0); 0 is ignored **[code]** | the `_IN` suffix suggests it arrives over acomms **[guess]** |
| `NAV_X` | MOOS | double, m | `handle_nav_x` | own vehicle east position **[code]** | used for the `max_range` cut-off **[code]** |
| `NAV_Y` | MOOS | double, m | `handle_nav_y` | own vehicle north position **[code]** | as `NAV_X` |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `TARGET_RESET` | MOOS | string | `SimTargets.cpp` | always `TRUE`, after every target update **[code]** | no lamss3 subscriber **[code]** |
| `tstr + "_SIM_STATE"` (built at run time) | MOOS | string: `<n>,<on 0/1>,<time>,<x>,<y>,<spd>,<hdg>,<dep>,<spl>,<freq>,<bw>,<str>,[<tone freq>,<tone dB>,...]` | `SimTargets.cpp` | `TGT_<n>_SIM_STATE`: target `n` extrapolated to now (`CTarget::target_state`) **[code]** | every `loop()` per target **[code]**; read by uSimPassiveSonar **[code]** |
| `NODE_REPORT` | MOOS | string: `NAME=tgt_<n>_sim,TYPE=track,UTC_TIME=,X=,Y=,LAT=,LON=,SPD=,HDG=,DEPTH=` | `SimTargets.cpp` | target position as a MOOS-IvP node report **[code]** | every 10 s per target **[code]** |
| `TARGET_SIM_LIST` | MOOS | string: comma-separated target numbers (`int_vec2str`) | `SimTargets.cpp` | current target numbers **[code]** | after each change; empty when no targets **[code]**; read by uSimPassiveSonar **[code]** |
| `prefix + "_NAV_X"` (built at run time) | MOOS | double | `SimTargets.cpp` | `TGT_<n>_SIM_NAV_X`: target east position now **[code]** | every 10 s **[code]**; no lamss3 subscriber **[code]** |
| `prefix + "_NAV_Y"` (built at run time) | MOOS | double | `SimTargets.cpp` | `TGT_<n>_SIM_NAV_Y`: target north position now **[code]** | every 10 s |
| `prefix + "_NAV_DEPTH"` (built at run time) | MOOS | double | `SimTargets.cpp` | `TGT_<n>_SIM_NAV_DEPTH` **[code]** | the depth of the last target updated, not target `n` (see open questions) **[code]** |
| `prefix + "_NAV_SPEED"` (built at run time) | MOOS | double | `SimTargets.cpp` | `TGT_<n>_SIM_NAV_SPEED` **[code]** | the speed of the last target updated, not target `n` **[code]** |
| `prefix + "_NAV_HEADING"` (built at run time) | MOOS | double | `SimTargets.cpp` | `TGT_<n>_SIM_NAV_HEADING`: target heading, deg **[code]** | every 10 s |
| `prefix + "_NAV_UTC"` (built at run time) | MOOS | double | `SimTargets.cpp` | `TGT_<n>_SIM_NAV_UTC`: time of the report **[code]** | every 10 s |
<!-- END GENERATED publishes -->

### Configuration

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `common` | GobyMOOSAppConfig |  | no | Configuration common to all Goby MOOS applications; its `lat_origin`/`lon_origin` set the initial grid **[code]** |
| `max_range` | double |  | no | targets farther than this from the vehicle are removed **[code]**; unset: no limit, and targets do not wait for `NAV_X`/`NAV_Y` **[code]** |
| `auto_target` | LAMSS_SIMULATE_TARGET |  | no | a target created at start-up if its `tgt_num` > 0 **[code]**; see Behaviour |
<!-- END GENERATED config -->

There is no example `.moos` file in the source directory. `lamss_virtual_ocean_moos_smoke` runs the
app with an empty block, and checks that `uSimTargets -e` prints a `ProcessConfig = uSimTargets`
block **[test]**. A block with an auto target, written from the config proto (not tested):

```
ProcessConfig = uSimTargets
{
  max_range: 20000
  auto_target {
    tgt_num: 1
    tgt_x: 1000
    tgt_y: 2000
    tgt_dep: 50
    tgt_hdg: 90
    tgt_spd: 2.5
    tgt_freq: 300
    tgt_bw: 100
    tgt_spl: 150
    tones { freq: 150 db: 160 }
  }
}
```

### Files, devices and external programs

None **[code]**.

## Behaviour

- **Target model.** Each target has a start position, start time, speed, heading, depth and
  acoustic parameters (source level, frequency, bandwidth, `tgt_str`, up to four tones)
  (`lamss::core::CTarget`) **[code]**. Its current position is the start position plus
  speed × elapsed time along the heading **[code]**.
- **Updates.** A `LAMSS_SIMULATE_TARGET` sets all of a target's values at once **[code]**:
  - the position is `tgt_x`/`tgt_y`, or `tgt_lat`/`tgt_lon` converted to the local grid;
  - the start time is `tgt_utc`, or now if it is absent;
  - unset fields take the proto defaults (0) rather than keeping old values.
- **Each `loop()`** (`app_tick`), for every target **[code]**:
  - it computes the range from (`NAV_X`, `NAV_Y`) and publishes `TGT_<n>_SIM_STATE`;
  - beyond `max_range` the target is marked off, its state published once more, and the target
    removed from the list.
- Every 10 s it publishes `NODE_REPORT` and `TGT_<n>_SIM_NAV_*` for every target **[code]**.
- **Auto target.** The configured target is created in the constructor **[code]**. On the first
  `loop()` with navigation (or immediately without `max_range`), the app also adds target number
  1, starting now from the same position, and publishes the list **[code]**. The intent is to
  restart the auto target when navigation arrives (comment: "wait for nav data before publishing
  target list") **[guess]**; with `tgt_num` other than 1 this leaves two targets (see open
  questions).
- **Origin change.** On `LAT_ORIGIN`/`LONG_ORIGIN` each target is restarted at its current
  position, re-expressed in the new grid, with its start time set to now **[code]**.

## Running it

- No mission component launches it yet; the M1 plan puts it in the `macrura/sensor_sim` community
  with uSimPassiveSonar (`core/src/doc/plan/claude/10-milestone-mbat-macrura.md`) **[code]**.
- Standalone: run `MOOSDB` with a mission file containing the block above, then
  `uSimTargets <file>.moos`, and post `TGT_STATE_IN` or rely on `auto_target`.
- Tested by `lamss_virtual_ocean_moos_smoke` (`-e` output and MOOSDB connection) **[test]** and by
  `lamss_virtual_ocean_protobuf_dccl`, which round-trips a `LAMSS_SIMULATE_TARGET` through DCCL
  **[test]**. Target motion and outputs are not tested.

## Known issues and open questions

- `target_AIS` publishes `TGT_<n>_SIM_NAV_DEPTH` and `_NAV_SPEED` from the members `target_depth`
  and `target_speed`, which hold the last target updated, instead of target `n`'s own values
  (`target_dep`, `target_spd`, read a few lines above) **[code]**. With more than one target these
  are wrong.
- With `auto_target.tgt_num` other than 1, the first `loop()` with navigation adds a second target
  numbered 1 (`t_num` starts at 0) **[code]**. Is the auto target meant to be renumbered, or
  restarted?
- `lat_origin_` and `lon_origin_` are not initialised. If `LAT_ORIGIN` and `LONG_ORIGIN` arrive in
  different mail batches, `reinitialize_targets` initialises the geodesy with one uninitialised
  value **[code]**.
- Without `max_range`, `rangeCrit` is read uninitialised in `source_range < rangeCrit ||
  noRangeCrit` (the result does not depend on it) **[code]**.
- Removing a target inside the `loop()` iteration skips the next target for that tick **[code]**.
- A target out of `max_range` is removed, not just switched off, so it does not come back when the
  vehicle closes in again **[code]**. Is that intended? The `on` flag in `_SIM_STATE` is then only
  ever 0 in the last post before removal.
- A message with neither `tgt_x`/`tgt_y` nor `tgt_lat`/`tgt_lon` (or `tgt_x` with `tgt_lat`) places
  the target at latitude 0, longitude 0 in the local grid **[code]**.
- `TARGET_RESET` and `TGT_<n>_SIM_NAV_*` have no subscriber in lamss3 **[code]**. Are they still
  needed?
- Members such as `prosecuting`, `deploy_mission`, `prosecute_mode`, `source_control`,
  `dest_id`, `last_time`, the constant `pi` and `driver_library_handle_` are unused **[code]**, left
  from older versions **[guess]**.

## History

- From GobySoft/lamss-shared `src/moos/uSimTargets`; first commit 2011-04-27 (Toby Schneider),
  "working src-shared from revision 3258"; the code carries a 2010 comment by Stephanie Kemna, so
  it predates the repository **[code]**.
- 20 commits before the port, by Henrik Schmidt (15, under several author names) and Toby
  Schneider (5): `tgt_str` (2011), uSimTargets and pTargetSimAIS merged into one GobyMOOSApp
  (2017-10-07), Goby3 (2019), `tgt_utc` defaulting to now (2020), dynamic origin (2021),
  `GEO_COMMAND` support (2021), and `tgt_x`/`tgt_y` or `tgt_lat`/`tgt_lon` as `oneof` (2024).
- lamss3 port (GobySoft/lamss3#23): config proto moved to `package lamss.virtual_ocean`, importing
  `lamss/virtual_ocean/targetsim.proto`; `CTarget` from core's `henrik_util`; the empty `CTarget`
  is value-initialised; build with `lamss_add_app`.
