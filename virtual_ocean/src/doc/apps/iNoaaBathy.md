---
title: iNoaaBathy
toolbox: virtual_ocean
kind: moos-app
source: virtual_ocean/src/bin/iNoaaBathy/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# iNoaaBathy

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

iNoaaBathy reads a gridded bathymetry file (NetCDF, NOAA or GEBCO-style `lat`/`lon`/`z` grid) and
publishes the water depth under the vehicle as `BATHY_Z` at a fixed interval **[code]**. When built
with netsim it also answers `BATHY_REQUEST` with the depths at a list of positions, which is how
netsim's acoustic models get bathymetry along a propagation path **[code]** (the source comment
names `uSimSoundVelocityMSEAS` as the requester **[code]**; in lamss3 the requester would be
netsim's Bellhop front end **[guess]**). It sits beside the vehicle simulator, taking `NAV_X`/`NAV_Y`
**[code]**, and its `BATHY_Z` is read by simulators that need the bottom depth, such as a DVL or
altimeter model **[guess]**.

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `LAT_ORIGIN` | MOOS | double | `iNoaaBathy.cpp`, `OnNewMail` | new local-grid origin latitude **[code]** | re-initialises the app's geodesy, but not the one inside `LamssNetCdf` **[code]**; see open questions |
| `LONG_ORIGIN` | MOOS | double | `iNoaaBathy.cpp`, `OnNewMail` | new local-grid origin longitude **[code]** | as `LAT_ORIGIN` |
| `NAV_X` | MOOS | double, m | `iNoaaBathy.cpp` | vehicle east position in the local grid **[code]** | name matched case-insensitively **[code]**; default 0 until received |
| `NAV_Y` | MOOS | double, m | `iNoaaBathy.cpp` | vehicle north position in the local grid **[code]** | as `NAV_X` |
| `BATHY_REQUEST` | MOOS | string: protobuf `netsim.SVP.protobuf.BathyRequest` (`parse_for_moos`) | `iNoaaBathy.cpp` | request for depths at a list of lat/lon points, with a `request_id` **[code]** | only registered when built with netsim (`LAMSS_VIRTUAL_OCEAN_HAVE_NETSIM`) **[code]**; requests missing required fields are dropped **[code]** |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `BATHY_Z` | MOOS | double, m (negative down) | `iNoaaBathy.cpp` | grid elevation at the vehicle position **[code]**, interpolated from the three nearest grid points **[code]** | every `publish_interval` s **[code]**; NaN outside the grid **[code]**; `-100` over a flat 100 m grid **[test]** |
| `BATHY_RESPONSE` | MOOS | string | `iNoaaBathy.cpp` | protobuf `netsim.SVP.protobuf.BathyResponse` (`serialize_for_moos`): the request's `request_id` and one depth per requested point **[code]** | netsim builds only **[code]** |
<!-- END GENERATED publishes -->

### Configuration

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `LatOrigin` | double |  | yes | global (mission file); origin latitude of the local grid **[code]** |
| `LongOrigin` | double |  | yes | global (mission file); origin longitude of the local grid **[code]** |
| `nc_bathy_file` | string |  | yes | path of the NetCDF bathymetry grid **[code]**; the app exits if it is missing or cannot be opened **[code]** |
| `publish_interval` | double | `5` | no | seconds between `BATHY_Z` publications **[code]** |
<!-- END GENERATED config -->

Example (from `iNoaaBathy.moos`):

```
LatOrigin  = 42.358
LongOrigin = -71.087

ProcessConfig = iNoaaBathy
{
  AppTick    = 4
  CommsTick  = 4

  // NOAA/GEBCO-style grid: lat, lon and a short z (or Band1) variable, negative down
  nc_bathy_file = bathy.nc

  // seconds between BATHY_Z publications
  publish_interval = 5
}
```

### Files, devices and external programs

- Reads `nc_bathy_file` with the NetCDF C API through `lamss::virtual_ocean::LamssNetCdf`
  (`henrik_util`) **[code]**. The file needs 1-D coordinate variables named `lat`/`lon` (or `y`/`x`)
  and a 2-D `short` variable `Band1` or `z`, indexed (lat, lon) **[code]**. The app exits if any
  of these is missing **[code]**.

## Behaviour

- On connecting to the MOOSDB it reads its configuration, initialises the local grid from
  `LatOrigin`/`LongOrigin`, opens the grid file and registers for its variables **[code]**.
  Nothing is published until the file is open **[code]**.
- Each `Iterate` checks whether `publish_count × publish_interval` seconds have passed since start-up;
  if so it converts (`NAV_X`, `NAV_Y`) to latitude/longitude and publishes the depth there as
  `BATHY_Z` **[code]**. The rate is therefore at most `AppTick` **[code]**.
- Depth is a linear interpolation over the triangle of the three grid points nearest the position,
  in local metres **[code]**. Outside the grid the value is NaN **[code]**.
- `BATHY_REQUEST` (netsim builds) stores the request's points; the next `Iterate` looks up each
  point and publishes all depths in one `BATHY_RESPONSE` **[code]**. Only the latest request
  between two iterations is answered **[code]**.
- A `LAT_ORIGIN` or `LONG_ORIGIN` mail re-initialises the geodesy used to turn `NAV_X`/`NAV_Y` into
  latitude/longitude **[code]**.

## Running it

- No mission component launches it yet.
- Standalone, in `virtual_ocean/src/bin/iNoaaBathy/` with a grid file `bathy.nc`: run `MOOSDB
  iNoaaBathy.moos`, then `iNoaaBathy iNoaaBathy.moos`.
- Tested by `lamss_virtual_ocean_moos_smoke`: on a flat 100 m grid written by
  `make_flat_bathy`, the app connects and publishes `BATHY_Z` containing `-100` **[test]**.
  `BATHY_REQUEST` is not tested.

## Known issues and open questions

- `LAT_ORIGIN` and `LONG_ORIGIN` are handled independently in one mail batch: a batch with only one
  of them initialises the geodesy with 0 for the other (`iNoaaBathy.cpp`, `OnNewMail`) **[code]**.
  Should the app keep the last value of each, as pViewMessage does?
- The geodesy inside `LamssNetCdf`, which turns latitude/longitude into the local metres used for
  interpolation, is never moved to the new origin **[code]**. Interpolation is done relative to
  the configured origin, which is harmless as long as the grid points and the query use the same
  origin **[guess]**. Is that the intent?
- `ReadConfiguration` runs in `OnConnectToServer`, so a reconnection re-initialises `LamssNetCdf`
  and reopens the file without closing it **[code]**. Should configuration move to `OnStartUp`?
- `start_time_` is set in `OnStartUp` and passed to `initialize_noaa_nc`, which ignores it
  **[code]**.
- The header declares unused members (`get_bathy`, `ncid_`, the `min_lat_val*`/`min_lon_val*`
  fields, `lat_vec_`, `lon_vec_`, `bathy_response_sent_`) **[code]**. Leftovers from iMseasBathy, of
  which this app began as a copy **[guess]**.
- Missing `LatOrigin`/`LongOrigin` returns false from `ReadConfiguration`, which the caller ignores;
  the app then fails later when opening an empty file name **[code]**.
- `BATHY_Z` is published with the sign of the grid (negative down for NOAA grids) **[code]**. Do
  consumers expect a positive water depth?

## History

- From GobySoft/lamss `src/moos/iNoaaBathy`; first commit 2018-03-09 (Henrik Schmidt), "added new
  iNoaaBathy for reading NOAA bathymetry netcdf files". It began as a copy of iMseasBathy, which is
  not imported.
- 21 commits before the port, mostly by Henrik Schmidt, with Toby Schneider (netsim namespace
  updates 2020, `goby::moos::CMOOSGeodesy` 2019). Dynamic UTM origin (`LAT_ORIGIN`/`LONG_ORIGIN`)
  added 2021-06-08 and registered 2023-02-09.
- lamss3 port (GobySoft/lamss3#23): netsim made optional, so `BATHY_REQUEST`/`BATHY_RESPONSE`
  exist only in netsim builds; `LamssNetCdf` moved to `lamss::virtual_ocean`; the origin locals in
  `OnNewMail` initialised to 0 (they were uninitialised); new `iNoaaBathy.moos` example.
