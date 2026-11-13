# Mission components

*This page written by Claude*

`virtual_ocean` simulates the ocean and the vehicle's sensors in it. Its components run only in simulation (`sim_only`) and live in [`virtual_ocean/missions/component/`](../../../missions/component).

| Component | Processes | Placed by | Notes |
|---|---|---|---|
| [`bathymetry`](../../../missions/component/bathymetry/bathymetry.py) | [iNoaaBathy](../apps/iNoaaBathy.md) | an environment's `components` | reads `environment.data_dir`/`environment.bathymetry.file` (NetCDF); `publish_interval` (5 s) |
| [`gps_sim`](../../../missions/component/gps_sim/gps_sim.py) | [uSimGPS](../apps/uSimGPS.md) | a platform's simulation subsystem | fixes while shallower than `max_depth` (5 m), every `interval` (2 s), after `min_time` (15 s) at the surface |

An environment lists the components it simulates and names the subsystem they join (`subsystem`). For example, the `mit` toolbox's `mass_bay` puts `bathymetry` in each platform's `frontseat_sim`.
