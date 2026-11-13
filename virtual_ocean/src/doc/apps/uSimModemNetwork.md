---
title: uSimModemNetwork
toolbox: virtual_ocean
kind: goby-moos-app
source: virtual_ocean/src/bin/uSimModemNetwork/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# uSimModemNetwork

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

uSimModemNetwork models the acoustic channel between pairs of acoustic-modem nodes **[code]**. It
keeps an eigenray table from iBellhop (netsim) for every ordered pair of configured nodes, using
their positions from `NODE_REPORT` **[code]**. On request it answers with the channel's impulse
response (`IMPULSE_REQUEST` → `IMPULSE_RESPONSE`) or with predicted link performance (SNR,
multipath, MAE) for candidate receiver depths (`PERFORMANCE_REQUEST` → `PERFORMANCE_RESPONSE`)
**[code]**. The impulse responses feed a modem signal simulator (uSimModemSignal in the old
repository, not in lamss3) **[guess]**. The performance responses feed depth-adaptation
autonomy (pDepthAdapt, BHV_OptAcommsDepth) according to the commit history **[guess]**. It is
built only where netsim is installed **[code]**.

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `LAT_ORIGIN` | MOOS | double | `handle_lat_origin` | new local-grid origin latitude **[code]** | NaN ignored; geodesy re-initialised in `loop()` **[code]** |
| `LONG_ORIGIN` | MOOS | double | `handle_lon_origin` | new local-grid origin longitude **[code]** | NaN ignored |
| `BELLHOP_RESPONSE` | MOOS | string: serialised `netsim.protobuf.iBellhopResponse` | `handle_bellhop_response` | arrival file for one of this app's ray-tracing requests **[code]** | matched on `requestor == "uSimModemNetwork"` and request number **[code]** |
| `SVP_RESPONSE` | MOOS | string: serialised `netsim.SVP.protobuf.Response` | `handle_svp_response` | sound-speed profile; id 0 initialises all contacts **[code]** |  |
| `NODE_REPORT` | MOOS | string: `NAME=...,UTC_TIME=...,X=...,Y=...,DEPTH=...,SPD=...,HDG=...[,PITCH=...]` | `handle_node_report` | position of a configured `node` **[code]** | names compared case-insensitively; lower-case keys and `TIME`/`DEP` accepted; older reports ignored **[code]** |
| `NETSIM_NAV_UPDATE` | MOOS | protobuf | `handle_netsim_nav_update` | `netsim.protobuf.NavUpdate` from the netsim side, republished as a `NODE_REPORT` named by the modem TCP port **[code]** |  |
| `IMPULSE_REQUEST` | MOOS | string: serialised `netsim.protobuf.ImpulseRequest` | `handle_impulse_request` | request for the impulse response from source to receiver (names or modem ids) **[code]** | always answered, empty if no ray table yet **[code]** |
| `PERFORMANCE_REQUEST` | MOOS | string: serialised `netsim.protobuf.ObjFuncRequest` | `handle_performance_request` | request for predicted link performance from a contact to receivers at given depths **[code]** |  |
| `TRANSMIT_TEST` | MOOS | double | `handle_transmit_test` | index of a contact pair; publishes a test `TRANSMIT_REQUEST` **[code]** | for testing the signal simulator **[guess]** |
| `AMBIENT_NL_DEPTH` | MOOS | string: `depths=...,NL=...` or `delta=...` | `handle_ambient_nl_depth` | noise level versus depth **[code]** |  |
| `NOISE_VERT_DIR` | MOOS | string: `theta=...`/`elevation=...` with `S=...`/`delta=...` | `handle_noise_vert_dir` | vertical noise directionality **[code]** | used for array noise covariances **[code]** |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `TRANSMIT_REQUEST` | MOOS | string | `SimModemNetwork.cpp` | serialised `lamss.virtual_ocean.TransmitRequest` with `test_replica` **[code]** | only on `TRANSMIT_TEST` |
| `NODE_REPORT` | MOOS | string | `SimModemNetwork.cpp` | node report built from `NETSIM_NAV_UPDATE`, `NAME=<modem tcp port>` **[code]** |  |
| `IMPULSE_RESPONSE` | MOOS | string | `SimModemNetwork.cpp` | serialised `netsim.protobuf.ImpulseResponse`: per ray the per-element delays, amplitude, Doppler, elevation and bounces; range; noise level; sound speeds; optional covariance file **[code]** |  |
| `PERFORMANCE_RESPONSE` | MOOS | string | `SimModemNetwork.cpp` | serialised `netsim.protobuf.ObjFuncResponse`: per receiver range, depth, SNR, multipath metric, TL, NL, delay, MAE and `metric_db` **[code]** | `metric_db` follows `optimize` **[code]** |
| `BELLHOP_REQUEST` | MOOS | string: serialised `netsim.protobuf.iBellhopRequest` | `SimModemNetwork.cpp` | ray-tracing request for a node pair **[code]** | queued; one sent per app tick, last in first out **[code]** |
| `SVP_REQUEST` | MOOS | string: serialised `netsim.SVP.protobuf.Request` | `SimModemNetwork.cpp` | sound-speed profile request **[code]** | `mseas_svp_updates` only; one outstanding at a time **[code]** |
| `PERFORMANCE_REQUEST` | MOOS | string | `SimModemNetwork.cpp` | self-test request **[code]** | in `test_performance_request()`, which is disabled (`if (false)`) **[code]** |
<!-- END GENERATED publishes -->

### Configuration

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `common` | GobyMOOSAppConfig |  | no | Configuration common to all Goby MOOS applications |
| `base_dir_name` | string |  | no | prefix of covariance files (`<base_dir_name><node>_<time>.cov`) **[code]** |
| `acomms_frequency` | double | `10000` | no | modem centre frequency (Hz), used for ray tracing **[code]** |
| `acomms_bandwidth` | double | `2000` | no | modem bandwidth (Hz) **[code]** |
| `test_replica` | string | `replica.dat` | no | replica path in `TRANSMIT_TEST` requests **[code]** |
| `noise_level` | double | `40` | no | ambient noise level (dB) without a noise profile **[code]** |
| `max_number_bounces` | int32 | `1` | no | bounce limit when reading arrival files **[code]** |
| `raytracing_interval` | double | `30` | no | seconds before re-requesting ray tracing without a table; twice this before giving up on an answer **[code]** |
| `raytracing_range_box` | double | `20` | no | range step of the ray-tracing grid (m); re-trace when the range changes by twice this **[code]** |
| `mseas_svp_updates` | bool | `false` | no | request sound-speed profiles for each pair **[code]** |
| `mseas_update_interval` | double | `60` | no | seconds between SVP requests per pair **[code]** |
| `svp_range_resolution` | double | `200` | no | range step of SVP requests (m) **[code]** |
| `svp_depth_resolution` | double | `20` | no | depth step of SVP requests (m) **[code]** |
| `node` | repeated string |  | no | names of the modem nodes; every ordered pair becomes a contact **[code]** |
| `array_element` | repeated ArrayElement |  | no | receiving-array element offsets (`x_offset`, `y_offset`, `z_offset`, m); more than one enables array noise covariances **[code]** |
| `depth_performance_prediction` | bool | `false` | no | ray-trace over `min_depth`–`max_depth` instead of around the receiver depth **[code]** |
| `min_depth` | double | `2` | no | **[code]** |
| `max_depth` | double | `200` | no | **[code]** |
| `num_depths` | int32 | `100` | no | **[code]** |
| `bounce_penalty` | int32 | `0` | no | dB added to the multipath metric per bounce of the strongest ray **[code]** |
| `multipath_penalty` | int32 | `0` | no | exponent of the penalty for rays arriving within 20 ms of the strongest **[code]** |
| `optimize` | ModemPerformanceMetric | `SNR` | no | `SNR`, `MAE` or other (multipath metric) as `metric_db` **[code]** |
| `num_ff_tabs` | int32 | `21` | no | feed-forward equaliser taps for the MAE prediction **[code]** |
| `num_fb_tabs` | int32 | `20` | no | feedback equaliser taps for the MAE prediction **[code]** |
| `nbaud` | int32 | `18` | no | samples per symbol for the MAE prediction **[guess]** |
| `source_level` | double | `183` | no | modem source level (dB) for SNR **[code]** |
| `isotropic_noise` | bool | `false` | no | not read: the isotropic model is always off **[code]** |
| `surface_rms_roughness` | double | `0` | no | surface roughness for the MAE prediction **[code]** |
| `surface_roughness_loss` | double | `0` | no | coherence loss per surface bounce for the MAE prediction **[code]** |
| `modem_id_lookup_path` | string | `modemidlookup.txt` | no | Goby modem id lookup file, to convert ids in requests to names **[code]** |
<!-- END GENERATED config -->

There is no example `.moos` file in the source directory; `uSimModemNetwork -e` prints one
(checked by `lamss_virtual_ocean_moos_smoke` when netsim is found **[test]**). The smoke test uses:

```
ProcessConfig = uSimModemNetwork
{
  base_dir_name: "<directory>/"
}
```

### Files, devices and external programs

- Reads `modem_id_lookup_path` at start-up **[code]** (the smoke test starts without one
  **[test]**).
- Reads the Bellhop arrival files named in `BELLHOP_RESPONSE` **[code]**.
- Writes `<base_dir_name><node>_<time>.cov` for each impulse response when more than one
  `array_element` is configured: the same layout as uSimNoiseCovariance's file but with float
  matrix entries **[code]**. Exits if it cannot be opened **[code]**.

## Behaviour

- At start-up a contact is made for every ordered pair of `node`s **[code]**.
- Each app tick sends one queued `BELLHOP_REQUEST` **[code]**. A slower loop (every
  `min(mseas_update_interval / pairs, 10)` s) sends one queued SVP request and, for every pair
  whose two nodes have reports, reads the eigenrays and the receiver's noise level **[code]**.
- Node positions are dead-reckoned from the last report to the current time **[code]**.
- Ray tracing is requested for a range–depth box around the receiver, widened by the range rate,
  with 3500 beams between ±60° **[code]**. Request ids are random **[code]**.
- Impulse responses: the eigenrays at the receiver, per element of the array, with Doppler from
  both nodes' speeds and headings and the receiver sound speed from the SVP **[code]**.
- Performance responses: TL, SNR (`source_level - TL - NL`), a multipath metric, mean delay, and
  optionally the MAE of an equaliser predicted from the channel and noise **[code]**.

## Running it

- No mission component launches it yet.
- Needs netsim's iBellhop (for `BELLHOP_RESPONSE`) and node reports for the configured nodes
  **[code]**. Without them it answers impulse requests with empty responses **[code]**.
- Standalone: `MOOSDB` and `uSimModemNetwork <file>.moos` with the block above and `node:` entries.
- Tested by `lamss_virtual_ocean_moos_smoke`, only when netsim is found: `-e` prints a
  configuration and the app connects to the MOOSDB **[test]**. No node, Bellhop or request is
  simulated.

## Known issues and open questions

- `getNodeInfo` dead-reckons with `heading · π / 80` instead of `π / 180` **[code]**, so
  extrapolated positions are wrong whenever a node moves **[guess]**.
- `last_slow_loop_t_` is never initialised **[code]**, as in pLamssAEL
  ([GobySoft/lamss3#32](https://github.com/GobySoft/lamss3/issues/32)).
- `Nodes[i].utc` is uninitialised before the first report but compared with it **[code]**; a
  garbage value could block all updates for that node.
- Impulse and MAE computations index `svp_response.water_column(size - 1)` **[code]**; with no SVP
  response yet that is index -1.
- A contact's `noise_level` is set only in the slow loop, but an impulse request can be answered
  before that **[code]**.
- `ownship_pitch` is never set and is used in the array geometry and Doppler **[code]**.
- The covariance file header writes `minimum_frequency` twice **[code]**, as in
  uSimNoiseCovariance. With `acomms_frequency + bandwidth/2 ≥ fs/2`, `indx_last = nfft/2 + 1`
  indexes past `noise_covar` **[code]**.
- `compute_tl` sets TL to NaN when there are no rays but does not return, so the result becomes
  300 dB **[code]**. Which was intended?
- In a performance response `node_id` is copied when the *contact* was given by id, not when the
  receiver was **[code]**.
- `isotropic_noise` is in the configuration but never read **[code]**.
- `test_performance_request()` has hard-coded node names and is disabled; can it be removed?
- `pi = 3.14159` is declared but `M_PI` is used **[code]**.

## History

- From GobySoft/lamss-shared `src/moos/uSimModemNetwork` (imported with its history at
  `043f556`). It began as a copy of uSimPassiveSonar. First commit 2017-07-12 (Henrik Schmidt,
  "Added uSimModemNetwork"); 97 commits before the port, by Henrik Schmidt and Toby Schneider.
- Main changes in the old history: netsim `NETSIM_NAV_UPDATE` and request ids (2017–2018), modem
  arrays (2018), vertical noise directionality and covariance files (2019), performance prediction
  with SNR, multipath and MAE (2020–2021), dynamic origin (2021).
- lamss3 port (Phase 2, GobySoft/lamss3#21–#23): built only where netsim is installed; noise level
  and `max_tau`/`max_power_bounces` initialised; braces around the `if (false)` block; newmat
  include and the unbuilt `SimModemNetworkMain.cpp` removed; MIT/GPL-3.0-or-later headers.
