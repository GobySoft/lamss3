---
title: henrik_util
toolbox: sonar
kind: library
source: sonar/src/lib/henrik_util/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# henrik_util

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

sonar's `lamss_sonar_henrik_util` (`lamss::sonar_henrik_util`) holds one class, `CTrack`: a bearing
track (number, priority, state, side, array position, bearing, accuracy, rate, SNR, time,
classification features) and its MOOS string formats **[code]**. It is sonar's share of the old
lib_henrik_util; the rest is in core's henrik_util **[code]**. [pLamssBearingTracker](../apps/pLamssBearingTracker.md)
uses it to write `BEARING_TRACKS`, and [pLamssTrackManager](../apps/pLamssTrackManager.md) to read
them and to write `BEARING_STAT` **[code]**. The formats come from the PLUSNet project (code
comments) **[code]**.

## Interfaces

A library has no MOOS interface of its own; it builds strings that the apps publish **[code]**.

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
<!-- END GENERATED publishes -->

### Configuration

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
<!-- END GENERATED config -->

### Files, devices and external programs

None **[code]**.

## API

Header `lamss/sonar/henrik_util/CTrack.h`, namespace `lamss::sonar` **[code]**. Times are
`MOOSTime()` **[code]**.

| Member | What it does |
|---|---|
| `setTrack(num, prio, templ, state, ips, array_x, array_y, bearing, sigma, rate, rate_sigma, snr, features)` | stores a track and stamps it with the current time **[code]** |
| `getTrack(...)` | returns the stored values, including the time **[code]** |
| `clrTrack()` | zeroes priority, state, side, bearing, accuracy, rate, SNR, features and dip angle; sets template 5 **[code]** |
| `postTrack(moosvar, sval)` | `moosvar = "BEARING_TRACKS"`, `sval = "track=…,priority=…,template=…,state=…,ips=…,ael_x=…,ael_y=…,bearing=…,bearing_sigma=…,rate=…,rate_sigma=…,snr=…,time=…,nfeat=N"` followed by N feature values; returns false (setting nothing) for a NaN bearing **[code]** |
| `decodeTrack(sval)` | parses that string back; returns false for a NaN bearing **[code]** |
| `bearingState(node, nav_x, nav_y)` | `node=…,state=…,bearing=…,xp=…,yp=…,beamno=0,sigma=…,rate=…,rate_sigma=…,snr=…,time=…`, the `BEARING_STAT` format **[code]** |
| `bearingRate(bearing, sigma, rate, rate_sigma)` | (bearing − stored bearing)/(time since stored); rate σ = 2σ/Δt **[code]** |
| `setDipAngle`, `getDipAngle`, `dipAngleRate`, `postDipAngle` | the same for an elevation (dip) angle; `postDipAngle` uses `ELEVATION_TRACKS` and negates the angle and rate **[code]** |

State values used by the apps: 0 no track, 1 relative/ambiguous, 2 absolute; side `ips` -1 port,
1 starboard, 0 ambiguous **[code]**.

Used by: pLamssBearingTracker and pLamssTrackManager **[code]**. The dip-angle functions have no
caller in lamss3 **[code]**.

## Known issues and open questions

- `CTrack` has no constructor: until `setTrack()`, `clrTrack()` or `decodeTrack()`, its members are
  uninitialised, and `clrTrack()` leaves `trk_num`, position, time and depth as they were.
  `decodeTrack()` leaves fields missing from the string unchanged, so a fresh `CTrack` decoding an
  incomplete string holds uninitialised values.
- `postTrack()` leaves `moosvar` unset when it returns false; pLamssBearingTracker then publishes
  with an empty variable name.
- `bearingRate()` does not unwrap across 0°/360°.
- The comments call the states "Relative" and "Absolute" while pLamssBearingTracker uses 1 for
  "LR_TRACK" (ambiguous) and 2 for "TRACKING". Are these the same states?
- Who reads `ELEVATION_TRACKS`? Nothing in lamss3 calls the dip-angle functions.

## History

- From GobySoft/lamss `src/lib/lib_henrik_util` (`CTrack.h`, `CTrack.cpp`); first commit 2011-04-27
  (Toby Schneider, during the migration to the lamss repository); 3 commits by Toby Schneider and
  Henrik Schmidt before the port, the last on 2015-11-16.
- lamss3 port (GobySoft/lamss3#22): clang-format; built as `lamss_sonar_henrik_util`, header
  moved to `lamss/sonar/henrik_util/`, namespace `lamss::sonar`; MOOS includes moved into the `.cpp`
  and MOOS linked privately so the installed library does not require MOOS; GPL-3.0-or-later SPDX
  headers added.
