# Mission components

*This page written by Claude*

`virtual_ocean` simulates the ocean and the vehicle's sensors in it. Its components run only in simulation (`sim_only`) and live in [`virtual_ocean/missions/component/`](../../../missions/component).

| Component | Processes | Placed by | Notes |
|---|---|---|---|
| [`bathymetry`](../../../missions/component/bathymetry/bathymetry.py) | [iNoaaBathy](../apps/iNoaaBathy.md) | an environment's `components` | reads `environment.data_dir`/`environment.bathymetry.file` (NetCDF); `publish_interval` (5 s) |
| [`gps_sim`](../../../missions/component/gps_sim/gps_sim.py) | [uSimGPS](../apps/uSimGPS.md) | a platform's simulation subsystem | fixes while shallower than `max_depth` (5 m), every `interval` (2 s), after `min_time` (15 s) at the surface |
| [`targets`](../../../missions/component/targets/targets.py) | [uSimTargets](../apps/uSimTargets.md) | a platform's simulation subsystem | starts the cruise's `target` (`x`, `y`, `depth`, `heading`, `speed`, `freq`, `bandwidth`, `spl`, `strength`, `tones`) as `auto_target`; more arrive as `TGT_STATE_IN` |
| [`passive_sonar_sim`](../../../missions/component/passive_sonar_sim/passive_sonar_sim.py) | [uSimTowedArray](../apps/uSimTowedArray.md), [uSimNoiseCovariance](../apps/uSimNoiseCovariance.md), [uSimPassiveSonar](../apps/uSimPassiveSonar.md), `sonar_data_cleaner` | a platform's simulation subsystem | see below |

An environment lists the components it simulates and names the subsystem they join (`subsystem`). For example, the `mit` toolbox's `mass_bay` puts `bathymetry` in each platform's `frontseat_sim`.

## passive_sonar_sim

*This section written by Claude*

Simulates a towed array and the targets it hears. uSimTowedArray shapes the array behind the vehicle from `NAV_*`. uSimNoiseCovariance and uSimPassiveSonar write element time series into the subsystem's run directory, 3 MB a frame, and announce each file in `SONAR_FILENAME` for a beamformer (the `sonar` toolbox's `passive` component) to read. `sonar_data_cleaner` deletes files older than `keep_minutes` (2), so a long simulation does not fill the disk.

| Parameter | Default | Meaning |
|---|---|---|
| `bellhop` | `false` | eigenrays from iBellhop (not run by this component; see [plan 10](../../../../core/src/doc/plan/claude/10-milestone-mbat-macrura.md)); otherwise analytic rays in the environment's linear profile (`environment.sound_speed`, `environment.bathymetry.depth`) |
| `keep_minutes` | 2 | age at which data files are deleted |
| `noise_level` | 40 | ambient noise (dB) |
| `elements`, `sampling_frequency`, `samples`, `array_type` | 32, 12000, 24000, `DURIP` | the array and its frames |
| `tow_point`, `spacing_groups` | DURIP | array geometry |
