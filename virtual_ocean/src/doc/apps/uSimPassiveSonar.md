---
title: uSimPassiveSonar
toolbox: virtual_ocean
kind: goby-moos-app
source: virtual_ocean/src/bin/uSimPassiveSonar/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# uSimPassiveSonar

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

uSimPassiveSonar generates simulated passive sonar data: frames of time series for every element
of a hydrophone (or vector-sensor) array, written to binary files, with ambient noise, broadband
and tonal signals from simulated targets, and pulses from simulated pingers **[code]**. It stands
in for the array's acquisition system, so that the beamforming and tracking chain can run on
simulated data in the same file format **[guess]**. Its inputs are the array geometry from
uSimTowedArray (`ARRAY_*`), target states from uSimTargets (`TGT_<n>_SIM_STATE`), pinger commands
(`LAMSS_PINGER_COMMAND_IN`), and optionally eigenray tables from iBellhop (netsim) and noise
covariances from uSimNoiseCovariance **[code]**. Each new frame is announced with `SONAR_FILENAME`
and `SONAR_FILE_NUM` **[code]**.

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `LAT_ORIGIN` | MOOS | double | `handle_lat_origin` | new local-grid origin latitude **[code]** | NaN ignored; geodesy re-initialised in `loop()`, pinger positions recomputed **[code]** |
| `LONG_ORIGIN` | MOOS | double | `handle_lon_origin` | new local-grid origin longitude **[code]** | NaN ignored |
| `SONAR_CONTROL` | MOOS | string: `ON` or other | `handle_sonar_control` | `ON` (any case) starts frame generation, anything else stops it **[code]** | off at start-up **[code]** |
| `AEL_HEADING` | MOOS | double | `handle_ael_heading` | array heading (degrees), for receiver Doppler **[code]** | from uSimTowedArray **[code]** |
| `AEL_PITCH` | MOOS | double | `handle_ael_pitch` | array pitch, for receiver Doppler **[code]** |  |
| `NAV_SPEED` | MOOS | double | `handle_nav_speed` | receiver speed, for Doppler **[code]** |  |
| `ARRAY_X` | MOOS | string: comma-separated doubles | `handle_array_x` | element x **[code]** | count must equal `num_array_elements`, otherwise the app exits **[code]** |
| `ARRAY_Y` | MOOS | string: comma-separated doubles | `handle_array_y` | element y **[code]** | as `ARRAY_X` |
| `ARRAY_Z` | MOOS | string: comma-separated doubles | `handle_array_z` | element z; depth is `-ARRAY_Z` **[code]** | as `ARRAY_X` |
| `ARRAY_DIR_X` | MOOS | string: comma-separated doubles | `handle_array_dir_x` | element directivity vectors **[code]** | elements with a zero vector are omnidirectional **[code]** |
| `ARRAY_DIR_Y` | MOOS | string: comma-separated doubles | `handle_array_dir_y` | as `ARRAY_DIR_X` |  |
| `ARRAY_DIR_Z` | MOOS | string: comma-separated doubles | `handle_array_dir_z` | as `ARRAY_DIR_X` |  |
| `ARRAY_HEADING` | MOOS | string: comma-separated doubles | `handle_array_heading` | element headings (degrees), for VSA projection and NAS files **[code]** |  |
| `ARRAY_PITCH` | MOOS | string: comma-separated doubles | `handle_array_pitch` | element pitches (radians) **[code]** | a change of the mean by 5° triggers a new `COVARIANCE_REQUEST` **[code]** |
| `TARGET_SIM_LIST` | MOOS | string: comma-separated target numbers | `handle_target_sim_list` | the simulated targets **[code]** | from uSimTargets; subscribes to `TGT_<n>_SIM_STATE` for each new target **[code]** |
| `AMBIENT_NL_DEPTH` | MOOS | string: `depths=...,NL=...` or `delta=...` | `handle_ambient_nl_depth` | noise level versus depth **[code]** |  |
| `NOISE_VERT_DIR` | MOOS | string: `theta=...`/`elevation=...` with `S=...`/`delta=...` | `handle_noise_vert_dir` | vertical noise directionality **[code]** | only switches correlated noise on; the directionality itself is used by uSimNoiseCovariance **[code]** |
| `COVARIANCE_FILE` | MOOS | string: path | `handle_covariance_file` | noise covariance file to load **[code]** | used only after `NOISE_VERT_DIR` and the first frame **[code]** |
| `SVP_RESPONSE` | MOOS | string: serialised `netsim.SVP.protobuf.Response` | `handle_svp_response` | sound-speed profile for a contact **[code]** | netsim builds only **[code]** |
| `VEHICLE_NAME` | MOOS | string | `handle_vehicle_name` | own name, upper-cased into RAY file names **[code]** | default `me` **[code]** |
| `SOUND_SPEED` | MOOS | double | `handle_sound_speed` | sound speed for Doppler **[code]** | default `svp_surface` **[code]** |
| `SOUND_SPEED_GRAM` | MOOS | double | `handle_sound_speed` | as `SOUND_SPEED` **[code]** |  |
| `LAMSS_PINGER_COMMAND_IN` | MOOS | string: serialised `lamss.virtual_ocean.LAMSS_PINGER_COMMAND` | `handle_pinger_command` | create, update or switch off pinger n **[code]** | DCCL message, so it can arrive over acomms **[guess]** |
| `var` (built at run time) | MOOS |  | `handle_target_sim_state`, `handle_target_arrival_file`, `handle_pinger_arrival_file` | `TGT_<n>_SIM_STATE` (target state from uSimTargets), `TGT_<n>_SIM_ARR_FILE` and `PINGER_<n>_SIM_ARR_FILE` (Bellhop arrival files) **[code]** | name built at run time; the `ARR_FILE` ones only with `ray_acoustics` **[code]** |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `TARGET_XPOS` | MOOS | double | `SimPassiveSonar.cpp` | x of the last target in the list **[code]** | a code comment says it was meant to rotate through targets **[code]** |
| `TARGET_YPOS` | MOOS | double | `SimPassiveSonar.cpp` | y of the last target **[code]** |  |
| `NODE_REPORT` | MOOS | string | `SimPassiveSonar.cpp` | node report for each pinger, `NAME=pinger_<n>_sim,TYPE=track,...` **[code]** | on each pinger command and before each repeated ping **[code]**; for display and tracking cues **[guess]** |
| `SONAR_DIR` | MOOS | string | `SimPassiveSonar.cpp` | `base_dir_name`, after each frame **[code]** |  |
| `SONAR_FILE_NUM` | MOOS | double | `SimPassiveSonar.cpp` | frame number just written **[code]** |  |
| `SVP_REQUEST` | MOOS | string: serialised `netsim.SVP.protobuf.Request` | `SimPassiveSonar.cpp` | request for the sound-speed profile between array and contact **[code]** | queued, one sent per slow-loop period **[code]**; netsim builds only |
| `COVARIANCE_REQUEST` | MOOS | string | `SimPassiveSonar.cpp` | `nfft=N,sampling_frequency=F` for uSimNoiseCovariance **[code]** | when correlated noise is on and the array pitch changed by 5°, or after NaNs in the noise **[code]** |
| `SONAR_FILENAME` | MOOS | string: path | `SimPassiveSonar.cpp` | the acoustic file just written **[code]** |  |
| `BELLHOP_REQUEST` | MOOS | string | `SimPassiveSonar.cpp` | serialised `netsim.protobuf.iBellhopRequest` for a contact's eigenrays **[code]** | `ray_acoustics` only **[code]** |
<!-- END GENERATED publishes -->

### Configuration

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `common` | GobyMOOSAppConfig |  | no | Configuration common to all Goby MOOS applications |
| `ray_acoustics` | bool | `false` | no | eigenrays from iBellhop (netsim builds only; the app stops at start-up otherwise) **[code]** |
| `rap_acoustics` | bool | `false` | no | analytic ray tracing in a linear profile (core's `CRapRays`); with both false, cylindrical spreading **[code]** |
| `svp_surface` | double | `1430` | no | surface sound speed (m/s) **[code]** |
| `svp_gradient` | double | `0.016` | no | sound-speed gradient ((m/s)/m) for `rap_acoustics` **[code]** |
| `max_rap_depth` | double | `5000` | no | maximum depth for `rap_acoustics` **[code]** |
| `water_depth` | double | `5000` | no | water depth (m), for spreading and `rap_acoustics` **[code]** |
| `bottom_loss` | double | `0.5` | no | attenuation in the cylindrical-spreading model, applied as `exp(-bottom_loss/8686 · r)` (so dB/km) **[code]** |
| `noise_level` | double | `50` | no | ambient noise spectral level (dB) **[code]** |
| `array_type` | ArrayTypeEnum | `ARRAY_TYPE_HYD` | no | `ARRAY_TYPE_VSA` writes pressure plus three velocity channels; `HYD` and `DURIP` are the same **[code]** |
| `max_number_bounces` | int32 | `1` | no | bounce limit when reading Bellhop arrival files **[code]** |
| `num_array_elements` | int32 |  | yes | number of elements **[code]** |
| `sampling_frequency` | double |  | yes | Hz **[code]** |
| `num_samples` | int32 |  | yes | samples per frame; frame length `num_samples / sampling_frequency` **[code]** |
| `hydrophone_gain` | double | `0` | no | dB applied to the pressure samples **[code]** |
| `accelerometer_gain` | double | `0` | no | dB applied to the vector channels **[code]** |
| `base_dir_name` | string |  | yes | output directory **[code]** |
| `acoustic_filename_prefix` | string | `HYD` | no | acoustic file prefix **[code]** |
| `acoustic_filename_suffix` | string | `.DAT` | no | suffix for acoustic and RAY files **[code]** |
| `nonacoustic_filename_prefix` | string | `NAS` | no | non-acoustic (array state) file prefix **[code]** |
| `nonacoustic_filename_suffix` | string | `.DAT` | no | non-acoustic file suffix **[code]** |
| `svp_updates` | bool | `false` | no | request sound-speed profiles (`SVP_REQUEST`) for Bellhop **[code]** |
| `svp_update_interval` | double | `60` | no | seconds between SVP requests per contact; also sets the slow-loop period **[code]** |
| `svp_range_resolution` | double | `200` | no | range step of the SVP request (m) **[code]** |
| `svp_depth_resolution` | double | `20` | no | depth step of the SVP request (m) **[code]** |
| `sync_path` | string | `` | no | if set, each file is copied there with `rsync` **[code]** |
| `sync_password` | string |  | no | put into `RSYNC_PASSWORD` on the `rsync` command line **[code]** |
| `surface_rms_roughness` | double | `0` | no | read but not used **[code]** |
| `surface_roughness_loss` | double | `0` | no | read but not used **[code]** |
| `bottom_rms_roughness` | double | `0` | no | read but not used **[code]** |
| `bottom_roughness_loss` | double | `0` | no | read but not used **[code]** |
| `replica_path` | string | `./` | no | prefix of `pinger_<n>_replica` files for `WAVEFORM_FILE` pingers **[code]** |
| `coherent_sources` | bool | `false` | no | targets share a random phase and get per-target magnitude and phase weights from `source_weight_file` **[code]** |
| `source_weight_file` | string | `source_weights.bin` | no | binary weights: frequency count, start, step, source count, then (magnitude, phase) per source and frequency **[code]** |
<!-- END GENERATED config -->

Example (from `uSimPassiveSonar.moos`):

```
ProcessConfig = uSimPassiveSonar
{
  water_depth: 100           # used for spherical spreading (m)
  noise_level: 60            # dB re 1 uPa
  ray_acoustics: false       # true: Bellhop (iBellhop, netsim builds only)
  rap_acoustics: false       # RAP raytracing (c(z) linear); both false: cylindrical
                             # spreading and bottom loss

  array_type: ARRAY_TYPE_DURIP
  num_array_elements: 32
  sampling_frequency: 4000   # Hz
  num_samples: 8000          # per file
  hydrophone_gain: 180       # dB, conversion from Pascals to float

  # files are [prefix][frame number][suffix] under base_dir_name
  acoustic_filename_prefix: "ACO"
  acoustic_filename_suffix: ".DAT"
  nonacoustic_filename_prefix: "NAS"
  nonacoustic_filename_suffix: ".DAT"
  base_dir_name: "./"
}
```

### Files, devices and external programs

Written under `base_dir_name` for each frame (frame number `%09d`) **[code]**:

- `<acoustic prefix><frame><suffix>`: float32, native byte order, element by element,
  `num_samples` pressure samples each; for VSA each element's pressure is followed by its x, y and
  z channels **[code]**.
- `HYD<frame><suffix>`: pressure only, VSA arrays only **[code]**.
- `<nonacoustic prefix><frame><suffix>`: text, time, frame number, sensor count, element
  heading/pitch/depth and positions **[code]**; the layout follows MATLAB's `SealabDataSim2.m`
  per a code comment **[code]**.
- `<nonacoustic prefix>_verbose_<frame><suffix>`: the same with labels and target information
  **[code]**.
- `<VEHICLE_NAME>_<source>_RAY<frame><suffix>`: binary eigenrays per element (delay, amplitude,
  phase, bounces, angles) for each target and pinger **[code]**.

Read **[code]**: the covariance file named in `COVARIANCE_FILE` (doubles, see uSimNoiseCovariance),
`source_weight_file` with `coherent_sources`, `<replica_path>pinger_<n>_replica` (float32) for
`WAVEFORM_FILE` pingers, and Bellhop arrival files named in `*_SIM_ARR_FILE`. The app exits if an
output file or a read file cannot be opened **[code]**.

Runs `rsync <file> <sync_path>/` through `system()` for every file when `sync_path` is set
**[code]**.

## Behaviour

- Nothing is generated until `SONAR_CONTROL=ON` and `ARRAY_X/Y/Z` have arrived **[code]**. While
  on, a frame is produced whenever `num_samples / sampling_frequency` seconds have passed; frame
  times advance by exactly that interval unless the app falls more than 10 % behind, when they
  restart from the current second **[code]**.
- Each frame **[code]**:
  1. noise: white noise at the level from `AMBIENT_NL_DEPTH` (or `noise_level`) at the array depth;
     with `NOISE_VERT_DIR`, correlated noise from the last covariance file **[code]**. NaNs in the
     noise cause a new `COVARIANCE_REQUEST` **[code]**;
  2. targets: for each target in `TARGET_SIM_LIST`, a broadband band (`freq ± bw/2` at its SPL)
     plus tones, each element delayed by the eigenrays to the target, added in the frequency domain
     with a random phase per target **[code]**;
  3. pingers: active pingers' pulses (CW, LFM, HFM, impulse or a replica file), Doppler-resampled
     per ray and added in the time domain when they fall in the frame **[code]**;
  4. the files above are written and `SONAR_FILENAME`, `SONAR_DIR`, `SONAR_FILE_NUM` published
     **[code]**.
- Propagation **[code]**:
  - `ray_acoustics`: eigenray tables from iBellhop for a box around the array, re-requested when
    the range changes by more than 30 m (or after 30 s without an answer), with optional SVPs;
  - `rap_acoustics`: one analytic ray in a linear profile;
  - otherwise one horizontal ray with cylindrical spreading `1/(water_depth·√r)` and attenuation.
- Pingers need eigenrays from `PINGER_<n>_SIM_ARR_FILE`, so they only sound with
  `ray_acoustics` **[code]**. A pinger with `ping_interval > 0` repeats; it moves along its
  heading at its speed between pings **[code]**.
- The random generators are seeded with fixed values (1, and the target number), so runs are
  repeatable **[code]**.

## Running it

- No mission component launches it yet.
- Standalone: `MOOSDB` and `uSimPassiveSonar uSimPassiveSonar.moos` in
  `virtual_ocean/src/bin/uSimPassiveSonar/`, then uSimTowedArray (for `ARRAY_*`), uSimTargets, and
  `SONAR_CONTROL=ON` **[code]**. `uSimPassiveSonar -e` prints every field.
- Tested by `lamss_virtual_ocean_moos_smoke`: `-e` prints a configuration, and the app starts with
  a 4-element configuration and connects to the MOOSDB **[test]**. The test does not switch the
  sonar on, so no frames are written.

## Known issues and open questions

- Without `ray_acoustics`, targets have no acoustic contact: `findAcousticContact` returns -1 and
  `loop()` passes `acoustic_contacts[-1]` to `generate_eigenray_table` **[code]**. This is
  out-of-bounds access in the default (analytic) mode as soon as a target is listed **[code]**;
  the same happens in `handle_target_sim_state` with `svp_updates` but without `ray_acoustics`.
  Should contacts be created for every target?
- `last_slow_loop_t_` is never initialised **[code]**, as in pLamssAEL
  ([GobySoft/lamss3#32](https://github.com/GobySoft/lamss3/issues/32)).
- For a `WAVEFORM_FILE` pinger, `generate_doppler_waveform` assigns the replica length to the
  member `num_samples` (samples per frame) **[code]**. Frames are then written and indexed with
  the wrong length **[code]**. It also sets the pulse width from the previous replica, not the one
  just read **[code]**.
- When the target band reaches 0 Hz (`f_lo = 0`), `spectrum[nfft - f_iter]` writes at index
  `nfft`, one past the end **[code]**.
- `read_covmat_file` stores frequency index `i_last` into `noise_covar`, which has `nfft/2 + 1`
  entries; uSimNoiseCovariance writes `i_last = nfft/2 + 1` when its `maximum_frequency` is at or
  above `fs/2` (its default) **[code]**. One past the end.
- `array_avg_pitch` is uninitialised until `ARRAY_PITCH` arrives but is compared every frame
  **[code]**.
- `TGT_<n>_SIM_STATE` is indexed without checking it has at least two values **[code]**.
- `sync_password` is put into a `bash -c` command line (visible in the process list and logged at
  `DEBUG2`), unquoted in two of the three places **[code]**. Can the copy use an rsync password
  file or SSH keys instead? The 2017 default password was removed from the imported history.
- `pi = 3.14159` is used for the array yaw **[code]**; elsewhere `M_PI`.
- `svp_surface` defaults to 1430 m/s, low for sea water **[guess]**. Intended?
- `bottom_loss` is applied as dB/km; the header calls it "modal attenuation dB/lambda" **[code]**.
- The four roughness parameters are read but not used **[code]**.
- `notes.txt` describes `pSimpleAcousticSim` (2008) rather than this app **[code]**.
- The relative (`delta`) noise profile is parsed against `noise_level`, which `getAmbientNoise`
  overwrites with the last level every frame **[code]**. Should it stay relative to the configured
  level?

## History

- From GobySoft/lamss-shared `src/moos/uSimPassiveSonar` (imported with its history at `043f556`).
  First commit 2011-04-27 (Toby Schneider, "working src-shared from revision 3258"); 133 commits
  before the port, mostly by Henrik Schmidt, with Toby Schneider, Andrew Poulsen, Kay Gemba and
  Arthur Anderson. MATLAB scripts were left out of the import.
- Main changes in the old history: GobyMOOSApp with a protobuf configuration (2016), multiple and
  repeated pingers (2024), `PINGER_GEO` and lat/lon pinger positions (2022–2024), RAY files for
  all sources (2024), double-precision covariance files and reloading covariances after NaNs
  (2025-12-15/16).
- lamss3 port (Phase 2, GobySoft/lamss3#21–#23): Bellhop and SVP requests compiled only with netsim
  (the app refuses `ray_acoustics`/`svp_updates` otherwise); ray-tracing box initialised; noise level
  and hydrophone file pointer initialised; FFT length via `memcpy`; unchecked `fread`/`system()`
  results kept in `[[maybe_unused]]` variables; newmat include and the unbuilt
  `SimPassiveSonarMain.cpp` removed; MIT/GPL-3.0-or-later headers.
