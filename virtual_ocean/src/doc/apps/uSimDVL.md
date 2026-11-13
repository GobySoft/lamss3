---
title: uSimDVL
toolbox: virtual_ocean
kind: moos-app
source: virtual_ocean/src/bin/uSimDVL/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# uSimDVL

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

uSimDVL simulates a Doppler velocity log: it takes the vehicle's body-frame velocity and attitude
from the vehicle simulator (`FRONTSEAT_*`, as uMVS_Bluefin publishes with `OutputPrefix=FRONTSEAT_`
in its example **[code]**), turns it into the velocity a DVL would measure, adds Gaussian noise and a
bias, and publishes it as `HYDROMAN_SIM_DVL_U/V/W` **[code]**. It can model an upward-looking DVL
tracking drifting sea ice (velocity relative to an ice camp given by `NODE_REPORT`), losing lock
outside a range window, and water-track mode **[code]**. It was written to test the HydroMAN
navigation filter in simulation (commit of 2019-07-18) **[code]**; HydroMAN is not in lamss3
**[guess]**.

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `FRONTSEAT_DEPTH` | MOOS | double, m | `SimDVL.cpp` | vehicle depth **[code]** | stored but not used **[code]** |
| `FRONTSEAT_ROLL` | MOOS | double, rad | `SimDVL.cpp` | vehicle roll, converted to degrees **[code]** | uMVS_Bluefin always posts 0 **[code]** |
| `FRONTSEAT_PITCH` | MOOS | double, rad | `SimDVL.cpp` | vehicle pitch, converted to degrees **[code]** |  |
| `FRONTSEAT_HEADING` | MOOS | double, deg | `SimDVL.cpp` | vehicle heading **[code]** |  |
| `NODE_REPORT` | MOOS | string: `NAME=...,SPD=...,HDG=...,...` | `SimDVL.cpp` | ice camp drift: speed and heading of the node named `ice_sheet_name` **[code]** | only with `ice_tracking_mode = 1` **[code]** |
| `FRONTSEAT_BODY_VEL_Y` | MOOS | double, m/s | `SimDVL.cpp` | body velocity, taken as surge u **[code]** | its arrival time drives the publication rate **[code]** |
| `FRONTSEAT_BODY_VEL_X` | MOOS | double, m/s | `SimDVL.cpp` | body velocity, negated to give sway v (starboard positive) **[code]** | code comment: "X seems positive port" |
| `FRONTSEAT_BODY_VEL_Z` | MOOS | double, m/s | `SimDVL.cpp` | body velocity, negated to give heave w (down positive) **[code]** |  |
| `CURRENT_X` | MOOS | double, m/s | `SimDVL.cpp` | water current, east **[code]** | also read by uMVS_Bluefin **[code]** |
| `CURRENT_Y` | MOOS | double, m/s | `SimDVL.cpp` | water current, north **[code]** |  |
| `ACTUAL_ALTITUDE` | MOOS | double, m | `SimDVL.cpp` | range to the tracked surface (bottom or ice) **[code]** | default 10; no lamss3 app publishes it **[code]** |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `HYDROMAN_SIM_DVL_U` | MOOS | double, m/s | `SimDVL.cpp` | simulated DVL velocity, body x (forward) **[code]** | at most once per `dvl_update_frequency` s **[code]** |
| `HYDROMAN_SIM_DVL_V` | MOOS | double, m/s | `SimDVL.cpp` | simulated DVL velocity, body y (starboard) **[code]** | negated when `inverted_dvl = 1` **[code]** |
| `HYDROMAN_SIM_DVL_W` | MOOS | double, m/s | `SimDVL.cpp` | simulated DVL velocity, body z (down) **[code]** | negated when `inverted_dvl = 1` **[code]** |
<!-- END GENERATED publishes -->

### Configuration

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `dvl_update_frequency` | double, s | none (uninitialised) | not checked | minimum time between publications **[code]**; despite the name a period, not a frequency |
| `dvl_std` | double, m/s | none (uninitialised) | not checked | standard deviation of the white noise added to each axis **[code]** |
| `dvl_bias_err` | double, m/s | none (uninitialised) | not checked | constant bias added to each axis **[code]** |
| `ice_tracking_mode` | int (0 or 1) | none (uninitialised) | not checked | 1: subtract the ice camp drift from `NODE_REPORT` **[code]** |
| `ice_sheet_name` | string | empty | with `ice_tracking_mode = 1` | `NAME` of the ice camp's `NODE_REPORT` **[code]** |
| `inverted_dvl` | double (0 or 1) | none (uninitialised) | not checked | 1: negate v and w, for an upward-looking DVL **[code]** |
| `dvl_max_range` | double, m | `200` | no | lock is lost at or above this altitude **[code]** |
| `dvl_min_range` | double, m | `5` | no | lock is lost at or below this altitude **[code]** |
| `simulate_dvl_range` | double (0 or 1) | none (uninitialised) | not checked | 1: switch to water track outside the range window **[code]** |
| `using_umvs_bluefin` | bool | `true` | no | the body velocities come from uMVS_Bluefin, which changes how `CURRENT_X/Y` is applied (see Behaviour) **[code]** |
| `foo` | string |  | no | template placeholder, ignored **[code]** |
| `bar` | string |  | no | template placeholder, ignored **[code]** |
<!-- END GENERATED config -->

Example (from `uSimDVL.moos`):

```
ProcessConfig = uSimDVL
{
  AppTick   = 4
  CommsTick = 4
  dvl_update_frequency = 1
  dvl_std = 0.005
  dvl_bias_err = 0.002
  ice_tracking_mode = 1 // ice tracking mode = 1, bottom-tracking mode = 0
}
```

### Files, devices and external programs

None **[code]**.

## Behaviour

- Each `Iterate` (`AppTick`) **[code]**:
  1. rotates the body velocity (u, v, w) to the earth frame with roll, pitch and heading;
  2. computes the ice camp's earth-frame drift from its speed and heading (zero unless
     `ice_tracking_mode = 1` and a matching `NODE_REPORT` has arrived);
  3. forms the velocity relative to the tracked surface, adding the water current or not (table
     below);
  4. rotates back to the body frame and adds noise (`dvl_std`) and bias (`dvl_bias_err`).
- The result is published only when a `FRONTSEAT_BODY_VEL_Y` has arrived later than the previous
  publication time plus `dvl_update_frequency` **[code]**. Without body velocity input nothing is
  published **[code]**.
- Water current (`CURRENT_X`/`CURRENT_Y`) is added as follows **[code]**:

| `simulate_dvl_range` | Altitude within (min, max) | `using_umvs_bluefin` true | `using_umvs_bluefin` false |
|---|---|---|---|
| 1 | yes (bottom or ice lock) | added | not added |
| 1 | no (water track) | not added | added |
| 0 | ignored (always lock) | added | not added |

  uMVS_Bluefin's body velocity is relative to the water **[guess]**, so adding the current gives
  velocity over ground in lock and leaving it out gives velocity through the water in water track
  **[guess]**.
- Noise is drawn from a global `std::default_random_engine` with its default seed, so every run
  produces the same noise sequence **[code]**.

## Running it

- No mission component launches it yet.
- Standalone, in `virtual_ocean/src/bin/uSimDVL/`: run `MOOSDB uSimDVL.moos`, then
  `uSimDVL uSimDVL.moos`, and post `FRONTSEAT_BODY_VEL_X/Y/Z`, `FRONTSEAT_ROLL/PITCH/HEADING`
  (e.g. from uMVS_Bluefin). The example lacks `inverted_dvl` and `simulate_dvl_range`, which have
  no default (see open questions).
- `lamss_virtual_ocean_moos_smoke` only checks that it connects to the MOOSDB **[test]**.
- `uSimDVL -e`, `-i` and `-h` print MOOS-IvP template text that does not describe this app
  (`SimDVL_Info.cpp`) **[code]**.

## Known issues and open questions

- `dvl_update_frequency`, `dvl_std`, `dvl_bias_err`, `ice_tracking_mode`, `inverted_dvl` and
  `simulate_dvl_range` have no default; when left out of the configuration they are read
  uninitialised (undefined behaviour; a negative `dvl_std` is invalid for
  `std::normal_distribution`) **[code]**. The example `uSimDVL.moos` omits two of them. What
  defaults are intended?
- The body velocities, attitude and water current are also uninitialised until their first mail
  **[code]**.
- `MOOSTrace` prints the double members `inverted_dvl`, `dvl_max_range`, `dvl_min_range` and
  `simulate_dvl_range` with `%i` (undefined behaviour in a varargs call) **[code]**.
- The bias is the same on all three axes and is not negated with `inverted_dvl` **[code]**. Is
  that intended?
- `FRONTSEAT_DEPTH`, `m_previous_*`, `m_icecamp_u/v/w` are stored but never used **[code]**.
- The trace line "Inverted DVL measurement" is printed for every publication, inverted or not
  **[code]**.
- The output names say `HYDROMAN_SIM_`; HydroMAN is not ported. Which lamss3 app should consume
  them (uSimPhinsINS does not)? Should they be renamed?
- The noise seed is fixed (see Behaviour). Is repeatability wanted?

## History

- From GobySoft/lamss-shared `src/moos/HydroMANSimUtilities/uSimDVL`; first commit 2019-07-18
  (Supun Randeni), "Adding DVL and LBL simulators to test hydroman in simulations".
- 14 commits before the port, 12 by Supun Randeni (ice tracking and inverted DVL 2020, range
  limits 2020-11-18, water track for uSimUnderseaVehicle 2022-12-06) and 2 by Toby Schneider
  (`FRONTSEAT_*` inputs for frontseat simulation, 2019).
- lamss3 port (GobySoft/lamss3#23): licence headers, build with `lamss_add_app` (no Goby), new
  `uSimDVL.moos` example; no change to behaviour.
