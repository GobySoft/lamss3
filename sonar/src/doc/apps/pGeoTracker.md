---
title: pGeoTracker
toolbox: sonar
kind: moos-app
source: sonar/src/bin/pGeoTracker/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# pGeoTracker

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
| `tgt_navx_var` (built at run time) | MOOS |  | `GeoTracker.cpp` |  | name built at run time |
| `tgt_navy_var` (built at run time) | MOOS |  | `GeoTracker.cpp` |  | name built at run time |
| `tgt_navutc_var` (built at run time) | MOOS |  | `GeoTracker.cpp` |  | name built at run time |
| `tgt_navhdg_var` (built at run time) | MOOS |  | `GeoTracker.cpp` |  | name built at run time |
| `tgt_navspd_var` (built at run time) | MOOS |  | `GeoTracker.cpp` |  | name built at run time |
| `MISSION_STATE` | MOOS |  | `GeoTracker.cpp`, `OnNewMail` |  |  |
| `NAV_HEADING` | MOOS |  | `GeoTracker.cpp`, `OnNewMail` |  |  |
| `BEARING_STAT` | MOOS |  | `GeoTracker.cpp`, `OnNewMail` |  |  |
| `CLOSE_RANGE` | MOOS |  | `GeoTracker.cpp`, `OnNewMail` |  |  |
| `NAV_SPEED` | MOOS |  | `GeoTracker.cpp`, `OnNewMail` |  |  |
| `PROSECUTE_RANGE` | MOOS |  | `GeoTracker.cpp`, `OnNewMail` |  |  |
| `TRACKER_COMMAND` | MOOS |  | `GeoTracker.cpp`, `OnNewMail` |  |  |
| `VEHICLE_ID` | MOOS |  | `GeoTracker.cpp`, `OnNewMail` |  |  |
| `TARGET_ID` | MOOS |  | `GeoTracker.cpp`, `OnNewMail` |  |  |
| `TRACKING_MODE` | MOOS |  | `GeoTracker.cpp`, `OnNewMail` |  |  |
| `TARGET_CUE` | MOOS |  | `GeoTracker.cpp`, `OnNewMail` |  |  |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `TARGET_X` | MOOS | double | `GeoTracker.cpp` |  |  |
| `TARGET_Y` | MOOS | double | `GeoTracker.cpp` |  |  |
| `TARGET_HEADING` | MOOS | double | `GeoTracker.cpp` |  |  |
| `TARGET_SPEED` | MOOS | double | `GeoTracker.cpp` |  |  |
| `TRACK_STAT` | MOOS |  | `GeoTracker.cpp` |  |  |
| `CLOSE_RANGE` | MOOS | string | `GeoTracker.cpp` |  |  |
| `moosPar` (built at run time) | MOOS | double | `GeoTracker.cpp` |  | name built at run time |
| `MY_TARGET` | MOOS | string | `GeoTracker.cpp` |  |  |
<!-- END GENERATED publishes -->

### Configuration

<!-- For Goby-config apps (GobyMOOSApp, Goby3): generated from the config proto (fields, types,
defaults, (goby.field).description) and the `-e` example. For CMOOSApp: generated from the
GetConfigurationParam / m_MissionReader calls. -->

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `init_meas` |  |  |  |  |
| `range_guess` |  |  |  |  |
| `range_max` |  |  |  |  |
| `range_min` |  |  |  |  |
| `speed_max` |  |  |  |  |
| `min_measurements` |  |  |  |  |
| `Z_length` |  |  |  |  |
| `sampling_theta` |  |  |  |  |
| `bearing_rate_interval` |  |  |  |  |
| `bearing_rate_threshold` |  |  |  |  |
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
