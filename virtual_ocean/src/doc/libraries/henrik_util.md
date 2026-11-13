---
title: henrik_util
toolbox: virtual_ocean
kind: library
source: virtual_ocean/src/lib/henrik_util/
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

virtual_ocean's henrik_util (`lamss::virtual_ocean_henrik_util`) is one class, `LamssNetCdf`, that
reads ocean-environment NetCDF files: bathymetry (NOAA, GEBCO), sound-speed or temperature and
salinity profiles (GDEM, MSEAS, HYCOM), and EOF representations of the sound-speed profile
**[code]**. It interpolates them to a latitude, longitude, depth and time **[code]**. It is the
environment back end of the virtual ocean: in lamss3 only iNoaaBathy uses it **[code]**; the HYCOM,
MSEAS, GDEM and EOF readers served simulators not ported yet (uSVP_Oceanmodel and an ADCP
simulator are named in the history) **[guess]**. The rest of the old `lib_henrik_util` (vector
utilities, eigenrays, RAP ray tracing, covariances) is in core's henrik_util **[code]**.

## Interfaces

The library has no MOOS interface **[code]**.

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

None; callers pass file names and the local origin **[code]**.

### Files, devices and external programs

- Opens NetCDF files read-only with `nc_open` (NetCDF C API) **[code]**. HYCOM input may be one
  `.nc` file or a directory of them, sorted in natural order (`strnatcmp.hpp`, zlib licence)
  **[code]**.
- Variable names: NOAA and GEBCO `lat`/`lon` (or `y`/`x` in the newer NOAA format); MSEAS `tlat`,
  `tlon`, `level`, `time`; HYCOM `lat`, `lon`, `depth`, `time`; GDEM names found from the file
  **[code]**.
- Prints progress and errors to standard output, and calls `exit(EXIT_FAILURE)` on most NetCDF
  errors **[code]**.

## Behaviour

- Each source has an `initialize_*_nc(file, lat_origin, lon_origin, moos_start, fixed_start)` that
  opens the file, reads the grid and sets the local origin **[code]**.
- Values at a point come from the three nearest grid points (`closest_lat_lon`), interpolated on
  the plane through them (`interp_triangle`), in local metres **[code]**.
- Time: the file's `units` attribute (e.g. `hours since 2000-01-01 00:00:00`) is parsed by
  `extract_date` and converted with a MATLAB-style `datenum` **[code]**. With `fixed_start` the
  first model time is mapped to the MOOS start time instead **[code]**.
- EOF profiles: base profile plus weighted EOFs, interpolated to the requested depths **[code]**.
  Weights can be changed one at a time, or modulated by a plane internal wave
  (`internal_wave_eof_weight`) **[code]**.

## API

All in `namespace lamss::virtual_ocean`, header
`lamss/virtual_ocean/henrik_util/CLamssNetcdf.h` **[code]**.

| Function | What it does |
|---|---|
| `initialize_noaa_nc`, `get_noaa_bathy(lat, lon)` | NOAA bathymetry (short integers) at a point, NaN outside the grid **[code]** |
| `initialize_gebco_nc`, `get_gebco_bathy(lat, lon)` | GEBCO bathymetry at a point **[code]** |
| `initialize_gdem_nc`, `get_gdem_profile(lat, lon, depths, month)` | GDEM climatology profile for a month **[code]** |
| `initialize_mseas_nc`, `get_mseas_profile(...)`, `get_mseas_bathy` | MSEAS model on its skewed grid: temperature and salinity at a time; bathymetry **[code]** |
| `initialize_hycom_nc(file_or_dir, "ts"/"uv", ...)`, `get_hycom_profile(...)` | HYCOM temperature and salinity (or currents) at a time; longitudes 0–360 or ±180 matched to the file **[code]** |
| `initialize_eof_nc`, `get_eof_profile(lat, lon, depths)` | sound-speed profile from EOFs (position not used) **[code]** |
| `handle_eof_weight_update_message`, `current_eof_weights`, `internal_wave_eof_weight` | change and read the EOF weights **[code]** |
| `datenum`, `extract_date`, `init_timebase`, `init_geodesy` | time and coordinate helpers **[code]** |

Used by: iNoaaBathy (`initialize_noaa_nc`, `get_noaa_bathy`) **[code]**. uSimPassiveSonar and
uSimModemNetwork link the library but include none of its headers **[code]**.

Tested by `lamss_virtual_ocean_moos_smoke` through iNoaaBathy: a flat 100 m NOAA-format grid
returns `BATHY_Z = -100` **[test]**.

## Known issues and open questions

- `datenum`: month days fall through and minutes and seconds are swapped
  ([GobySoft/lamss3#27](https://github.com/GobySoft/lamss3/issues/27)).
- `extract_date` accepts a date with only three numbers (e.g. `days since 2000-01-01`), but
  `datenum` always reads six elements, past the end of the vector **[code]**.
- `utc_vec_` and `time_vec_` are `float`; Unix times near 1.7e9 then have a resolution of about
  two minutes **[code]**. Should they be `double`?
- `init_timebase` sets `utc_vec_[i] = start + (i - 1) · delta`, so the first entry is one step
  before the start **[code]**. Off by one?
- `initialize_hycom_nc` with a regular file that is not `.nc` leaves `files` empty and then opens
  `files[0]` **[code]**.
- Many members (`nc_is_open_`, `geodesy_initialized`, `datenum_start`) are initialised only by the
  `initialize_*` functions; the class has no constructor **[code]**.
- Library code that calls `exit()` on a bad file stops the whole app. Should errors be returned
  instead?
- Are the GDEM, MSEAS, HYCOM and EOF readers still needed, and which simulators will use them?

## History

- From GobySoft/lamss `src/lib/lib_henrik_util` (imported at `3186a63`), with `CLamssNetcdf`'s
  earlier name `CGdemNetcdf`. First commit 2018-06-11 (Henrik Schmidt, "added CGdemNetcdf to
  lib_henrik_util"); about 65 commits before the port, mostly by Henrik Schmidt, with novitzky,
  Toby Schneider, Cara Oehlert, csa0301, oviquezr and Eeshan Bhatt.
- Main changes in the old history: HYCOM, MSEAS and GDEM readers (2018), EOF profiles and weight
  updates (2019), new NOAA bathymetry format and internal waves (2020), HYCOM directories and
  natural sort (2022), GEBCO (2022).
- lamss3 port (Phase 2, GobySoft/lamss3#23): built as `lamss_virtual_ocean_henrik_util` in
  `namespace lamss::virtual_ocean`; CRapRays and CEigenRays moved to core's henrik_util;
  `strnatcmp.hpp` kept private with its zlib notice; NetCDF statuses initialised; explicit
  `[[fallthrough]]` in `datenum` (behaviour unchanged); `<algorithm>` and `<iomanip>` included;
  MIT/GPL-3.0-or-later headers.
