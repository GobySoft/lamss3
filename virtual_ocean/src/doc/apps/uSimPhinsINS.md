---
title: uSimPhinsINS
toolbox: virtual_ocean
kind: moos-app
source: virtual_ocean/src/bin/uSimPhinsINS/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# uSimPhinsINS

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

uSimPhinsINS simulates an iXblue PHINS inertial navigation system: it takes the vehicle's true
attitude, body velocity and position from the vehicle simulator (`FRONTSEAT_*`, e.g. uMVS_Bluefin
in a front-seat simulation community) and publishes a PHINS-like message
(`lamss.core.IXBlueINSData`) with attitude, attitude rates, noisy accelerations and velocities, and
a dead-reckoned position as `FRONTSEAT_PHINS_DATA` **[code]**. The app's `README.md` describes the
message and the sign conventions **[code]**. It was written to feed the HydroMAN navigation filter
in simulation (commit history, `HYDROMAN_ERROR_INS`) **[code]**; no lamss3 app reads
`FRONTSEAT_PHINS_DATA` yet **[code]**.

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `FRONTSEAT_DEPTH` | MOOS | double, m | `SimPhinsINS.cpp` | vehicle depth **[code]** | published as position altitude (negated) **[code]** |
| `FRONTSEAT_ROLL` | MOOS | double, rad | `SimPhinsINS.cpp` | roll, positive starboard down (README), converted to degrees **[code]** |  |
| `FRONTSEAT_PITCH` | MOOS | double, rad | `SimPhinsINS.cpp` | pitch, positive nose up (README), converted to degrees **[code]** |  |
| `FRONTSEAT_HEADING` | MOOS | double, deg | `SimPhinsINS.cpp` | heading, clockwise from north **[code]** | its arrival triggers an attitude-rate update **[code]** |
| `FRONTSEAT_BODY_VEL_Y` | MOOS | double, m/s | `SimPhinsINS.cpp` | surge u **[code]** | its arrival time is the time base for accelerations and dead reckoning **[code]** |
| `FRONTSEAT_BODY_VEL_X` | MOOS | double, m/s | `SimPhinsINS.cpp` | negated to give sway v, starboard positive **[code]** | README says starboard; code comment says "X seems positive port" |
| `FRONTSEAT_BODY_VEL_Z` | MOOS | double, m/s | `SimPhinsINS.cpp` | negated to give heave w, down positive **[code]** |  |
| `CURRENT_X` | MOOS | double, m/s | `SimPhinsINS.cpp` | water current, east **[code]** | added to the water-relative velocity **[code]** |
| `CURRENT_Y` | MOOS | double, m/s | `SimPhinsINS.cpp` | water current, north **[code]** |  |
| `FRONTSEAT_LATITUDE` | MOOS | double, deg | `SimPhinsINS.cpp` | true latitude **[code]** | resets the dead-reckoned position for the first 50 velocity updates, and gives the `HYDROMAN_ERROR_INS` reference **[code]** |
| `FRONTSEAT_LONGITUDE` | MOOS | double, deg | `SimPhinsINS.cpp` | true longitude **[code]** | as `FRONTSEAT_LATITUDE` |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `HYDROMAN_ERROR_INS` | MOOS | double, m | `SimPhinsINS.cpp` | horizontal distance between the dead-reckoned and the true position **[code]** | every `Iterate`; code comment: "not using this for anything" |
| `LAT_ORIGIN` | MOOS | double, deg | `SimPhinsINS.cpp` | `LatOrigin` from the mission file, posted once at start-up **[code]** | other apps (iNoaaBathy, uSimGPS) re-initialise their origin from it **[code]** |
| `LONG_ORIGIN` | MOOS | double, deg | `SimPhinsINS.cpp` | `LongOrigin`, posted once at start-up **[code]** | as `LAT_ORIGIN` |
| `FRONTSEAT_PHINS_DATA` | MOOS | string: protobuf `lamss.core.IXBlueINSData` (`serialize_for_moos`) | `SimPhinsINS.cpp` | simulated PHINS output: header, `attitude_deg`, `rot_rate_vessel_frame_degsec`, `acce_vessel_frame_msec2` and its stdev, `speed_vessel_frame_msec`, `position` **[code]** | every `Iterate` (`AppTick`) **[code]**; vessel-frame y and z are port and up positive **[code]** |
<!-- END GENERATED publishes -->

### Configuration

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `LatOrigin` | double |  | yes | global (mission file); local grid origin; the app exits without it **[code]** |
| `LongOrigin` | double |  | yes | global (mission file); the app exits without it **[code]** |
| `ins_bias_udot` | double, m/s² | none (uninitialised) | not checked | acceleration bias, surge **[code]** |
| `ins_bias_vdot` | double, m/s² | none (uninitialised) | not checked | acceleration bias, sway **[code]** |
| `ins_bias_wdot` | double, m/s² | none (uninitialised) | not checked | acceleration bias, heave **[code]** |
| `ins_scale_err` | double | none (uninitialised) | not checked | acceleration scale-factor error **[code]** |
| `ins_stdev` | double, m/s² | none (uninitialised) | not checked | acceleration white-noise standard deviation; also the published acceleration stdev **[code]** |
| `phins_ip` | string |  | yes **[guess]** | address for a UDP output of the PHINS binary protocol, "not implemented yet" **[code]** |
| `phins_port` | string |  | yes **[guess]** | port for that UDP output **[code]** |
| `FOO` | string |  | no | template placeholder, ignored **[code]** |
| `BAR` | string |  | no | template placeholder, ignored **[code]** |
<!-- END GENERATED config -->

`phins_ip` and `phins_port` are resolved at start-up to create the (unused) UDP socket; an empty or
unresolvable value makes the resolver throw, which ends the app **[guess]**.

Example (from `uSimPhinsINS.moos`):

```
ProcessConfig = uSimPhinsINS
{
   AppTick   = 20
   CommsTick = 20

   phins_ip = 127.0.0.1
   phins_port = 14000

   ins_bias_udot = 0.003
   ins_bias_vdot = 0.002
   ins_bias_wdot = 0.001
   ins_scale_err = 0.005
   ins_stdev = 0.01
}
```

### Files, devices and external programs

- A UDP socket (Boost.Asio) bound to an ephemeral port, with the endpoint `phins_ip:phins_port`
  **[code]**. Nothing is sent: `UDPClient::send` is never called **[code]**.

## Behaviour

On each `Iterate` (`AppTick`, 20 Hz in the example):

1. **Velocity.** The water-relative body velocity is rotated to the earth frame, the water
   current is added, and the result is rotated back: the true velocity over ground in the body
   frame **[code]**.
2. **Acceleration.** When a `FRONTSEAT_BODY_VEL_Y` has arrived since the last iteration and less
   than 2 s after the previous one, the acceleration is the difference of successive velocities
   over that time; otherwise the last value is kept **[code]**.
3. **Acceleration errors.** Each axis gets Gaussian noise (`ins_stdev`), a scale error
   (`ins_scale_err` × true acceleration) and a bias (`ins_bias_*dot`) **[code]**.
4. **Velocity output.** A velocity is drawn around the true one with ten times the acceleration
   error parameters on the surge axis (`ins_stdev` × 10) and ten times the scale and bias terms on
   all axes **[code]**. These factors are magic numbers.
5. **Position.** For the first 50 `FRONTSEAT_BODY_VEL_Y` updates the position is reset to the true
   one; after that it is integrated from the noisy velocity, so it drifts. `HYDROMAN_ERROR_INS` is
   the drift **[code]**.
6. **Attitude rates.** When a new `FRONTSEAT_HEADING` has arrived, roll, pitch and heading rates are
   the differences since the last heading update (heading unwrapped across 0/360°), averaged over
   the last 5 updates **[code]**.
7. **Output.** `FRONTSEAT_PHINS_DATA` is published with the true attitude, the averaged rates, the
   noisy accelerations and velocities (y and z negated to port and up positive) and the
   dead-reckoned position. The header fields are fixed placeholders, including
   `navigation_data_validity_time_100us`, marked TO-DO **[code]**.

- Noise is drawn from a global `std::default_random_engine` with its default seed, so every run
  produces the same noise sequence **[code]**.
- The code handles `LAT_ORIGIN`/`LONG_ORIGIN` mail by moving the origin and re-expressing the true
  and dead-reckoned positions in it, but the app never registers for these variables (see open
  questions) **[code]**.

## Running it

- No mission component launches it yet; the M1 plan puts it in the `macrura/frontseat_sim`
  community with uMVS_Bluefin and uSimDVL (`core/src/doc/plan/claude/10-milestone-mbat-macrura.md`)
  **[code]**.
- Standalone, in `virtual_ocean/src/bin/uSimPhinsINS/`: run `MOOSDB uSimPhinsINS.moos`, then
  `uSimPhinsINS uSimPhinsINS.moos`, with uMVS_Bluefin (`OutputPrefix=FRONTSEAT_`) providing the
  inputs.
- `lamss_virtual_ocean_moos_smoke` only checks that it connects to the MOOSDB **[test]**.
- `-e`, `-i` and `-h` print MOOS-IvP template text (`SimPhinsINS_Info.cpp`) **[code]**.

## Known issues and open questions

- [GobySoft/lamss3#28](https://github.com/GobySoft/lamss3/issues/28): uninitialised state
  (velocities, attitude, positions, `m_prev_u_time`, `m_current_heading_received`, the INS error
  parameters when not configured) and `m_current_u_received`, which is never reset.
- `LAT_ORIGIN` and `LONG_ORIGIN` are handled in `OnNewMail` but not registered in
  `RegisterVariables`, so the dynamic-origin code added in 2022 never runs **[code]**.
- The app posts `LAT_ORIGIN`/`LONG_ORIGIN` from its configuration in `OnStartUp` **[code]**. Is it
  meant to be the origin source for its community? Is the MOOSDB connection up at that point, so
  that the posts are delivered?
- The first attitude rate is computed against a previous heading, roll and pitch of 0 and the
  construction time, and stays in the 5-sample average for five updates **[code]**.
- The README shows the variable as `PHINS_DATA` and the type as
  `lamss.hydroman.protobuf.IXBlueINSData`; the code publishes `FRONTSEAT_PHINS_DATA` with
  `lamss.core.IXBlueINSData` **[code]**. The README needs updating.
- The UDP output is created but never used, yet it makes `phins_ip`/`phins_port` mandatory
  **[guess]**. Should it be finished or removed?
- Which lamss3 app is to consume `FRONTSEAT_PHINS_DATA` now that HydroMAN is not ported? Is
  `HYDROMAN_ERROR_INS` still wanted?
- The noise seed is fixed (see Behaviour). Is repeatability wanted?

## History

- From GobySoft/lamss-shared `src/moos/HydroMANSimUtilities/uSimPhinsINS`; first commit
  2019-10-03 (Supun Randeni), "Sketching up Phins INS simulator". It began as a copy of
  uSimDVL/uSimReplayPCAP, whose history stays with those apps.
- 17 commits before the port: Supun Randeni (2019–2020: the simulator, NaN handling, direct
  velocity computation) and Henrik Schmidt (2022: dynamic origin, README and sign-convention
  figure, position output, attitude rates, heading unwrap fix).
- lamss3 port (GobySoft/lamss3#23): the message moved from `lamss.hydroman.protobuf` to
  `lamss.core.IXBlueINSData`; `CMeasAvg` from core's `henrik_util`; Boost `io_service` replaced by
  `io_context`; a no-op statement `m_current_u_received == false;` removed (the flag is still never
  reset, #28); new `uSimPhinsINS.moos` example.
