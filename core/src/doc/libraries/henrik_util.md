---
title: henrik_util
toolbox: core
kind: library
source: core/src/lib/henrik_util/
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

henrik_util (`lamss_core_henrik_util`, namespace `lamss::core`) is the core part of LAMSS's shared
numerical library **[code]**: moving averages, small vector and matrix routines, complex covariance
matrices with Cholesky factorisation and random realisations, ambient-noise covariance for hydrophones
and vector sensors, Bessel functions, ray tables and a simple refracted-ray model, a random number
generator, and parsers for LAMSS status strings **[code]**. Most of it serves the acoustic
simulators in virtual_ocean and the trackers in sonar **[code]** (see [API](#api) for users).
It has no MOOS interface of its own **[code]**; it links MOOS and MOOS-IvP's `mbutil` for string
helpers and `MOOSTime` **[code]**.

## Interfaces

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

None: the library reads no configuration **[code]**. Callers pass every parameter.

### Files, devices and external programs

- `CEigenRayTable::readEigenRayTable` and `getEigenRayTable` read an ASCII arrivals file: dimension
  tag, frequency, one source depth, receiver depths, ranges, then per receiver the arrivals
  (amplitude, phase in degrees, delay, dip angles, bounce counts) **[code]**. This is the BELLHOP
  `.arr` format **[guess]**.
- Several functions print to standard output (`parse_tgt_stat` prints every string it parses;
  dimension errors in `CovMat`) **[code]**. `cminv`, `cmmul` and `cmvmul` call `exit(EXIT_FAILURE)` on a
  dimension mismatch or singular matrix **[code]**.

## Behaviour

- Matrices in `CmyVectorUtils` (`mmul`, `minv`, `bminv`, `bsolve`) and `CovMat` are flat vectors,
  column-major, with 1-based `indx(i, j)` **[code]**. `CovMat` stores the lower triangle packed row by
  row (`indx_tri`) **[code]**. `cmmul`, `cmvmul`, `cminv` and `transp` use `vector<vector<…>>` with
  0-based indices **[code]**.
- `CMeasAvg`, `CAngleAvg` (degrees) and `CRadianAvg` keep the last `length` values and return their
  mean; the angle versions average sine and cosine **[code]**. `getAvg()` is 0 before the first value
  **[code]**.
- `CovMat::getCholLow`, `realization` and `wn_realization` factorise on first use; if the matrix is
  not positive definite they add white noise (1e-6 of the mean diagonal, then ×10 each try) until it
  is **[code]**.
- `CBessel::bessel_J` and `sphbes_j` interpolate tables built by `initBessel`/`initSphBes` (step π/20)
  and extend the table when called beyond it **[code]**. Calling them before the init function is
  undefined behaviour **[code]**.
- `CRandom` uses the Numerical Recipes `ran2` generator with a fixed seed (-123456), so its sequence is
  the same on every run **[code]**; `gauss_real` instead seeds `std::default_random_engine` from the
  clock on every call **[code]**.

## API

| Header | Main classes and functions | Used by (lamss3) |
|---|---|---|
| `CMeasAvg.h` | `CMeasAvg`, `CAngleAvg`, `CRadianAvg`: moving averages **[code]** | iSerialNMEA, pLamssMissionManager, pGeoTracker, pLamssAEL, pLamssCBF, pLamssTrackManager, uSimPhinsINS |
| `CVectorUtils.h` | `CmyVectorUtils`: vector arithmetic, `interp`, `resample`, `cumsum`, `cumtrapz`, real and complex matrix product, Gauss-Jordan inverse (`minv`, banded `bminv`, `bsolve`, complex `cminv`), complex `cholesky`, `conv`, Hann and Tukey windows, `lin_least_square`, `prune_timeseries`, `NN` (nearest index) **[code]** | MaintainBearing behavior, pGeoTracker, pLamssCBF, uSimModemNetwork, uSimNoiseCovariance, uSimPassiveSonar, uSimTowedArray |
| `CCovMat.h` | `CovMat`: Hermitian covariance matrix, Cholesky factor, `realization` (correlated complex Gaussian noise vector), `scaleCovMat` **[code]** | uSimModemNetwork, uSimNoiseCovariance, uSimPassiveSonar |
| `CNoiseCovariance.h` | `ComputeCovariance`: noise covariance between two hydrophones (`hydCovariance`) or two 4-channel vector sensors (`vsaCovariance`) for a given vertical directionality, by Filon integration; isotropic versions **[code]** | uSimModemNetwork, uSimNoiseCovariance |
| `CBessel.h` | `CBessel`: tabulated cylindrical and spherical Bessel functions **[code]** | uSimNoiseCovariance (and `ComputeCovariance`) |
| `CEigenRays.h` | `CEigenRayTable`: read a ray-arrivals file, nearest-grid lookup `getEigenRays` **[code]** | OptAcommsDepth behavior, uSimModemNetwork, uSimPassiveSonar |
| `CRapRays.h` | `CRapRays`: rays in a linear sound-speed profile; reliable-acoustic-path (RAP) range, depth, delay and footprint, and `rapGeoTrack` **[code]** | OptAcommsDepth behavior, uSimPassiveSonar |
| `CTarget.h` | `CTarget`: simulated target state with tones; `target_state()` dead-reckons it to now **[code]** | uSimModemNetwork, uSimPassiveSonar, uSimTargets |
| `Random.h` | `CRandom`: `ran2`, `gauss_dev`, `gauss_real`, `gauss_cmplx` **[code]** | uSimModemNetwork, uSimNoiseCovariance, uSimPassiveSonar |
| `Measurement.h`, `parse_stat.h` | `parse_measurement`, `parse_brng_stat`, `parse_tgt_stat`, `parse_track_stat`: read `key=value,…` status strings with `MOOSValFromString` **[code]** | pGeoTracker, pLamssTrackManager, uSimTargets |
| `str_num.h` | `str2dbl_vec`, `dbl_vec2str`, `str2int_vec`, `int_vec2str`: comma-separated numbers **[code]** | uSimModemNetwork, uSimNoiseCovariance, uSimPassiveSonar, uSimTargets, uSimTowedArray |

The "used by" column is from the `#include` lines in lamss3 **[code]**. sonar and virtual_ocean have
their own `henrik_util` libraries for the parts only they use **[code]**.

Tests: none exercise the library directly **[code]**. It is built and linked by every app above, and
iSerialNMEA's use of `CMeasAvg` runs in `lamss_core_moos_smoke` **[test]**.

## Known issues and open questions

- `CmyVectorUtils::cholesky` never updates `arg` from `sum`, so every diagonal element is
  `sqrt(a(1,1))` **[code]**; see [GobySoft/lamss3#31](https://github.com/GobySoft/lamss3/issues/31).
  Nothing in lamss3 calls it **[code]**; `CovMat` has its own Cholesky.
- `CBessel::sphbes_j`, extending its table, loops `m <= sphbes_table.size()` and writes one table
  past the end **[code]**. `hydIsotropicCovariance` reaches this when `k·ΔR` exceeds the 40π that
  uSimNoiseCovariance and uSimModemNetwork pass to `initSphBes` **[code]**.
- `CovMat::getCCovMat` loops `i <= num_rcv` over a vector of `num_rcv` rows **[code]**: out of bounds on
  every call. Unused in lamss3 **[code]**.
- `CmyVectorUtils::vpair_sort` tests `done[i]` instead of `done[j]`, so every output element is the
  minimum **[code]**. Unused in lamss3 **[code]**.
- `CmyVectorUtils::cumtrapz` stops one step early: `out[last]` is never written and `out[last-1]`
  includes the last interval **[code]**. Only a commented-out call in uSimTowedArray **[code]**.
- `CEigenRayTable::readEigenRayTable` returns `true` when the file cannot be opened, leaving an empty
  table **[code]**; `getEigenRays` then reads `range[0]` of an empty vector **[code]**.
- `CTarget::setTargetValues` reads `values[0..9]` without checking the size; with fewer than 10
  values, `values.size() - 10` also wraps around **[code]**.
- `CRandom::gauss_real` re-seeds a new engine from the clock on every call **[code]**, so successive
  values are correlated, and identical within one clock tick **[guess]**. It is used for surface
  jitter in uSimModemNetwork **[code]**.
- The `parse_*` functions reuse one string for every integer field; when a key is missing, the field
  gets the previous key's value (e.g. `state` = `node`) **[guess]** (if `MOOSValFromString` leaves
  its output unchanged on a miss, which was not checked).
- `CRapRays::rapFootprint` checks only the last of its three `rapRay` results **[code]**.
- `Random.h` defines macros `IM1`, `NTAB`, `EPS` and others in every file that includes it
  **[code]**. Could they clash with other headers?
- `parse_tgt_stat` truncates `tgt_utc` to whole seconds with `atoi` and prints every string to
  standard output **[code]**. Intended?
- `parse_brng_stat` is declared in both `Measurement.h` and `parse_stat.h` but defined in
  `Measurement.cpp` **[code]**.

## History

- From GobySoft/lamss `lib_henrik_util` (per the plan's source inventory); the imported history shows
  only lamss3 paths. First commit 2011-04-27 by Toby Schneider ("many additions to finalize initial
  migration"); `CMeasAvg` from 2012.
- 114 commits before the port, 2011–2025, mostly by Henrik Schmidt (under several author spellings),
  with Toby Schneider, Andrew Poulsen and a few others. Last change before the port: `gauss_real`
  seeded from the current time (2025-06-30).
- lamss3 port: `CMeasAvg` came first, with the core port (GobySoft/lamss3#9–#12, 2026-10-29); the
  other files followed on 2026-11-03 and 2026-11-04 for the autonomy, sonar and virtual_ocean ports
  (#21–#23) **[guess]** (from the commit messages). Changes: clang-format, namespace `lamss::core`, one
  `parse_stat` shared by the toolboxes, MOOS kept out of the installed link interface, and fixes for
  gcc's `-O2` warnings (variables initialised; `NN` returns the default index when `num` is outside the
  vector). `CLamssNetcdf` is in virtual_ocean and `CTrack` in sonar; the rest of
  `lib_henrik_util` (acoustic model, Kalman filter) was not ported (plan
  [09](../plan/claude/09-toolboxes.md)).
