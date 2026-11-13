---
title: ConstantAltitude
toolbox: autonomy
kind: ivp-behavior
source: autonomy/src/lib/behaviors/depth/ConstantAltitude/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# ConstantAltitude

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

`BHV_ConstantAltitude` is an IvP behavior that asks the helm for the depth that keeps the vehicle
at a fixed `altitude` above the bottom, or below the ice when the DVL looks up **[code]**. It is
for bottom-following surveys with a sensor that needs a constant height **[guess]**;
pLamssMissionManager posts `SENSOR_ALTITUDE` = `altitude=<deploy_altitude>`, which reads like an
`updates` variable for this behavior **[guess]**.

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `NAV_DEPTH` | MOOS (helm) | double | `BHV_ConstantAltitude.cpp` | vehicle depth, m **[code]** | missing: posts an error, no objective function **[code]** |
| `NAV_ALTITUDE` | MOOS (helm) | double | `BHV_ConstantAltitude.cpp` | DVL range to the bottom (or to the ice with `inverted_dvl`), m **[code]** | missing or ≤ 0.1: no objective function **[code]** |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `MIN_ALT_STATUS` | MOOS (helm) | string | `BHV_ConstantAltitude.cpp` | `Missing NAV_DEPTH info` **[code]** | probably meant to be `CONST_ALT_STATUS` (copied from MinAltitude) **[guess]** |
| `CONST_ALT_STATUS` | MOOS (helm) | string | `BHV_ConstantAltitude.cpp` | `Missing NAV_ALTITUDE info` or `Invalid NAV_ALTITUDE info` **[code]** | consumer unknown |
<!-- END GENERATED publishes -->

The objective function itself goes to the helm over the `depth` decision variable **[code]**.

### Configuration

The behavior's own parameters, read in `setParam` **[code]**. The extractor does not list them yet
(it matches `param ==`, and this code uses `g_param ==`), so they are in a separate table below the
generated one.

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
<!-- END GENERATED config -->

| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `altitude` | double ≥ 0, m | `0` | no | altitude to hold **[code]** |
| `inverted_dvl` | bool | `false` | no | `NAV_ALTITUDE` is the range up to the ice; hold `altitude` below the ice instead of above the bottom (a code comment says ICEX) **[code]** |
| `missing_altitude_critical` | bool | `true` | no | intended to choose whether a missing `NAV_ALTITUDE` is an error **[guess]**; has no effect, see open questions **[code]** |

The standard IvPBehavior parameters (`name`, `pwt`, `condition`, `updates`, `endflag`, `duration`
and the others in `setParamCommon`) also apply **[code]**. The helm must have a `depth` domain
**[code]**.

Example (from `autonomy/src/test/moos_smoke/smoke.bhv`):

```
Behavior = BHV_ConstantAltitude
{
  name = constant_altitude
  altitude = 0.5
  condition = MODE == SMOKE
}
```

### Files, devices and external programs

None **[code]**.

## Behaviour

On each helm iteration while the behavior runs **[code]**:

- Normal DVL: target depth = `NAV_DEPTH` + `NAV_ALTITUDE` − `altitude` (water depth minus the
  altitude) **[code]**.
- `inverted_dvl`: target depth = `NAV_DEPTH` − `NAV_ALTITUDE` + `altitude` (ice depth plus the
  altitude) **[code]**.
- The objective function is a `ZAIC_PEAK` on `depth` with its summit at the target depth, peak
  width 0.1 × `altitude`, base width (target depth − peak width), summit delta 90 and utility
  0–100, weighted by `pwt` **[code]**.
- No function is produced when `NAV_DEPTH` is missing, `NAV_ALTITUDE` is missing, or
  `NAV_ALTITUDE` ≤ 0.1 m (treated as no bottom lock) **[code]**. The helm's other depth
  behaviors then decide **[guess]**.
- `onIdleState` does nothing; there is no state between iterations **[code]**.

## Running it

- No mission component launches it yet. It runs inside pHelmIvP **[code]**.
- It is built into `liblamss_autonomy_behaviors.so` (`autonomy/src/lib/behaviors/CMakeLists.txt`).
  The build makes a link `libBHV_ConstantAltitude.so` to that library in
  `autonomy/src/lib/behaviors/ivp_behaviors/` (installed in `<prefix>/lib/lamss/ivp_behaviors/`),
  and `create_behavior.cpp` returns this class for the name `BHV_ConstantAltitude` **[code]**.
  Point pHelmIvP at the directory with `ivp_behavior_dir` (or `IVP_BEHAVIOR_DIRS`) and add a
  `domain = depth:...` line **[code]**.
- Standalone: `MOOSDB`, and pHelmIvP with a `.bhv` containing the example above; post `NAV_DEPTH`
  and `NAV_ALTITUDE` (for example with `uPokeDB`) to see a depth decision **[guess]**.
- Tested by `lamss_autonomy_behaviors_load` (`autonomy/src/test/moos_smoke/behaviors_load.sh`
  with `smoke.bhv`): pHelmIvP loads the library, builds this behavior with the example parameters
  (`all_builds_ok: true`) and reaches `IVPHELM_STATE=DRIVE` **[test]**. The test publishes no
  navigation, so it does not check the depth decision **[test]**.

## Known issues and open questions

- `missing_altitude_critical` changes nothing: with it false the code sets the altitude to 0 but
  then returns no function, exactly as when it is true **[code]**. What was the intended behaviour
  without a bottom lock?
- A missing `NAV_DEPTH` is reported on `MIN_ALT_STATUS`, not `CONST_ALT_STATUS` **[code]**. Is
  that a copy-paste from BHV_MinAltitude?
- The base width is set to (target depth − peak width), so it grows with depth **[code]**. Was a
  fixed base width intended? Near the surface it becomes negative.
- `extractOF()` is not checked for a null result before `setPWT` **[code]**.
- Who reads `CONST_ALT_STATUS` and `MIN_ALT_STATUS`? Nothing in lamss3 subscribes to them.

## History

- From GobySoft/lamss `src/lib/lib_behaviors-henrik/BHV_ConstantAltitude.{h,cpp}`; first commit
  2011-04-28 (Toby Schneider); 5 commits by Toby Schneider and Henrik Schmidt before the port. Main
  changes: inactive without bottom lock (2014), `inverted_dvl` for ICEX (2020).
- lamss3 port (GobySoft/lamss3#21): clang-format, the class moved into `lamss::autonomy`, and
  the per-behavior `createBehavior` replaced by the shared `create_behavior.cpp`; behaviour
  unchanged.
