# lamss_autonomy_behaviors

*This section written by Claude*

IvP behaviors for pHelmIvP (and IvPico later), built into one library, `liblamss_autonomy_behaviors.so`.
The helm loads behaviors from every `libBHV_<name>.so` in a directory and passes `createBehavior` the name `BHV_<name>`; each of those files here is a link to the one library, which picks the class by that name.

| Behavior (`.bhv` name) | Directory | Domain |
|---|---|---|
| `BHV_SmartYoyo` | `depth/SmartYoyo` | depth |
| `BHV_MinAltitude` | `depth/MinAltitude` | depth |
| `BHV_ConstantAltitude` | `depth/ConstantAltitude` | depth |
| `BHV_ConstantBuoyancy` | `depth/ConstantBuoyancy` | buoyancy |
| `BHV_OptAcommsDepth` (only built with netsim) | `depth/OptAcommsDepth` | depth |
| `BHV_RaceTrack` | `survey/RaceTrack` | course, speed |
| `BHV_MaintainBearing` | `tracking/MaintainBearing` | course |
| `BHV_WiggleTail` | `tracking/WiggleTail` | course |
| `BHV_LamssZigZag` | `tracking/ZigZag` | course, speed |

`BHV_LamssZigZag` was `BHV_ZigZag`: MOOS-IvP 24.8 has an unrelated `BHV_ZigZag` built into pHelmIvP, which the helm prefers to a loaded one.

The links are installed in `<prefix>/lib/lamss/ivp_behaviors/` (in the build tree, `autonomy/src/lib/behaviors/ivp_behaviors/`). Point the helm at that directory:

```
ProcessConfig = pHelmIvP
{
  ivp_behavior_dir = <prefix>/lib/lamss/ivp_behaviors
  domain = buoyancy:-10:10:21  // only for BHV_ConstantBuoyancy
  ...
}
```

or set `IVP_BEHAVIOR_DIRS`. `src/test/moos_smoke/smoke.bhv` uses each behavior once.
