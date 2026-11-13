---
title: ConstantBuoyancy
toolbox: autonomy
kind: ivp-behavior
source: autonomy/src/lib/behaviors/depth/ConstantBuoyancy/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# ConstantBuoyancy

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

`BHV_ConstantBuoyancy` is an IvP behavior that asks the helm for a fixed value of a `buoyancy`
decision variable **[code]**. It is for a vehicle with a variable buoyancy engine, such as a
glider, which a downstream controller turns into pump or ballast commands **[guess]**. The unit
and sign of `buoyancy` are not defined in the code **[code]**.

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `NAV_DEPTH` | MOOS (helm) | double | `BHV_ConstantBuoyancy.cpp` | vehicle depth **[code]** | only its presence is used: missing means no function **[code]** |
| `NAV_ALTITUDE` | MOOS (helm) | double | `BHV_ConstantBuoyancy.cpp` | DVL altitude **[code]** | only checked; missing posts an error but the function is still produced **[code]** |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `CONST_BUOY_STATUS` | MOOS (helm) | string | `BHV_ConstantBuoyancy.cpp` | `Missing NAV_DEPTH info` **[code]** | consumer unknown |
<!-- END GENERATED publishes -->

The objective function goes to the helm over the `buoyancy` decision variable **[code]**.

### Configuration

The behavior's own parameter, read in `setParam` **[code]**. The extractor does not list it yet
(this code uses `g_param ==`), so it is in a separate table below the generated one.

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
<!-- END GENERATED config -->

| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `buoyancy` | double (any sign) | `0` | no | buoyancy setting to hold **[code]** |

The standard IvPBehavior parameters (`name`, `pwt`, `condition`, `updates`, `endflag` and the
others in `setParamCommon`) also apply **[code]**. The helm must declare a `buoyancy` domain, e.g.
`domain = buoyancy:-10:10:21` as in `behaviors_load.sh` and the behaviors README **[code]**.

Example (from `autonomy/src/test/moos_smoke/smoke.bhv`):

```
Behavior = BHV_ConstantBuoyancy
{
  name = constant_buoyancy
  buoyancy = -5.0
  condition = MODE == SMOKE
}
```

### Files, devices and external programs

None **[code]**.

## Behaviour

- On each helm iteration with `NAV_DEPTH` present, the behavior returns a `ZAIC_PEAK` on
  `buoyancy` with summit `buoyancy`, peak width 0.1 × |`buoyancy`|, base width 0.9 ×
  |`buoyancy`|, summit delta 80 and utility 0–100, weighted by `pwt` **[code]**.
- Neither depth nor altitude changes the setting; they are only checked **[code]**.
- `onIdleState` does nothing **[code]**.

## Running it

- No mission component launches it yet. It runs inside pHelmIvP **[code]**.
- It is built into `liblamss_autonomy_behaviors.so` (`autonomy/src/lib/behaviors/CMakeLists.txt`).
  The build links `libBHV_ConstantBuoyancy.so` to that library in
  `autonomy/src/lib/behaviors/ivp_behaviors/` (installed in `<prefix>/lib/lamss/ivp_behaviors/`),
  and `create_behavior.cpp` returns this class for `BHV_ConstantBuoyancy` **[code]**. Point
  pHelmIvP at the directory with `ivp_behavior_dir` (or `IVP_BEHAVIOR_DIRS`) **[code]**.
- Standalone: `MOOSDB`, and pHelmIvP with a `buoyancy` domain and the example above; post
  `NAV_DEPTH` to see a `buoyancy` decision **[guess]**.
- Tested by `lamss_autonomy_behaviors_load` (`autonomy/src/test/moos_smoke/behaviors_load.sh`
  with `smoke.bhv`, helm domain `buoyancy:-10:10:21`): pHelmIvP loads and builds the behavior and
  reaches `DRIVE` **[test]**.

## Known issues and open questions

- Without a `buoyancy` domain in the helm, the behavior's domain is empty. In a probe against the
  installed IvP library, constructing a `ZAIC_PEAK` on an empty domain crashed (segmentation fault)
  **[test]**, so a helm without the domain would probably crash on the behavior's first run
  **[guess]**. Should the behavior check `m_domain` as BHV_OptAcommsDepth does?
- With the default `buoyancy` = 0 the peak and base widths are both 0 **[code]**.
- What do `buoyancy` values mean (unit, sign), and which app turns the helm's `DESIRED_BUOYANCY`
  into an actuator command? Nothing in lamss3 subscribes to it.
- Why read `NAV_DEPTH` and `NAV_ALTITUDE` when neither changes the result? (The code began as a
  copy of BHV_MinAltitude.)
- `extractOF()` is not checked for a null result before `setPWT` **[code]**.

## History

- From GobySoft/lamss `src/lib/lib_behaviors-henrik/BHV_ConstantBuoyancy.{h,cpp}`; one commit,
  2020-07-10 (Henrik Schmidt), as a copy of BHV_MinAltitude.
- lamss3 port (GobySoft/lamss3#21–#23): clang-format, the class moved into `lamss::autonomy`, the
  shared `create_behavior.cpp`, and `setParam` now returns false for an unknown parameter (it fell
  off its end, undefined behaviour).
