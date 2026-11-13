---
title: uMVS_Bluefin
toolbox: autonomy
kind: moos-app
source: autonomy/src/bin/uMVS_Bluefin/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# uMVS_Bluefin

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
| `pAUV->m_sInputPrefix + "DESIRED_ELEVATOR"` (built at run time) | MOOS |  | `MVSim.cpp` |  | name built at run time |
| `pAUV->m_sInputPrefix + "DESIRED_RUDDER"` (built at run time) | MOOS |  | `MVSim.cpp` |  | name built at run time |
| `pAUV->m_sInputPrefix + "DESIRED_THRUST"` (built at run time) | MOOS |  | `MVSim.cpp` |  | name built at run time |
| `pAUV->m_sInputPrefix + "CABLE_TENSION"` (built at run time) | MOOS |  | `MVSim.cpp` |  | name built at run time |
| `pAUV->m_sInputPrefix + "ARRAY_SHAPE_X"` (built at run time) | MOOS |  | `MVSim.cpp` |  | name built at run time |
| `pAUV->m_sInputPrefix + "ARRAY_SHAPE_Y"` (built at run time) | MOOS |  | `MVSim.cpp` |  | name built at run time |
| `pAUV->m_sInputPrefix + "ARRAY_SHAPE_Z"` (built at run time) | MOOS |  | `MVSim.cpp` |  | name built at run time |
| `pAUV->m_sInputPrefix + "TRIM_KG"` (built at run time) | MOOS |  | `MVSim.cpp` |  | name built at run time |
| `LAT_ORIGIN` | MOOS |  | `MVSim.cpp`, `OnNewMail` |  |  |
| `LONG_ORIGIN` | MOOS |  | `MVSim.cpp`, `OnNewMail` |  |  |
| `TRIM_CONTROL` | MOOS |  | `MVSim.cpp` |  |  |
| `BUOYANCY_CONTROL` | MOOS |  | `MVSim.cpp` |  |  |
| `BATHY_Z` | MOOS |  | `MVSim.cpp` |  |  |
| `SIM_RESET` | MOOS |  | `MVSim.cpp` |  |  |
| `GPS_UPDATE_RECEIVED` | MOOS |  | `MVSim.cpp`, `OnNewMail` |  |  |
| `GPS_X` | MOOS |  | `MVSim.cpp`, `OnNewMail` |  |  |
| `GPS_Y` | MOOS |  | `MVSim.cpp`, `OnNewMail` |  |  |
| `CURRENT_X` | MOOS |  | `MVSim.cpp`, `OnNewMail` |  |  |
| `CURRENT_Y` | MOOS |  | `MVSim.cpp`, `OnNewMail` |  |  |
| `CURRENT_Z` | MOOS |  | `MVSim.cpp`, `OnNewMail` |  |  |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `SIMULATION_MODE` | MOOS | string | `MVSim.cpp` |  |  |
| `BUOYANCY_REPORT` | MOOS | string | `MVSim.cpp` |  |  |
| `TRIM_REPORT` | MOOS | string | `MVSim.cpp` |  |  |
| `LAT_ORIGIN` | MOOS |  | `MVSim.cpp` |  |  |
| `LONG_ORIGIN` | MOOS |  | `MVSim.cpp` |  |  |
| `TRUE_X` | MOOS | double | `MVSim.cpp` |  |  |
| `TRUE_Y` | MOOS | double | `MVSim.cpp` |  |  |
| `TRUE_Z` | MOOS | double | `MVSim.cpp` |  |  |
| `sP + "X"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "Y"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "LATITUDE"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "LONGITUDE"` (built at run time) | MOOS |  | `MVSim.cpp` |  | name built at run time |
| `sP + "Z"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "TOW_POS_X"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "TOW_POS_Y"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "TOW_POS_Z"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "YAW"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "YAW_RATE"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "DEPTH"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "SPEED"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "HEADING"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "BODY_VEL_Y"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "BODY_VEL_X"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "BODY_VEL_Z"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "PITCH"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "PITCH_RATE"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "ROLL"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "RUDDER"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "ELEVATOR"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "RPM"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "TOW_VEL_X"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "TOW_VEL_Y"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `sP + "TOW_VEL_Z"` (built at run time) | MOOS | double | `MVSim.cpp` |  | name built at run time |
| `TIDE_HEIGHT` | MOOS |  | `MVSim.cpp` |  |  |
<!-- END GENERATED publishes -->

### Configuration

<!-- For Goby-config apps (GobyMOOSApp, Goby3): generated from the config proto (fields, types,
defaults, (goby.field).description) and the `-e` example. For CMOOSApp: generated from the
GetConfigurationParam / m_MissionReader calls. -->

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `LatOrigin` |  |  |  | global (mission file) |
| `LongOrigin` |  |  |  | global (mission file) |
| `TerrainFile` |  |  |  | global (mission file) |
| `TideHeight` |  |  |  |  |
| `MagneticOffset` |  |  |  | global (mission file) |
| `dvl_lock` |  |  |  |  |
| `trim_tolerance` |  |  |  |  |
| `max_trim_delay` |  |  |  |  |
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
