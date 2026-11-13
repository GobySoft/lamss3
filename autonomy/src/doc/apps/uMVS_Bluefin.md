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

uMVS_Bluefin simulates the vehicle under a frontseat **[guess]**. It integrates a six-degree-of-freedom
AUV model, driven by rudder, elevator and thrust commands, and posts the resulting state as
`<OutputPrefix>X`, `…DEPTH`, `…HEADING` and so on **[code]**. In the missions the prefix is
`FRONTSEAT_`, and the other frontseat simulators read those variables (uSimPhinsINS, uSimGPS via
`TRUE_X`) **[code]**. The M1 plan runs it in the `frontseat_sim` subsystem with pMarinePID, uSimGPS,
uSimDVL, uSimPhinsINS, iBackSeatServer and iNoaaBathy **[guess]**. It is MOOS's `uMVS` (Paul Newman, David
Battle; Oxford/MIT, 2001–2005) with the hydrodynamics of an Odyssey III AUV by default **[code]**.
The LAMSS changes add:

- current drift and GPS resets **[code]**;
- seabed contact from `BATHY_Z` **[code]**;
- simulated Bluefin trim and buoyancy requests **[code]**;
- tow-cable forces from a towed array **[code]**;
- a moving local-grid origin **[code]**.

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `pAUV->m_sInputPrefix + "DESIRED_ELEVATOR"` (built at run time) | MOOS | double (deg) | `MVSim.cpp` | elevator command **[code]** | `<InputPrefix>DESIRED_ELEVATOR`; from pMarinePID **[guess]** |
| `pAUV->m_sInputPrefix + "DESIRED_RUDDER"` (built at run time) | MOOS | double (deg) | `MVSim.cpp` | rudder command **[code]** | `<InputPrefix>DESIRED_RUDDER` |
| `pAUV->m_sInputPrefix + "DESIRED_THRUST"` (built at run time) | MOOS | double (%) | `MVSim.cpp` | thrust command; RPM = thrust × 500 / 100 **[code]** | smoke-tested with 50 **[test]** |
| `pAUV->m_sInputPrefix + "CABLE_TENSION"` (built at run time) | MOOS | double (N) | `MVSim.cpp` | tow-cable tension applied at the tow point **[code]** | only with an array shape **[code]**; from a towed-array simulator **[guess]** |
| `pAUV->m_sInputPrefix + "ARRAY_SHAPE_X"` (built at run time) | MOOS | string: `x1,x2` | `MVSim.cpp` | x of the first two array elements; sets the cable direction **[code]** | from virtual_ocean uSimTowedArray **[guess]** |
| `pAUV->m_sInputPrefix + "ARRAY_SHAPE_Y"` (built at run time) | MOOS | string: `y1,y2` | `MVSim.cpp` | as `ARRAY_SHAPE_X` **[code]** | name built at run time |
| `pAUV->m_sInputPrefix + "ARRAY_SHAPE_Z"` (built at run time) | MOOS | string: `z1,z2` | `MVSim.cpp` | as `ARRAY_SHAPE_X` **[code]** | name built at run time |
| `pAUV->m_sInputPrefix + "TRIM_KG"` (built at run time) | MOOS | double (kg) | `MVSim.cpp` | excess buoyancy **[code]** | default 2.2 kg **[code]** |
| `LAT_ORIGIN` | MOOS | double | `MVSim.cpp`, `OnNewMail` | new origin latitude: the vehicle keeps its lat/lon and gets new x/y **[code]** | handled only when the AUV's `InputPrefix` is empty **[code]**; the app posts it itself at start-up **[code]** |
| `LONG_ORIGIN` | MOOS | double | `MVSim.cpp`, `OnNewMail` | as `LAT_ORIGIN` **[code]** | as `LAT_ORIGIN` |
| `TRIM_CONTROL` | MOOS | string: `true` | `MVSim.cpp` | starts a simulated trim manoeuvre that reports on `TRIM_REPORT` **[code]** | from the Bluefin frontseat driver **[guess]** |
| `BUOYANCY_CONTROL` | MOOS | string: `true` | `MVSim.cpp` | sets trim to 0 kg and starts a simulated buoyancy adjustment that reports on `BUOYANCY_REPORT` **[code]** | crashes when `InputPrefix` is not empty (see open questions) **[code]** |
| `BATHY_Z` | MOOS | double (m, negative water depth) | `MVSim.cpp` | sea floor below the vehicle **[code]**; altitude below 1 m puts the vehicle on the bottom **[code]** | handled only with an empty `InputPrefix` **[code]**; from virtual_ocean iNoaaBathy **[code]** |
| `SIM_RESET` | MOOS | any | `MVSim.cpp` | re-initialises the simulation from the configuration **[code]** |  |
| `GPS_UPDATE_RECEIVED` | MOOS | string: `Timestamp=…` | `MVSim.cpp`, `OnNewMail` | a GPS fix arrived: move the vehicle to `GPS_X`/`GPS_Y` and reset the current drift **[code]** | posted by pLamssMissionManager **[code]** |
| `GPS_X` | MOOS | double | `MVSim.cpp`, `OnNewMail` | GPS position x used on `GPS_UPDATE_RECEIVED` **[code]** | from virtual_ocean uSimGPS **[code]** |
| `GPS_Y` | MOOS | double | `MVSim.cpp`, `OnNewMail` | GPS position y **[code]** |  |
| `CURRENT_X` | MOOS | double (m/s) | `MVSim.cpp`, `OnNewMail` | east current estimate, integrated into the position drift **[code]** |  |
| `CURRENT_Y` | MOOS | double (m/s) | `MVSim.cpp`, `OnNewMail` | north current estimate **[code]** |  |
| `CURRENT_Z` | MOOS | double (m/s) | `MVSim.cpp`, `OnNewMail` | vertical current estimate **[code]** |  |
<!-- END GENERATED subscribes -->

### Publishes

With LBL transponders configured (`ADD_TRANSPONDER`), the vehicle's transceiver also posts
`<OutputPrefix>LBL_TOF` (string `Tx=…,Ch[n]=<time of flight>`) for each reply, through
`CSimEnvironment::AddReport` **[code]**. The extractor does not list it.

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `SIMULATION_MODE` | MOOS | string | `MVSim.cpp` | `TRUE`, on connecting **[code]** |  |
| `BUOYANCY_REPORT` | MOOS | string: `status=1,error=0,progressing,buoyancy=0.0` then `status=2,…,completed,…` | `MVSim.cpp` | progress of a buoyancy request **[code]** | completed after 5 s; progress every 5 s **[code]** |
| `TRIM_REPORT` | MOOS | string: `status=…,error=0,progressing,trim_pitch=…,trim_roll=0.0` | `MVSim.cpp` | progress of a trim request **[code]** | completed when pitch is within `trim_tolerance` after 5 s, or after `max_trim_delay` **[code]** |
| `LAT_ORIGIN` | MOOS | double | `MVSim.cpp` | `LatOrigin`, at start-up and on `SIM_RESET` **[code]** |  |
| `LONG_ORIGIN` | MOOS | double | `MVSim.cpp` | `LongOrigin`, as above **[code]** |  |
| `TRUE_X` | MOOS | double | `MVSim.cpp` | true position x including current drift **[code]** | read by virtual_ocean uSimGPS **[code]** |
| `TRUE_Y` | MOOS | double | `MVSim.cpp` | true position y including drift **[code]** |  |
| `TRUE_Z` | MOOS | double | `MVSim.cpp` | true z including drift **[code]** |  |
| `sP + "X"` (built at run time) | MOOS | double | `MVSim.cpp` | `<OutputPrefix>X`: position x **[code]** | with drift when `dvl_lock` (default) **[code]** |
| `sP + "Y"` (built at run time) | MOOS | double | `MVSim.cpp` | `<OutputPrefix>Y` **[code]** | as `X` |
| `sP + "LATITUDE"` (built at run time) | MOOS | double | `MVSim.cpp` | latitude of the posted x/y **[code]** | name built at run time |
| `sP + "LONGITUDE"` (built at run time) | MOOS | double | `MVSim.cpp` | longitude of the posted x/y **[code]** | name built at run time |
| `sP + "Z"` (built at run time) | MOOS | double | `MVSim.cpp` | z (negative down) **[code]** | name built at run time |
| `sP + "TOW_POS_X"` (built at run time) | MOOS | double | `MVSim.cpp` | tow point position x **[code]** | name built at run time |
| `sP + "TOW_POS_Y"` (built at run time) | MOOS | double | `MVSim.cpp` | tow point position y **[code]** | name built at run time |
| `sP + "TOW_POS_Z"` (built at run time) | MOOS | double | `MVSim.cpp` | tow point position z **[code]** | posted twice; the second post drops the drift **[code]** |
| `sP + "YAW"` (built at run time) | MOOS | double | `MVSim.cpp` | yaw (rad) **[code]** | name built at run time |
| `sP + "YAW_RATE"` (built at run time) | MOOS | double | `MVSim.cpp` | yaw rate (rad/s) **[code]** | name built at run time |
| `sP + "DEPTH"` (built at run time) | MOOS | double | `MVSim.cpp` | depth (m) **[code]** | smoke-tested **[test]** |
| `sP + "SPEED"` (built at run time) | MOOS | double | `MVSim.cpp` | speed through water (m/s) **[code]** | smoke-tested **[test]** |
| `sP + "HEADING"` (built at run time) | MOOS | double | `MVSim.cpp` | heading (deg), minus `MagneticOffset` **[code]** | name built at run time |
| `sP + "BODY_VEL_Y"` (built at run time) | MOOS | double | `MVSim.cpp` | body-frame velocity **[code]** | read by virtual_ocean uSimPhinsINS **[code]** |
| `sP + "BODY_VEL_X"` (built at run time) | MOOS | double | `MVSim.cpp` | body-frame velocity **[code]** | name built at run time |
| `sP + "BODY_VEL_Z"` (built at run time) | MOOS | double | `MVSim.cpp` | body-frame heave velocity **[code]** | name built at run time |
| `sP + "PITCH"` (built at run time) | MOOS | double | `MVSim.cpp` | pitch (rad) **[code]** | name built at run time |
| `sP + "PITCH_RATE"` (built at run time) | MOOS | double | `MVSim.cpp` | pitch rate (rad/s) **[code]** | name built at run time |
| `sP + "ROLL"` (built at run time) | MOOS | double | `MVSim.cpp` | always 0 (roll is not modelled) **[code]** | name built at run time |
| `sP + "RUDDER"` (built at run time) | MOOS | double | `MVSim.cpp` | actual rudder angle (deg) **[code]** | limited to ±13° **[code]** |
| `sP + "ELEVATOR"` (built at run time) | MOOS | double | `MVSim.cpp` | actual elevator angle (deg) **[code]** | limited to ±13° **[code]** |
| `sP + "RPM"` (built at run time) | MOOS | double | `MVSim.cpp` | propeller RPM **[code]** | name built at run time |
| `sP + "TOW_VEL_X"` (built at run time) | MOOS | double | `MVSim.cpp` | tow point velocity **[code]** | name built at run time |
| `sP + "TOW_VEL_Y"` (built at run time) | MOOS | double | `MVSim.cpp` | tow point velocity **[code]** | name built at run time |
| `sP + "TOW_VEL_Z"` (built at run time) | MOOS | double | `MVSim.cpp` | tow point velocity **[code]** | name built at run time |
| `TIDE_HEIGHT` | MOOS | double | `MVSim.cpp` | `TideHeight` **[code]** |  |
<!-- END GENERATED publishes -->

### Configuration

Besides the parameters below, the `ProcessConfig` block takes **[code]**:

| Parameter | Meaning |
|---|---|
| `ADD_AUV = pose=[4x1]{x,y,z,heading},name=…,InputPrefix=…,OutputPrefix=…[,ResponderChannel=…,TAT=…]` | adds a vehicle **[code]**; `InputPrefix`, `OutputPrefix` and `name` are required **[code]** |
| `ADD_TRANSPONDER = name=…,pose=[3x1]{x,y,z},Tx=…,Rx=…,TAT=…` | adds an LBL transponder **[code]** |
| `<name>_HullLength`, `<name>_Mass`, `<name>_Trim`, `<name>_MaxRudder`, … (34 keys) | vehicle hydrodynamic parameters; defaults are an Odyssey III in long configuration **[code]** |
| `LogFile` | log file for vehicle states and acoustics (none by default) **[code]** |
| `AddNoise` | `TRUE` adds white noise to the posted position, depth, heading and velocities **[code]** |
| `XYNOISE`, `ZNOISE`, `YAWNOISE` (deg), `YAWBIAS`, `XYVELNOISE` | noise standard deviations (defaults 0.1, 0.03, 0.5, 0, 0.02) **[code]** |
| `TOFNoise`, `MultiPathProbablity`, `InstantLogAcoustics` | LBL acoustics noise, multipath probability and logging **[code]** |
| `CURRENT_DEPTH`, `CURRENT_SPEED`, `CURRENT_HEADING` | if all three are present a flag is set, but nothing uses it **[code]** |

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `LatOrigin` | double |  | yes | global (mission file); the app exits without it **[code]** |
| `LongOrigin` | double |  | yes | global (mission file); the app exits without it **[code]** |
| `TerrainFile` | string | `terrain.dat` | no | global (mission file); terrain grid, read but its altitude is not used **[code]**; a missing file only warns **[code]** |
| `TideHeight` | double | `30` | no | posted as `TIDE_HEIGHT` **[code]** |
| `MagneticOffset` | double (deg) | `0` | no | global (mission file); subtracted from the heading **[code]** |
| `dvl_lock` | string `true`/`false` | `true` | no | include current drift in the posted position **[code]** |
| `trim_tolerance` | double (deg) | `5` | no | pitch within which a trim request completes **[code]** |
| `max_trim_delay` | double (s) | `10` | no | time after which a trim request completes anyway **[code]** |
<!-- END GENERATED config -->

Example (from `uMVS_Bluefin.moos`):

```
LatOrigin  = 42.358
LongOrigin = -71.087
MagneticOffset = 0

ProcessConfig = uMVS_Bluefin
{
    AppTick   = 10
    CommsTick = 10

    // subscribes to DESIRED_RUDDER/ELEVATOR/THRUST, publishes FRONTSEAT_X, FRONTSEAT_DEPTH, ...
    ADD_AUV = pose=[4x1]{0,0,-1,90},name=example,InputPrefix=,OutputPrefix=FRONTSEAT_

    TideHeight = 0
    InstantLogAcoustics = false

    // standard deviation of noise on TOF measurements: 1 ms = 1.5 m
    TOFNoise = 0.00066
}
```

### Files, devices and external programs

- Reads the terrain grid `TerrainFile` (default `terrain.dat`: rows, columns, x and y axes, then the grid) **[code]**. A missing file only warns, and the terrain altitude is not used by the model (the code is commented out) **[code]**.
- Writes `LogFile`, if set, truncating it at start-up **[code]**.

## Behaviour

- Start-up and `SIM_RESET` **[code]**:
    - delete all entities;
    - read the origin (exiting if it is missing) and post it;
    - load the parameters and create the `ADD_AUV` and `ADD_TRANSPONDER` entities.
- Each iteration **[code]**:
    - advances the model to the current MOOS time, in sub-steps of at most 10 ms;
    - integrates the equations of motion with an adaptive ODE solver (Numerical Recipes `odeint`), with a fourth-order actuator model for rudder and elevator;
    - limits rudder and elevator to ±13°.
- Roll is not modelled; `ROLL` is always 0 **[code]**.
- Results are posted at 5 Hz of simulator time **[code]**.
- Drift **[code]**:
    - between GPS fixes, the `CURRENT_X/Y/Z` estimates are integrated into a drift that is added to `TRUE_*` and, with `dvl_lock`, to the posted position;
    - `GPS_UPDATE_RECEIVED` moves the vehicle to `GPS_X`/`GPS_Y` and zeroes the drift.
- Seabed contact (`BATHY_Z`) **[code]**:
    - the altitude is water depth + z;
    - at 1 m or less, the vehicle is held at 0.5 m altitude or more, with zero velocities and pitch;
    - below half the hull diameter the model adds bottom drag and support.
- Buoyancy at the surface depends on how much of the hull is submerged **[code]**. Excess buoyancy is `TRIM_KG` (default 2.2 kg) **[code]**.
- Trim and buoyancy requests are simulated as timed handshakes **[code]**; they do not change the dynamics except that `BUOYANCY_CONTROL` sets the trim to 0 **[code]**.
- A new origin keeps each vehicle's latitude/longitude and recomputes its x/y **[code]**.
- `Initialise()` calls `SetAppFreq(50)` during start-up **[code]**, which probably overrides `AppTick` **[guess]**.

## Running it

- No mission component launches it yet.
- Standalone, from `autonomy/src/bin/uMVS_Bluefin/`: `MOOSDB uMVS_Bluefin.moos &` then `uMVS_Bluefin uMVS_Bluefin.moos` (an optional second argument sets the process name) **[code]**. Post `DESIRED_THRUST` to make it move.
- Tested by `lamss_autonomy_moos_smoke`: with `ADD_AUV … InputPrefix=,OutputPrefix=FRONTSEAT_` and `DESIRED_THRUST=50`, it checks that `FRONTSEAT_DEPTH` and `FRONTSEAT_SPEED` are posted **[test]**.

## Known issues and open questions

- `MakeBeacon` reads the `Rx` field into `sTx` instead of `sRx`, so a transponder never gets receive channels (`MVSim.cpp`, `MOOSValFromString(sTx, sConfig, "Rx")`) **[code]**. It may never answer pings. Are LBL transponders still used?
- `GPSUpdate()` and `UTMUpdate()` `dynamic_cast` every entity to `CSixDOFAUV` without a null check **[code]**. With an `ADD_TRANSPONDER` configured, a `GPS_UPDATE_RECEIVED` or a new origin dereferences a null pointer **[code]**.
- `BUOYANCY_CONTROL` is handled outside the `if (pAUV)` block but writes `pAUV->m_dfTrim` **[code]**. With a non-empty `InputPrefix`, `GetSubscriber` returns null and the app crashes **[code]**.
- `LAT_ORIGIN`, `LONG_ORIGIN`, `BATHY_Z` and `TRIM_CONTROL` are handled inside the per-vehicle prefix match, so they work only with an empty `InputPrefix` **[code]**. Intended?
- `gps_x`/`gps_y` are not initialised **[code]**. A `GPS_UPDATE_RECEIVED` before any `GPS_X`/`GPS_Y` moves the vehicle to an indeterminate position.
- `pitch_degrees` is not initialised before the first post, and `m_dfProbMultiPath` is not initialised unless `MultiPathProbablity` is configured **[code]**. The transceiver reads the latter on every reply.
- `<OutputPrefix>TOW_POS_Z` is posted twice per update; the second post drops the drift **[code]**.
- `MakeBeacon` leaks the beacon when its pose is not a 3-vector **[code]**.
- `GPS_UPDATE_RECEIVED` parses `Latitude`/`Longitude` and discards them **[code]**.
- The terrain is loaded but not used **[code]**. Should `TerrainFile` go, now that `BATHY_Z` gives the depth?
- The non-real-time branch (`m_bRealTime` false) cannot be selected **[code]**.
- The name says Bluefin, but the default vehicle parameters are an Odyssey III **[code]**. Do the missions set Bluefin parameters (`<name>_*`)?

## History

- From GobySoft/lamss `src/moos/uMVS_Bluefin/` (imported at lamss 3186a63) **[code]**. It is MOOS's `uMVS`; the source files carry the 2001–2005 MIT/Oxford copyright **[code]**. First commit in lamss 2011-04-26 (Toby Schneider). 26 commits before the port, mostly by Henrik Schmidt:
    - buoyancy and trim control, 2012;
    - current drift fix, 2012;
    - bottoming and `TRIM_KG`, 2020;
    - dynamic UTM, 2021;
    - `FRONTSEAT_LATITUDE/LONGITUDE`, 2022.

    Other authors: Toby Schneider (RPM output, 2019), Stephanie Petillo (vertical currents, 2014) and Supun Randeni (heave velocity, 2019).
- lamss3 Phase 2 port (GobySoft/lamss3#21–#23):
    - newmat now comes from Ubuntu's `libnewmat10-dev` instead of the vendored copy;
    - `-Wall -Wextra` fixes with no change in behaviour;
    - `uMVS.moos` became `uMVS_Bluefin.moos`, with a one-vehicle `ADD_AUV`;
    - Windows left-overs removed (`icon1.ico`, `uMVS.rc`, `resource.h`, a 2005 `SimLog.txt`);
    - licence headers set to GPL-3.0-or-later;
    - formatted with clang-format.
