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

pGeoTracker does single-vehicle bearings-only target motion analysis **[code]**: from a series of
true bearings taken at different vehicle positions it estimates the target's position, heading and
speed, assuming the target moves at constant velocity **[code]**. It is fed by
[pLamssTrackManager](pLamssTrackManager.md), whose `BEARING_STAT` carries the selected bearing and
the array position, and which also drives it with `TRACKER_COMMAND` **[code]**. Its `TRACK_STAT`
goes back to pLamssTrackManager for the track quality test, and its `CLOSE_RANGE` tells
pLamssTrackManager that enough bearings have been taken **[code]**. It sits after the passive
sonar chain ([pLamssAEL](pLamssAEL.md) → [pLamssCBF](pLamssCBF.md) →
[pLamssBearingTracker](pLamssBearingTracker.md) → pLamssTrackManager) **[code]**. Its
`<TARGET_ID>_NAV_*` and `TARGET_*` outputs are for track-and-trail behaviors (code comment) **[code]**.

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `tgt_navx_var` (built at run time) | MOOS | double | `GeoTracker.cpp` | `<TARGET_ID>_NAV_X`: target x **[code]** | registered when `TARGET_ID` arrives **[code]**; also published by this app, so its own solution comes back as the next initial guess **[code]** |
| `tgt_navy_var` (built at run time) | MOOS | double | `GeoTracker.cpp` | `<TARGET_ID>_NAV_Y`: target y **[code]** | as above |
| `tgt_navutc_var` (built at run time) | MOOS | double | `GeoTracker.cpp` | `<TARGET_ID>_NAV_UTC`: time of that position **[code]** | as above |
| `tgt_navhdg_var` (built at run time) | MOOS | double | `GeoTracker.cpp` | `<TARGET_ID>_NAV_HEADING`: target heading, degrees **[code]** | as above |
| `tgt_navspd_var` (built at run time) | MOOS | double | `GeoTracker.cpp` | `<TARGET_ID>_NAV_SPEED`: target speed, m/s **[code]** | as above; capped at `speed_max` for the guess **[code]** |
| `MISSION_STATE` | MOOS | string | `GeoTracker.cpp`, `OnNewMail` | mission state **[code]** | sets `prosecute`, which is not used **[code]** |
| `NAV_HEADING` | MOOS | double, degrees | `GeoTracker.cpp`, `OnNewMail` | vehicle heading, for the turn-rate filter **[code]** | from pLamssMissionManager or pGateway **[code]** |
| `BEARING_STAT` | MOOS | string: `node=…,state=…,bearing=…,sigma=…,beamno=…,xp=…,yp=…,time=…` | `GeoTracker.cpp`, `OnNewMail` | bearing measurement (compass degrees) and array position **[code]** | from pLamssTrackManager **[code]**; `state=2` is used, anything else resets **[code]** |
| `CLOSE_RANGE` | MOOS | string: `TRUE` / other | `GeoTracker.cpp`, `OnNewMail` | close-range flag **[code]** | its own output; overwritten on the next bearing **[code]** |
| `NAV_SPEED` | MOOS | double, m/s | `GeoTracker.cpp`, `OnNewMail` | vehicle speed, for the measurement interval **[code]** |  |
| `PROSECUTE_RANGE` | MOOS | double, m | `GeoTracker.cpp`, `OnNewMail` | expected target range, for the measurement interval **[code]** | default 800 m **[code]**; no publisher in lamss3 **[code]** |
| `TRACKER_COMMAND` | MOOS | string: `RESET`, `ON`, other | `GeoTracker.cpp`, `OnNewMail` | reset, enable or disable tracking **[code]** | from pLamssTrackManager **[code]** |
| `VEHICLE_ID` | MOOS | double | `GeoTracker.cpp`, `OnNewMail` | own id, written into `TRACK_STAT` **[code]** |  |
| `TARGET_ID` | MOOS | string | `GeoTracker.cpp`, `OnNewMail` | name of the target, upper-cased for the `_NAV_*` variables **[code]** | without it no `_NAV_*` is published **[code]** |
| `TRACKING_MODE` | MOOS | string: `SINGLE` / `MULTIPLE` | `GeoTracker.cpp`, `OnNewMail` | in `MULTIPLE` no solution is published **[code]** |  |
| `TARGET_CUE` | MOOS | string: target status (`parse_tgt_stat`) | `GeoTracker.cpp`, `OnNewMail` | cued target position, heading and speed **[code]** | from pLamssMissionManager **[code]**; used for an early estimate **[code]** |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `TARGET_X` | MOOS | double | `GeoTracker.cpp` | solved target x, m **[code]** | after each accepted solution **[code]** |
| `TARGET_Y` | MOOS | double | `GeoTracker.cpp` | solved target y, m **[code]** |  |
| `TARGET_HEADING` | MOOS | double | `GeoTracker.cpp` | solved target heading, degrees **[code]** |  |
| `TARGET_SPEED` | MOOS | double | `GeoTracker.cpp` | solved target speed, m/s **[code]** |  |
| `TRACK_STAT` | MOOS | string: `node=…,state=2,x=…,y=…,heading=…,speed=…,time=…` | `GeoTracker.cpp` | the solution **[code]** | read by pLamssTrackManager **[code]**; a `state=1` form exists but is never reached, see open questions |
| `CLOSE_RANGE` | MOOS | string | `GeoTracker.cpp` | `TRUE` once `init_meas` bearings have been accepted, else `FALSE` **[code]** | `FALSE` on connect, seen in the smoke test **[test]**; read by pLamssTrackManager **[code]** |
| `moosPar` (built at run time) | MOOS | double | `GeoTracker.cpp` | five variables `<TARGET_ID>_NAV_X`, `_NAV_Y`, `_NAV_HEADING`, `_NAV_SPEED`, `_NAV_UTC` **[code]** | the solution, or the cue-based estimate before one exists **[code]** |
| `MY_TARGET` | MOOS | string | `GeoTracker.cpp` | `contact=<TARGET_ID>` **[code]** | with every `_NAV_*` post **[code]** |
<!-- END GENERATED publishes -->

### Configuration

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `init_meas` | int (read as double) |  | yes | accepted bearings before `CLOSE_RANGE=TRUE` and before bearings are used for tracking **[code]** |
| `range_guess` | double, m |  | yes | initial range along the bearing, and the fallback when the guess is outside `range_min`–`range_max` **[code]** |
| `range_max` | double, m | `5000` | no | solutions farther than this are rejected **[code]** |
| `range_min` | double, m | `1000` | no | solutions nearer than this are rejected **[code]** |
| `speed_max` | double, m/s | `20` | no | solutions faster than this are rejected **[code]** |
| `min_measurements` | int (read as double) |  | yes | bearings in the contact before a solution is attempted **[code]** |
| `Z_length` | int (read as double) |  | yes | bearings kept for the fit (window) **[code]**; at most 300 **[code]** |
| `sampling_theta` | double, degrees |  | yes | expected bearing change between used bearings; sets the measurement interval **[code]** |
| `bearing_rate_interval` | double, s | `300` | no | window of the bearing-rate regression **[code]** |
| `bearing_rate_threshold` | double, degrees/min | `3` | no | bearings are ignored when the bearing rate is below this (after the first half window) **[code]** |
<!-- END GENERATED config -->

`OnStartUp()` fails if a required parameter is missing **[code]**.

Example (from `pGeoTracker.moos`):

```
ProcessConfig = pGeoTracker
{
  AppTick   = 2
  CommsTick = 4

  range_max = 6000
  speed_max = 10.0
  range_guess = 3000
  init_meas = 8
  min_measurements = 30
  Z_length = 40
  sampling_theta = 0.6
}
```

### Files, devices and external programs

None **[code]**.

## Behaviour

- **Accepting a bearing** (`BEARING_STAT`, `state=2`, while tracking is on) **[code]**:
    - bearings are spaced in time by `tan(sampling_theta)·PROSECUTE_RANGE / NAV_SPEED` seconds;
    - a bearing is ignored when the absolute bearing rate (linear regression over
      `bearing_rate_interval`) is below `bearing_rate_threshold`, or when the vehicle's turn rate
      (moving average of 10 `NAV_HEADING` samples) exceeds 10°/min.
- Any other `BEARING_STAT` state resets the contact and posts `CLOSE_RANGE=FALSE` **[code]**.
- The first `init_meas` accepted bearings only count; after that each one is added to the contact
  (the first is skipped) **[code]**. Each new bearing also gives a quick estimate where it crosses
  the cued target's track (`TARGET_CUE`), or `range_guess` along the bearing if they disagree, and
  this estimate is posted as `<TARGET_ID>_NAV_*` while the contact has fewer than
  `min_measurements` bearings, or when cue and bearing disagree, and no accepted solution exists **[code]**.
- **Solving** (once the contact has `min_measurements` bearings): starting from the last solution
  (or the `_NAV_*` guess, or `range_guess` along the last bearing), a BFGS quasi-Newton
  minimisation of the summed squared bearing residuals over the last `Z_length` bearings gives
  position and velocity **[code]**. A solution inside `range_min`–`range_max` and below `speed_max`
  is published in `Iterate()`; otherwise the optimiser is reset **[code]**. Accepted solutions are
  also smoothed by a straight-line fit of their history, used as the next guess **[code]**.
- `TRACKER_COMMAND=RESET` clears everything; `ON`/other switch tracking on/off **[code]**.
- `Iterate()` runs at `AppTick` and publishes only when a new solution is ready **[code]**.

## Running it

- Not launched by any mission component yet **[code]**.
- Standalone: `MOOSDB` and `pGeoTracker pGeoTracker.moos` in `sonar/src/bin/pGeoTracker/`; it needs
  `TRACKER_COMMAND=ON`, `NAV_SPEED`, `NAV_HEADING` and a stream of `BEARING_STAT` from a moving
  vehicle.
- Tested by `lamss_sonar_moos_smoke`: started from its example, checking that it connects and posts
  `CLOSE_RANGE=FALSE` **[test]**. No test exercises the solver.

## Known issues and open questions

- `tracking` is never initialised: until the first `TRACKER_COMMAND`, whether bearings are used is
  undefined. pLamssTrackManager sends `TRACKER_COMMAND=OFF` at start-up, which hides this in a
  full mission.
- `last_heading_time` is set once in the constructor and never updated, so the turn rate is the
  last heading change divided by the time since start-up, and the turn-rate gate fades out.
- `CGeoTrack::set_measurement()` keeps `Z_length + 1` bearings once the window is full, uses only
  the first `Z_length` (leaving out the newest), and its shift duplicates one bearing and overwrites
  another.
- The bearing residual in `track_func()` is not wrapped, so a target near west (±180° in the
  mathematical angle) gives residuals of about 2π.
- `auv_speed` and the cue values are read before they are set if `NAV_SPEED` or `TARGET_CUE` never
  arrive; `NAV_SPEED=0` makes the interval infinite.
- `parse_measurement()` always returns true, although the code says it returns false for another
  node's bearing; bearings from any node are used.
- The `state=1` (`AMBIGUOUS_BEARING`) `TRACK_STAT` is unreachable: `tgt_msg_ready` is set only with
  a full solution.
- `matrixinverse()` in [track](../libraries/track.md) calls `exit(1)` on a singular matrix (it is
  not called by this app).
- `MISSION_STATE` (`prosecute`) and `PROSECUTE_RANGE` (no publisher in lamss3) look like leftovers.
  Who should set `PROSECUTE_RANGE`?
- Magic numbers: 800 m default range, 10°/min turn rate, filter length 10, 300-bearing buffer,
  `search_eps` 1e-9.

## History

- From GobySoft/lamss-shared `src/moos/pGeoTracker`; first commit 2011-10-24 (Henrik Schmidt,
  "pGeoTracker added"); 27 commits before the port, by Henrik Schmidt, Toby Schneider and one other
  contributor, the last on 2020-08-31 ("Enable tracking in deploy modes").
- lamss3 port (GobySoft/lamss3#21–#23): clang-format; built as a plain CMOOSApp (`NO_GOBY`) against
  MOOS 10, IvP's mbutil, [track](../libraries/track.md) and core's henrik_util (now
  `lamss::core`), without the unused IvP geometry library; warning fixes with no change of
  behaviour; MIT copyright and SPDX headers added; example `.moos` and smoke test added.
