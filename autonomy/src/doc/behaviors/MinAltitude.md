---
title: MinAltitude
toolbox: autonomy
kind: ivp-behavior
source: autonomy/src/lib/behaviors/depth/MinAltitude/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# MinAltitude

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

`BHV_MinAltitude` is an IvP behavior that keeps the vehicle at least `min_altitude` above the
bottom, or below the ice when the DVL looks up **[code]**. It is a safety constraint, run with a
high priority weight alongside the behaviors that choose the depth **[guess]** (`smoke.bhv` gives
it `pwt = 1000`).

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `NAV_DEPTH` | MOOS (helm) | double | `BHV_MinAltitude.cpp` | vehicle depth, m **[code]** | not checked for presence; 0 if missing **[guess]** |
| `NAV_ALTITUDE` | MOOS (helm) | double | `BHV_MinAltitude.cpp` | DVL range to the bottom (or to the ice with `inverted_dvl`), m **[code]** | missing or exactly 0: no objective function **[code]** |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
<!-- END GENERATED publishes -->

Nothing is posted to the MOOSDB; the objective function goes to the helm over `depth` **[code]**.

### Configuration

The behavior's own parameters, read in `setParam` **[code]**. The extractor does not list them yet
(this code uses `g_param ==`), so they are in a separate table below the generated one.

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
<!-- END GENERATED config -->

| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `min_altitude` | double ≥ 0, m | `0` | no | minimum distance to keep from the bottom (or the ice) **[code]** |
| `inverted_dvl` | bool | `false` | no | `NAV_ALTITUDE` is the range up to the ice; keep below the ice instead of above the bottom **[code]** |

The standard IvPBehavior parameters (`name`, `pwt`, `condition`, `updates`, `endflag` and the
others in `setParamCommon`) also apply **[code]**. The helm must have a `depth` domain **[code]**.

Example (from `autonomy/src/test/moos_smoke/smoke.bhv`):

```
Behavior = BHV_MinAltitude
{
  name = min_altitude
  pwt = 1000
  min_altitude = 6.0
  condition = MODE == SMOKE
}
```

### Files, devices and external programs

None **[code]**.

## Behaviour

On each helm iteration, when `NAV_ALTITUDE` is present and not 0 **[code]**:

- Normal DVL: the deepest allowed depth is `NAV_DEPTH` + `NAV_ALTITUDE` − `min_altitude`. The
  function is a `ZAIC_PEAK` on `depth` with summit 0, peak width equal to that deepest depth,
  base width 1, summit delta 0.1, utility 0–100 **[code]**: every depth from the surface down to
  the limit is (almost) equally good, and deeper ones fall to 0 within a metre **[guess]**.
- `inverted_dvl`: the shallowest allowed depth is `NAV_DEPTH` − `NAV_ALTITUDE` + `min_altitude`
  + 1. The function is a `ZAIC_HEQ` (greater-or-equal) with that summit and base width 1
  **[code]**.
- The function is weighted by `pwt` **[code]**.
- `NAV_ALTITUDE` = 0 is treated as no bottom lock: no function **[code]**.
- `onIdleState` does nothing **[code]**.

## Running it

- No mission component launches it yet. It runs inside pHelmIvP **[code]**.
- It is built into `liblamss_autonomy_behaviors.so` (`autonomy/src/lib/behaviors/CMakeLists.txt`).
  The build links `libBHV_MinAltitude.so` to that library in
  `autonomy/src/lib/behaviors/ivp_behaviors/` (installed in `<prefix>/lib/lamss/ivp_behaviors/`),
  and `create_behavior.cpp` returns this class for `BHV_MinAltitude` **[code]**. Point pHelmIvP
  at the directory with `ivp_behavior_dir` (or `IVP_BEHAVIOR_DIRS`) **[code]**.
- Standalone: `MOOSDB`, and pHelmIvP with a `depth` domain and the example above; post
  `NAV_DEPTH` and `NAV_ALTITUDE` to see the limit **[guess]**.
- Tested by `lamss_autonomy_behaviors_load` (`autonomy/src/test/moos_smoke/behaviors_load.sh`
  with `smoke.bhv`): pHelmIvP loads and builds the behavior with the example parameters and
  reaches `DRIVE` **[test]**. The depth limit itself is not tested.

## Known issues and open questions

- `NAV_DEPTH` is read but its presence is not checked **[code]**. Without it the limit is computed
  from depth 0, i.e. as if the vehicle were at the surface **[guess]**. Should the behavior return
  no function, as BHV_ConstantAltitude does?
- No bottom lock is signalled by `NAV_ALTITUDE` = 0 here, ≤ 0.1 in BHV_ConstantAltitude, and NaN in
  BHV_SmartYoyo (2021 commit "fixed yoyo behaviors for altitude NaN") **[code]**. Which does the
  navigation actually post?
- When the water is shallower than `min_altitude`, the deepest allowed depth is negative **[code]**.
  What should the helm do then?
- The inverted-DVL case adds 1 m to `min_altitude`; the normal case does not **[code]**. Why?

## History

- From GobySoft/lamss `src/lib/lib_behaviors-henrik/BHV_MinAltitude.{h,cpp}`; first commit
  2011-04-28 (Toby Schneider); 4 commits by Toby Schneider and Henrik Schmidt before the port. Main
  changes: run condition (2017), inverted DVL for ICEX (2020).
- lamss3 port (GobySoft/lamss3#21–#23): clang-format, the class moved into `lamss::autonomy`, and
  the shared `create_behavior.cpp`; behaviour unchanged.
