---
title: MaintainBearing
toolbox: autonomy
kind: ivp-behavior
source: autonomy/src/lib/behaviors/tracking/MaintainBearing/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# MaintainBearing

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

`BHV_MaintainBearing` is an IvP behavior that steers so that a tracked acoustic target stays at
`desired_angle` off the bow, to port or starboard **[code]**. With `alternate_interval` set, it
switches side when the target's bearing rate is too low, and otherwise keeps both sides open to
the helm **[code]**. It is for passive towed-array tracking: a bearing broadside to the array is
best resolved, and a bearing rate is needed to estimate range **[guess]**. Its input,
`BEARING_STAT`, is posted by pLamssTrackManager (sonar toolbox) **[code]**, and pLamssMissionManager
posts `ARRAY_TRACK_ANGLE` = `bearing_mode=…#desired_angle=…`, which reads like this behavior's
`updates` variable **[guess]**.

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `heading_variable` (built at run time) | MOOS (helm) | double | `BHV_MaintainBearing.cpp` | heading of the array or vehicle, degrees **[code]** | named by the `heading_variable` parameter |
| `bearing_variable` (built at run time) | MOOS (helm) | string: `key=value,...` (`state`, `bearing`, …) | `BHV_MaintainBearing.cpp` | target bearing track; only `state` and `bearing` are read **[code]** | named by the `bearing_variable` parameter; acts only when `state=2` (absolute bearing, `CTrack.cpp`) **[code]** |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `BEARINGS` | MOOS (helm) | double | `BHV_MaintainBearing.cpp` | number of bearings in the rate window **[code]** | every iteration with `state=2` |
| `BEARING_RATE` | MOOS (helm) | double | `BHV_MaintainBearing.cpp` | bearing rate by least squares over the window, degrees/minute **[code]** |  |
| `BEARING_RATE_RMS` | MOOS (helm) | double | `BHV_MaintainBearing.cpp` | RMS error of that fit, scaled to degrees/minute **[code]** |  |
| `PENDING_HEADING_SWITCH` | MOOS (helm) | double, s | `BHV_MaintainBearing.cpp` | time left before a side switch is allowed **[code]** | meaningful only with `alternate_interval` **[guess]** |
<!-- END GENERATED publishes -->

The objective function goes to the helm over `course` **[code]**.

### Configuration

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `width` | int, degrees | none | yes (no default) | course ZAIC peak width; base width is 180 − `width` **[code]** |
| `peak_width` | int | none | no | stored but not used **[code]** |
| `over_shoot` | double | `0` | no | aim (`over_shoot` × error) beyond `desired_angle` **[code]** |
| `desired_angle` | double, degrees | `90` | no | angle between bow and target bearing to hold **[code]** |
| `heading_variable` | string | `AEL_HEADING` | no | MOOS variable with the heading **[code]** |
| `bearing_variable` | string | `BEARING_STAT` | no | MOOS variable with the bearing track **[code]** |
| `bearing_type` | string | `relative` | no | `absolute` sets a flag that is not used **[code]** |
| `bearing_mode` | `single` / `dual` / `quad` | `dual` | no | sides offered to the helm; any other value means `dual` **[code]** |
| `alternate_interval` | double, s | none | yes with `bearing_mode` other than `single` in practice **[guess]** | > 0 enables side switching at most this often; also the rate window (half of it) **[code]** |
| `bearing_rate_threshold` | double, degrees/minute | `3` | no | bearing rate below which a side switch is wanted **[code]** |
<!-- END GENERATED config -->

`setParam` reads the values with `atof` and does not check them **[code]**. The standard
IvPBehavior parameters (`name`, `pwt`, `condition`, `updates`, `endflag` and the others in
`setParamCommon`) also apply **[code]**. The constructor sets `unifbox = course=3` and
`gridbox = course=9` **[code]**. The helm must have a `course` domain **[code]**.

Example (from `autonomy/src/test/moos_smoke/smoke.bhv`):

```
Behavior = BHV_MaintainBearing
{
  name = maintain_bearing
  width = 60
  heading_variable = NAV_HEADING
  bearing_variable = BEARING_STAT
  desired_angle = 45
  bearing_mode = dual
  alternate_interval = 450
  bearing_rate_threshold = 3.0
  condition = MODE == SMOKE
}
```

### Files, devices and external programs

None **[code]**.

## Behaviour

- No function unless both input variables are present and the bearing state is 2 **[code]**.
- Each iteration with `state=2` **[code]**:
    - the relative bearing (bearing − heading, −180 to 180) is computed;
    - the absolute bearing is unwrapped and appended to a time series, which is pruned to the last
      `alternate_interval` / 2 seconds;
    - the bearing rate and its RMS error come from a least-squares line (`CmyVectorUtils`) and are
      posted in degrees/minute.
- On the first iteration after idle, the side is the target's current side (starboard if the
  relative bearing ≥ 0) **[code]**.
- Side switch (only with `alternate_interval` > 0) **[code]**: when the rate estimate is good (RMS
  < `bearing_rate_threshold`) and either the rate is below the threshold and `alternate_interval`
  has passed, or the rate is below half the threshold and half the interval has passed. The rate
  history is then cleared.
- Course function: a `ZAIC_PEAK` on `course` (summit delta 50, utility 0–100, wrapping) with
  peaks at **[code]**:
    - `single`: bearing − angle (target to starboard), or bearing + angle when switching has put
      the target to port;
    - `dual`: both bearing ± angle, unless switching is enabled, then only the current side;
    - `quad`: as `dual`, plus the two rear headings bearing ± (180 − angle) with base width
      90 − `width`, unless `desired_angle` is 90 or 270.
    The angle is `desired_angle` + `over_shoot` × (`desired_angle` − |relative bearing|).
- The function is weighted by `pwt` **[code]**. `getRelevance()` always returns 1 **[code]**.
- `onIdleState` clears the rate history and the side **[code]**.

## Running it

- No mission component launches it yet. It runs inside pHelmIvP **[code]**.
- It is built into `liblamss_autonomy_behaviors.so` (`autonomy/src/lib/behaviors/CMakeLists.txt`),
  linked with `lamss::core_henrik_util` for `CmyVectorUtils`. The build links
  `libBHV_MaintainBearing.so` to that library in `autonomy/src/lib/behaviors/ivp_behaviors/`
  (installed in `<prefix>/lib/lamss/ivp_behaviors/`), and `create_behavior.cpp` returns this class
  for `BHV_MaintainBearing` **[code]**. Point pHelmIvP at the directory with `ivp_behavior_dir`
  (or `IVP_BEHAVIOR_DIRS`) **[code]**.
- Standalone: `MOOSDB`, and pHelmIvP with a `course` domain and the example above; post the heading
  and `BEARING_STAT=state=2,bearing=<deg>` **[guess]**. In a mission, pLamssTrackManager posts
  `BEARING_STAT` and the array simulator or pLamssAEL the heading **[guess]**.
- Tested by `lamss_autonomy_behaviors_load` (`autonomy/src/test/moos_smoke/behaviors_load.sh`
  with `smoke.bhv`): pHelmIvP loads and builds the behavior with the example parameters and
  reaches `DRIVE` **[test]**. The steering is not tested.

## Known issues and open questions

- `width` and `alternate_interval` have no defaults **[code]**. Without `width` the course function
  uses an indeterminate width; without `alternate_interval` the rate window and the switch timing
  do **[code]**.
- On the first iteration, `PENDING_HEADING_SWITCH` is computed from `last_switch` before it is set
  **[code]**.
- `true_bearing` and `bearing_state` keep their previous (initially indeterminate) values when the
  string lacks those keys **[code]**.
- In `single` mode with `alternate_interval` set, the single peak also switches side **[code]**.
  Is that intended?
- `quad` mode: a `width` above 90 gives a negative base width for the rear peaks **[code]**.
- `peak_width`, `bearing_type` and `absolute_bearing` are not used **[code]**.
- The comment on the threshold default says 1 degree/minute; the value is 3 **[code]**.
- `addInfoVars` is called after every recognised parameter, so the default `AEL_HEADING` and
  `BEARING_STAT` are registered even when other names are configured **[code]**; harmless
  **[guess]**.
- `extractOF()` is not checked for a null result before `setPWT` **[code]**.

## History

- From GobySoft/lamss `src/lib/lib_behaviors-henrik/BHV_MaintainBearing.{h,cpp}`; first commit
  2011-04-28 (Toby Schneider); 11 commits, mostly Henrik Schmidt, before the port. Main changes:
  `quad` mode (2011), base width 180 − width (2017), bearing-rate-adaptive side switching
  (November 2017).
- lamss3 port (GobySoft/lamss3#21): clang-format, the class moved into `lamss::autonomy`, the
  shared `create_behavior.cpp`, and `CmyVectorUtils` now from `lamss::core::henrik_util`.
