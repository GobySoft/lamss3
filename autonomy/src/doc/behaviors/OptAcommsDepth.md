---
title: OptAcommsDepth
toolbox: autonomy
kind: ivp-behavior
source: autonomy/src/lib/behaviors/depth/OptAcommsDepth/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# OptAcommsDepth

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
| `BELLHOP_RESPONSE` | MOOS (helm) | string | `BHV_OptAcommsDepth.cpp` |  |  |
| `AMBIENT_NL_DEPTH` | MOOS (helm) | string | `BHV_OptAcommsDepth.cpp` |  |  |
| `VLA_ARRAY_GAIN` | MOOS (helm) | string | `BHV_OptAcommsDepth.cpp` |  |  |
| `NAV_DEPTH` | MOOS (helm) | double | `BHV_OptAcommsDepth.cpp` |  |  |
| `NAV_SPEED` | MOOS (helm) | double | `BHV_OptAcommsDepth.cpp` |  |  |
| `LOGGER_DIRECTORY` | MOOS (helm) | string | `BHV_OptAcommsDepth.cpp` |  |  |
| `SVP_RESPONSE` | MOOS (helm) | string | `BHV_OptAcommsDepth.cpp` |  |  |
| `LAT_ORIGIN` | MOOS (helm) | double | `BHV_OptAcommsDepth.cpp` |  |  |
| `LONG_ORIGIN` | MOOS (helm) | double | `BHV_OptAcommsDepth.cpp` |  |  |
| `NAV_X` | MOOS (helm) | double | `BHV_OptAcommsDepth.cpp` |  |  |
| `NAV_Y` | MOOS (helm) | double | `BHV_OptAcommsDepth.cpp` |  |  |
| `NAV_HEADING` | MOOS (helm) |  | `BHV_OptAcommsDepth.cpp` |  |  |
| `_RANGE` | MOOS (helm) |  | `BHV_OptAcommsDepth.cpp` |  |  |
| `_RATE` | MOOS (helm) |  | `BHV_OptAcommsDepth.cpp` |  |  |
| `_ARR_FILE` | MOOS (helm) |  | `BHV_OptAcommsDepth.cpp` |  |  |
| `std::string(boost::to_upper_copy(collaborator_) + "_RANGE")` (built at run time) | MOOS (helm) | double | `BHV_OptAcommsDepth.cpp` |  | name built at run time |
| `std::string(boost::to_upper_copy(collaborator_) + "_RATE")` (built at run time) | MOOS (helm) | double | `BHV_OptAcommsDepth.cpp` |  | name built at run time |
| `boost::to_upper_copy(collaborator_) + "_NAV_X"` (built at run time) | MOOS (helm) | double | `BHV_OptAcommsDepth.cpp` |  | name built at run time |
| `boost::to_upper_copy(collaborator_) + "_NAV_Y"` (built at run time) | MOOS (helm) | double | `BHV_OptAcommsDepth.cpp` |  | name built at run time |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `BELLHOP_REQUEST` | MOOS (helm) | string | `BHV_OptAcommsDepth.cpp` |  |  |
| `BHV_ACOUST_DEPTH_UNFILT` | MOOS (helm) | string | `BHV_OptAcommsDepth.cpp` |  |  |
| `BHV_SNR_DEPTH_FUNC` | MOOS (helm) | string | `BHV_OptAcommsDepth.cpp` |  |  |
| `BHV_NL_DEPTH_FUNC` | MOOS (helm) | string | `BHV_OptAcommsDepth.cpp` |  |  |
| `BHV_SE_DEPTH_FUNC` | MOOS (helm) | string | `BHV_OptAcommsDepth.cpp` |  |  |
| `BHV_TL_DEPTH_FUNC` | MOOS (helm) | string | `BHV_OptAcommsDepth.cpp` |  |  |
| `SVP_REQUEST` | MOOS (helm) | string | `BHV_OptAcommsDepth.cpp` |  |  |
<!-- END GENERATED publishes -->

### Configuration

<!-- For Goby-config apps (GobyMOOSApp, Goby3): generated from the config proto (fields, types,
defaults, (goby.field).description) and the `-e` example. For CMOOSApp: generated from the
GetConfigurationParam / m_MissionReader calls. -->

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `look_ahead_seconds` |  |  |  |  |
| `calculate_interval` |  |  |  |  |
| `collaborator` |  |  |  |  |
| `contact` |  |  |  |  |
| `max_pitch` |  |  |  |  |
| `sonar` |  |  |  |  |
| `depth_min` |  |  |  |  |
| `depth_max` |  |  |  |  |
| `number_depths` |  |  |  |  |
| `theta_min` |  |  |  |  |
| `theta_max` |  |  |  |  |
| `number_theta` |  |  |  |  |
| `adapt_tl_threshold` |  |  |  |  |
| `multipath_penalty` |  |  |  |  |
| `bounce_penalty` |  |  |  |  |
| `max_number_bounces` |  |  |  |  |
| `depth_envelope` |  |  |  |  |
| `depth_uncertainty` |  |  |  |  |
| `activation_range` |  |  |  |  |
| `departure_range` |  |  |  |  |
| `approach_range` |  |  |  |  |
| `range_max` |  |  |  |  |
| `peak_filter` |  |  |  |  |
| `optimize` |  |  |  |  |
| `water_depth` |  |  |  |  |
| `surface_sound_speed` |  |  |  |  |
| `sound_speed_gradient` |  |  |  |  |
| `beam_width` |  |  |  |  |
| `minutil` |  |  |  |  |
| `maxutil` |  |  |  |  |
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
