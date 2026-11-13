---
title: ZigZag
toolbox: autonomy
kind: ivp-behavior
source: autonomy/src/lib/behaviors/tracking/ZigZag/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# ZigZag

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

`BHV_ZigZag`, loaded under the name **`BHV_LamssZigZag`**, is an IvP behavior that steers the
vehicle along a sinusoidal or zig-zag path about a base `heading`, at a fixed `speed`, starting
from where the behavior first runs **[code]**. It is for surveys that cover a swath or keep a towed
array moving across a bearing **[guess]**. pLamssMissionManager posts `ZIGZAG_CONFIG` =
`heading=…#period=…#amplitude=…#speed=…`, which reads like this behavior's `updates` variable
**[guess]**.

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `NAV_X` | MOOS (helm) | double | `BHV_ZigZag.cpp` | vehicle position, local grid, m **[code]** | not checked for presence **[code]** |
| `NAV_Y` | MOOS (helm) | double | `BHV_ZigZag.cpp` | vehicle position, local grid, m **[code]** | not checked for presence **[code]** |
| `CURRENT_X` | MOOS (helm) | double | `BHV_ZigZag.cpp` | east water current, m/s **[guess]** | only with `current_compensator = on` **[code]** |
| `CURRENT_Y` | MOOS (helm) | double | `BHV_ZigZag.cpp` | north water current, m/s **[guess]** | only with `current_compensator = on` **[code]** |
| `LAT_ORIGIN` | MOOS (helm) | double | `BHV_ZigZag.cpp` | local-grid origin latitude **[code]** | required before the start point is set; a change moves the start point so it stays fixed in lat/lon **[code]** |
| `LONG_ORIGIN` | MOOS (helm) | double | `BHV_ZigZag.cpp` | local-grid origin longitude **[code]** | as `LAT_ORIGIN` |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `COMPENSATED_HEADING` | MOOS (helm) | double, degrees | `BHV_ZigZag.cpp` | heading after current compensation **[code]** | only with `current_compensator = on`; BHV_RaceTrack posts the same name **[code]** |
<!-- END GENERATED publishes -->

The objective function goes to the helm over `course` and `speed` **[code]**.

### Configuration

The behavior's own parameters, read in `setParam` **[code]**. The extractor does not list them yet
(this code uses `g_param ==`), so they are in a separate table below the generated one.

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
<!-- END GENERATED config -->

| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `speed` | double > 0, m/s | none | yes (no default) | speed to hold **[code]** |
| `heading` | double ≥ 0, degrees | `0` | no | base heading of the path **[code]** |
| `period` | double > 0, m | `500` | no | along-track length of one full zig-zag **[code]** |
| `amplitude` | double ≥ 0, m | `250` | no | cross-track amplitude **[code]** (for `linear`, see open questions) |
| `zigzag_function` | string: `linear` or anything else | `sinusoidal` | no | `linear` gives straight legs; any other value a sine path **[code]** |
| `current_compensator` | `on` / `off` | `off` | no | correct the heading for `CURRENT_X`/`CURRENT_Y` **[code]** |

The standard IvPBehavior parameters (`name`, `pwt`, `condition`, `updates`, `endflag` and the
others in `setParamCommon`) also apply **[code]**. The helm must have `course` and `speed`
domains **[code]**.

Example (from `autonomy/src/test/moos_smoke/smoke.bhv`):

```
Behavior = BHV_LamssZigZag
{
  name = zigzag
  speed = 1.5
  period = 300
  amplitude = 30
  heading = 135
  zigzag_function = linear
  condition = MODE == SMOKE
}
```

### Files, devices and external programs

None **[code]**.

## Behaviour

- Start point: on the first iteration after `LAT_ORIGIN` and `LONG_ORIGIN` are known, the current
  `NAV_X`/`NAV_Y` becomes the start point and is also stored in lat/lon **[code]**. When the origin
  changes later, the start point is recomputed from that lat/lon **[code]** (support for a moving
  local grid, "dynamic UTM", 2021).
- Distance travelled is the projection of (position − start) on the base heading **[code]**.
- Heading command **[code]**:
    - behind the start point (distance < 0): the base heading;
    - sine: base + atan(2π × `amplitude` / `period` × cos(2π × distance / `period`)), the slope of
      a path offset by `amplitude` × sin(2π × distance / `period`);
    - `linear`: base ± atan(2 × `amplitude` / `period`), switching side every half `period`, with
      the switches at a quarter period, three quarters, and so on.
- With `current_compensator = on`, the heading is replaced by the heading through the water that
  makes good the commanded heading at `speed`, given the current; if the current is stronger than
  `speed`, the vehicle heads into the current **[code]**.
- The function couples a `ZAIC_PEAK` on `course` (peak width 30, base width 120, summit delta 60,
  wrapping) with a `ZAIC_PEAK` on `speed` (peak width 0.5, summit delta 100), weighted by `pwt`
  **[code]**.
- `onIdleState` forgets the start point, so each activation starts a new path from where the
  vehicle is **[code]**.

## Running it

- No mission component launches it yet. It runs inside pHelmIvP **[code]**.
- It is built into `liblamss_autonomy_behaviors.so` (`autonomy/src/lib/behaviors/CMakeLists.txt`)
  and loaded as `BHV_LamssZigZag`: the build links `libBHV_LamssZigZag.so` to the library in
  `autonomy/src/lib/behaviors/ivp_behaviors/` (installed in `<prefix>/lib/lamss/ivp_behaviors/`),
  and `create_behavior.cpp` returns this class for `BHV_LamssZigZag` **[code]**. pHelmIvP 24.8
  has its own unrelated `BHV_ZigZag` and prefers it, so a `.bhv` must say `BHV_LamssZigZag`
  **[code]** (README). Point pHelmIvP at the directory with `ivp_behavior_dir` (or
  `IVP_BEHAVIOR_DIRS`) **[code]**.
- Standalone: `MOOSDB`, and pHelmIvP with `course` and `speed` domains and the example above;
  post `LAT_ORIGIN`, `LONG_ORIGIN`, `NAV_X` and `NAV_Y` **[guess]**.
- Tested by `lamss_autonomy_behaviors_load` (`autonomy/src/test/moos_smoke/behaviors_load.sh`
  with `smoke.bhv`): pHelmIvP loads `BHV_LamssZigZag`, builds it with the example parameters and
  reaches `DRIVE` **[test]**. The path itself is not tested.

## Known issues and open questions

- `speed` has no default: without it the speed function uses an indeterminate value **[code]**.
  Should it default (e.g. to 0) or make the build fail?
- Without `LAT_ORIGIN`/`LONG_ORIGIN` the start point is never set and stays at (0, 0), so the
  distance is measured from the grid origin **[code]**. Should the behavior work without them?
- The `linear` path has half the cross-track excursion of the sine path for the same `amplitude`
  (± `amplitude`/2 against ± `amplitude`) **[code]**. Which is intended?
- The origin check compares the new origin with the previous one starting from 0, 0, so an origin
  of exactly (0, 0) is never used **[code]**; harmless in practice **[guess]**.
- The current is read every iteration here but only once at the start in BHV_RaceTrack **[code]**.
  Which app posts `CURRENT_X`/`CURRENT_Y` on a vehicle? In lamss3 only the simulators read them.

## History

- From GobySoft/lamss `src/lib/lib_behaviors-henrik/BHV_ZigZag.{h,cpp}`; first commit 2011-04-28
  (Toby Schneider); 2 commits before the port, the second (2021, Henrik Schmidt) adding dynamic
  UTM support.
- lamss3 port (GobySoft/lamss3#21): clang-format, the class moved into `lamss::autonomy`, the
  helm name changed from `BHV_ZigZag` to `BHV_LamssZigZag`, and `theta` in
  `currentCompensatedHeading` initialised to 0 (a gcc warning; it only mattered on a path that
  read it uninitialised).
