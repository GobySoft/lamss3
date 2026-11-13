---
title: uSimPassiveSonar
toolbox: virtual_ocean
kind: goby-moos-app
source: virtual_ocean/src/bin/uSimPassiveSonar/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# uSimPassiveSonar

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
| `SONAR_CONTROL` | MOOS |  | `handle_sonar_control` |  |  |
| `AEL_HEADING` | MOOS |  | `handle_ael_heading` |  |  |
| `AEL_PITCH` | MOOS |  | `handle_ael_pitch` |  |  |
| `NAV_SPEED` | MOOS |  | `handle_nav_speed` |  |  |
| `ARRAY_X` | MOOS |  | `handle_array_x` |  |  |
| `ARRAY_Y` | MOOS |  | `handle_array_y` |  |  |
| `ARRAY_Z` | MOOS |  | `handle_array_z` |  |  |
| `ARRAY_DIR_X` | MOOS |  | `handle_array_dir_x` |  |  |
| `ARRAY_DIR_Y` | MOOS |  | `handle_array_dir_y` |  |  |
| `ARRAY_DIR_Z` | MOOS |  | `handle_array_dir_z` |  |  |
| `ARRAY_HEADING` | MOOS |  | `handle_array_heading` |  |  |
| `ARRAY_PITCH` | MOOS |  | `handle_array_pitch` |  |  |
| `TARGET_SIM_LIST` | MOOS |  | `handle_target_sim_list` |  |  |
| `AMBIENT_NL_DEPTH` | MOOS |  | `handle_ambient_nl_depth` |  |  |
| `NOISE_VERT_DIR` | MOOS |  | `handle_noise_vert_dir` |  |  |
| `COVARIANCE_FILE` | MOOS |  | `handle_covariance_file` |  |  |
| `SVP_RESPONSE` | MOOS |  | `handle_svp_response` |  |  |
| `VEHICLE_NAME` | MOOS |  | `handle_vehicle_name` |  |  |
| `SOUND_SPEED` | MOOS |  | `handle_sound_speed` |  |  |
| `SOUND_SPEED_GRAM` | MOOS |  | `handle_sound_speed` |  |  |
| `LAMSS_PINGER_COMMAND_IN` | MOOS |  | `handle_pinger_command` |  |  |
| `var` (built at run time) | MOOS |  | `handle_target_sim_state`, `handle_target_arrival_file`, `handle_pinger_arrival_file` |  | name built at run time |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `TARGET_XPOS` | MOOS |  | `SimPassiveSonar.cpp` |  |  |
| `TARGET_YPOS` | MOOS |  | `SimPassiveSonar.cpp` |  |  |
| `NODE_REPORT` | MOOS | string | `SimPassiveSonar.cpp` |  |  |
| `SONAR_DIR` | MOOS |  | `SimPassiveSonar.cpp` |  |  |
| `SONAR_FILE_NUM` | MOOS | double | `SimPassiveSonar.cpp` |  |  |
| `SVP_REQUEST` | MOOS |  | `SimPassiveSonar.cpp` |  |  |
| `COVARIANCE_REQUEST` | MOOS | string | `SimPassiveSonar.cpp` |  |  |
| `SONAR_FILENAME` | MOOS |  | `SimPassiveSonar.cpp` |  |  |
| `BELLHOP_REQUEST` | MOOS | string | `SimPassiveSonar.cpp` |  |  |
<!-- END GENERATED publishes -->

### Configuration

<!-- For Goby-config apps (GobyMOOSApp, Goby3): generated from the config proto (fields, types,
defaults, (goby.field).description) and the `-e` example. For CMOOSApp: generated from the
GetConfigurationParam / m_MissionReader calls. -->

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `common` | GobyMOOSAppConfig |  | no | Configuration common to all Goby MOOS applications |
| `ray_acoustics` | bool | `false` | no |  |
| `rap_acoustics` | bool | `false` | no |  |
| `svp_surface` | double | `1430` | no |  |
| `svp_gradient` | double | `0.016` | no |  |
| `max_rap_depth` | double | `5000` | no |  |
| `water_depth` | double | `5000` | no |  |
| `bottom_loss` | double | `0.5` | no |  |
| `noise_level` | double | `50` | no |  |
| `array_type` | ArrayTypeEnum | `ARRAY_TYPE_HYD` | no |  |
| `max_number_bounces` | int32 | `1` | no |  |
| `num_array_elements` | int32 |  | yes |  |
| `sampling_frequency` | double |  | yes |  |
| `num_samples` | int32 |  | yes |  |
| `hydrophone_gain` | double | `0` | no |  |
| `accelerometer_gain` | double | `0` | no |  |
| `base_dir_name` | string |  | yes |  |
| `acoustic_filename_prefix` | string | `HYD` | no |  |
| `acoustic_filename_suffix` | string | `.DAT` | no |  |
| `nonacoustic_filename_prefix` | string | `NAS` | no |  |
| `nonacoustic_filename_suffix` | string | `.DAT` | no |  |
| `svp_updates` | bool | `false` | no |  |
| `svp_update_interval` | double | `60` | no |  |
| `svp_range_resolution` | double | `200` | no |  |
| `svp_depth_resolution` | double | `20` | no |  |
| `sync_path` | string | `` | no |  |
| `sync_password` | string |  | no |  |
| `surface_rms_roughness` | double | `0` | no |  |
| `surface_roughness_loss` | double | `0` | no |  |
| `bottom_rms_roughness` | double | `0` | no |  |
| `bottom_roughness_loss` | double | `0` | no |  |
| `replica_path` | string | `./` | no |  |
| `coherent_sources` | bool | `false` | no |  |
| `source_weight_file` | string | `source_weights.bin` | no |  |
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
