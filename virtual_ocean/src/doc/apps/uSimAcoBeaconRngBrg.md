---
title: uSimAcoBeaconRngBrg
toolbox: virtual_ocean
kind: moos-app
source: virtual_ocean/src/bin/uSimAcoBeaconRngBrg/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# uSimAcoBeaconRngBrg

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

uSimAcoBeaconRngBrg simulates a USBL-style range and bearing sensor that sees an acoustic beacon at
a known position (`BEACON_X`, `BEACON_Y`) when it is within range and within a forward-looking
swath **[code]**. It keeps a particle cloud for the beacon position, refreshes it from noisy
range/bearing detections and lets it random-walk between them, and publishes the cloud's mean
relative to the vehicle with range and azimuth standard deviations as `S-MOCAB_*` **[code]**.
pTetraHoming subscribes to the `S-MOCAB_*` variables and pLamssMissionManager posts
`USBL_ENABLED=TRUE` **[code]**, so the app stands in for a real USBL during homing to a
beacon, e.g. an ice camp under ice **[guess]**.

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `IFS_DATA_IN` | MOOS | string: `goby.middleware.frontseat.protobuf.InterfaceData` (`parse_for_moos`) | `SimAcoBeaconRngBrg.cpp` | vehicle navigation from the front-seat interface: `node_status.local_fix` x/y and `pose.heading` **[code]** | messages without `node_status` are ignored **[code]**; a code comment says this replaced `NAV_X/Y/HEADING` in 2020 for HydroMAN navigation during ICEX |
| `USBL_ENABLED` | MOOS | string: `TRUE` or anything else | `SimAcoBeaconRngBrg.cpp` | switches the sensor and all publications on (`TRUE`, exact case) or off **[code]** | off at start-up; posted `TRUE` by pLamssMissionManager **[code]** |
| `m_update_var` (built at run time) | MOOS | string: `beacon_x=<m>,beacon_y=<m>` | `SimAcoBeaconRngBrg.cpp` | moves the beacon; either field may be left out **[code]** | the variable named by `UPDATES` (e.g. `BEACON_UPDATE`); not registered when `UPDATES` is unset **[code]** |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `S-MOCAB_TGT_X` | MOOS | double, m | `SimAcoBeaconRngBrg.cpp` | estimated beacon east offset from the vehicle (particle mean minus vehicle x) **[code]** | every `Iterate` while enabled **[code]**; read by pTetraHoming **[code]** |
| `S-MOCAB_TGT_Y` | MOOS | double, m | `SimAcoBeaconRngBrg.cpp` | estimated beacon north offset from the vehicle **[code]** | as `S-MOCAB_TGT_X` |
| `S-MOCAB_STD_AZIMUTH` | MOOS | double, rad | `SimAcoBeaconRngBrg.cpp` | spread of the particle bearings around the mean bearing **[code]** | radians, as pTetraHoming expects **[code]**; see open questions |
| `S-MOCAB_STD_RANGE` | MOOS | double, m | `SimAcoBeaconRngBrg.cpp` | spread of the particle ranges around the mean range **[code]** |  |
| `VIEW_POINT` | MOOS | string: XYPoint spec | `SimAcoBeaconRngBrg.cpp` | one red point per particle (`P<i>`) and a white point at the mean (`T`) **[code]** | `NUM_PARTICLES` + 1 posts per `Iterate` **[code]** |
| `VIEW_POLYGON` | MOOS | string: XYPolygon spec | `SimAcoBeaconRngBrg.cpp` | pie wedge showing the sensor swath, label `sensor_swath` **[code]** | drawn at a tenth of `SENSOR_RANGE` **[code]** |
<!-- END GENERATED publishes -->

### Configuration

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `BEACON_X` | double, m | `0` | no | beacon east position in the local grid **[code]** |
| `BEACON_Y` | double, m | `0` | no | beacon north position **[code]** |
| `SENSOR_RANGE` | double, m | `1500` | no | maximum detection range **[code]** |
| `SENSOR_SWATH` | double, deg | `180` | no | total width of the detection sector, centred on the vehicle heading **[code]** |
| `SENSOR_RANGE_STDDEV` | double, m | `0.5` | no | range noise per particle **[code]** |
| `SENSOR_SWATH_STDDEV` | double, deg | `4` | no | bearing noise per particle **[code]** |
| `RANDOM_WALK_STDDEV` | double, m | `1` | no | particle random-walk step per sensor period without a detection **[code]** |
| `SENSOR_PROBABILITY` | double, 0 to 1 | `0.95` | no | probability that a beacon in the swath is detected **[code]** |
| `SENSOR_PERIOD` | double, s | `1` | no | time between sensor updates **[code]** |
| `NUM_PARTICLES` | unsigned int | `250` | no | size of the particle cloud **[code]** |
| `UPDATES` | string | empty | no | name of a variable that moves the beacon **[code]** |
| `FOO` | string |  | no | template placeholder, ignored **[code]** |
| `BAR` | string |  | no | template placeholder, ignored **[code]** |
<!-- END GENERATED config -->

Example (from `uSimAcoBeaconRngBrg.moos`):

```
ProcessConfig = uSimAcoBeaconRngBrg
{
   AppTick   = 4
   CommsTick = 4

   BEACON_X = 0.0             // beacon position
   BEACON_Y = 0.0
   SENSOR_RANGE = 1000.0      // sensor range
   SENSOR_SWATH = 180.0       // sensor arc swath (180 deg -> sensor can only 'see' ahead)
   SENSOR_RANGE_STDDEV = 0.5  // sensor std. dev.
   SENSOR_SWATH_STDDEV = 4.0
   RANDOM_WALK_STDDEV = 1.0   // particle random walk std. dev. per sensor period
   SENSOR_PROBABILITY = 0.95  // probability of sensor detecting beacon when in swath
   SENSOR_PERIOD = 1.0        // sensor update rate
   NUM_PARTICLES = 100        // number of particles
   UPDATES = BEACON_UPDATE    // allows update of beacon position (e.g. BEACON_UPDATE = beacon_x=4.0, beacon_y=-12.0)
}
```

### Files, devices and external programs

None **[code]**.

## Behaviour

- At start-up the particles are scattered around the grid origin with a standard deviation of
  500 m **[code]**.
- Nothing is computed or published until `USBL_ENABLED` is `TRUE` **[code]**. While enabled, each
  `Iterate` (`AppTick`) adds the elapsed time to an accumulator and, once `SENSOR_PERIOD` has
  passed, runs one sensor update **[code]**:
  - **In swath** (distance below `SENSOR_RANGE` and compass bearing to the beacon within
    heading ± `SENSOR_SWATH`/2) and a uniform draw below `SENSOR_PROBABILITY`: every particle is
    replaced by a new noisy range/bearing measurement from the vehicle **[code]**.
  - **Otherwise**: every particle random-walks by `RANDOM_WALK_STDDEV` in x and y **[code]**.
  - Then the mean position and the range and azimuth spreads are recomputed **[code]**.
- Until the first detection, and once navigation has arrived, the estimate is forced to the grid
  origin (0, 0) on every iteration **[code]**.
- Every enabled iteration publishes the four `S-MOCAB_*` values, all particles and the mean as
  `VIEW_POINT`, and the swath as `VIEW_POLYGON` **[code]**.
- Random numbers come from `rand()` without a seed, so runs repeat **[code]**.

## Running it

- No mission component launches it yet.
- Standalone, in `virtual_ocean/src/bin/uSimAcoBeaconRngBrg/`: run `MOOSDB uSimAcoBeaconRngBrg.moos`,
  then `uSimAcoBeaconRngBrg uSimAcoBeaconRngBrg.moos`, post `USBL_ENABLED=TRUE` and provide
  `IFS_DATA_IN` (e.g. from iFrontSeat with its simulator).
- `lamss_virtual_ocean_moos_smoke` only checks that it connects to the MOOSDB **[test]**.
- `-e`, `-i` and `-h` print MOOS-IvP template text under the old name pSimAcoBeaconRngBrg, not
  this app's interface (`SimAcoBeaconRngBrg_Info.cpp`) **[code]**.

## Known issues and open questions

- The swath test compares the compass bearing (0 to 360°) with heading ± swath/2 without
  wrapping, so near north the beacon is missed: with heading 10° and a 180° swath a beacon at
  350° is outside the swath **[code]**.
- `StdDevParticles` wraps angle differences with `fmod`, which keeps the sign of a negative
  argument; when the beacon is roughly west of the vehicle (bearings near ±π) particles on the other side of ±π
  count as nearly 2π away and the spread comes out far too large **[code]**. pTetraHoming, which compares it with its default
  7° threshold (in radians), would then not accept the estimate **[code]**.
- `m_target_x`, `m_target_y`, `m_std_azimuth` and `m_std_range` are not initialised; with
  `USBL_ENABLED` set before the first sensor update and before navigation arrives, uninitialised
  values are published **[code]**.
- `Randn` takes `log(u1)` of a uniform draw that can be exactly 0, giving an infinite particle
  coordinate **[code]**. `NUM_PARTICLES = 0` divides by zero **[code]**.
- Up to `NUM_PARTICLES` + 1 `VIEW_POINT` posts per iteration (e.g. 1004 per second at the defaults
  and `AppTick = 4`) **[code]**. Should drawing be optional or rate-limited?
- The swath drawing uses a tenth of `SENSOR_RANGE` (magic number), and the code comment says only
  convex wedges are drawn **[code]**.
- What does `S-MOCAB` stand for? `m_timewarp`, `m_iterations` and `last_nav_` are kept but unused
  **[code]**; `m_nav_set` is a `double` used as a flag **[code]**.

## History

- From GobySoft/lamss-shared `src/moos/uSimAcoBeaconRngBrg`; first commit 2020-02-20 (Henrik
  Schmidt), "Moved pAcoBearingRngBrgSim to here and renamed"; the earlier history of
  pAcoBearingRngBrgSim is not imported.
- 2 commits before the port (Henrik Schmidt; Toby Schneider, 2020-09-24, for goby-frontseat
  changes).
- lamss3 port (GobySoft/lamss3#23): licence headers, build with `lamss_add_app`, and the example
  `.moos` renamed from the `pSimAcoBeaconRngBrg` block; no change to behaviour.
