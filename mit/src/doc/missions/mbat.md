# MBAT cruise

*This page written by Claude*

`mbat` simulates the Massachusetts Bay AUV Testbed, centred on the dumping area east of Boston (datum 42.40587 N, 70.5619 W). The ship *Resolution* commands *Macrura*, a Bluefin 21 towing a passive array, over simulated acoustic communications. Macrura dives, loiters and tracks a simulated target. This is milestone M1 of the [plan](../../../../core/src/doc/plan/claude/10-milestone-mbat-macrura.md).

## Run it

On Ubuntu 22.04, 24.04 or 26.04, from a clone of lamss3:

```sh
./init.sh                                         # build the lamss tool, install the dependencies
lamss toolbox activate mit gui                    # mit brings autonomy, sonar and virtual_ocean
lamss dependencies install
lamss build
lm m sim mbat                                     # prints the supervisord and goby_liaison addresses
```

Open goby_liaison, which runs on resolution, and select the Map tab. About 30 s after start, macrura dives to 15 m and loiters 200 m around the deploy point (0, 0). Its position on the map is updated each time a status arrives over acomms, every 24 s. With `gui` active and a display, pMarineViewer also shows resolution's MOOS view.

To turn macrura's sonar on, send a deploy command from goby_liaison's Commander: `lamss.autonomy.LAMSS_DEPLOY` with `_rcv_id: 4`, `Sonar_Control: SONAR_CONTROL_ON`, `Sonar_Process: SONAR_PROCESS_ON`, and the loiter values above. Within a few minutes the track `macrura_track_0` appears on the map, north-east of macrura.

```sh
lm m status mbat                                  # every process RUNNING
lm m tail mbat -f                                 # follow the logs
lm m stop mbat
```

`lm m sim mbat smoke` runs the same cruise five times faster.

## Test

```sh
lm m test mbat smoke                              # about 3 minutes
```

| Check | Where |
|---|---|
| every subsystem's Goby apps report healthy, and no process exits during the test | `require_app`; `lamss mission test` |
| macrura settles at 15 m, then holds 15 ± 2 m inside its loiter for 40 s | test steps 2–3 |
| macrura's `LAMSS_STATUS` reaches resolution over acomms, as a `NODE_REPORT` and a goby `NodeStatus` | step 4 |
| a `LAMSS_DEPLOY` from resolution sends macrura to 25 m | step 5 |
| with the sonar on, macrura's track reaches resolution | step 6 |
| the track's bearing from macrura is within 20° of the true bearing to the target | `check_track_bearing.py`, after the test |
| generating the cruise twice gives byte-identical output | `test_mbat_generate.py` |

ctest runs these as `lamss_mit_mission_mbat_smoke`, `lamss_mit_mission_mbat_smoke_bearing` and `lamss_mit_mission_mbat_generate`. Configuring with `-DLAMSS_MIT_MBAT_REALTIME_TEST=ON` adds `lamss_mit_mission_mbat`, the same test in real time (about 10 minutes).

## What runs

| Concept | Class | Notes |
|---|---|---|
| Cruise | [`mbat`](../../../missions/cruise/mbat/mbat.py), variant [`smoke`](../../../missions/cruise/mbat/mbat_smoke.py) | datum, `op_box` (±4 km), `gateway_buoy`, `acomms` (`lamss_status`, `lamss_deploy`, `lamss_track`; a 12 s slot each for resolution and macrura), `target` (broadband 500–1500 Hz, 1.8 km north-east); `smoke` sets `warp` to 10 |
| Fleet | [`mit_bf21`](../../../missions/fleet/mit_bf21/mit_bf21.py) | `resolution`, `macrura` (topside `resolution`) |
| Platform | [`resolution`](../../../missions/platform/topside/resolution/resolution.py) | a [`topside`](../../../../core/src/doc/missions/platforms.md) with [`udp` acomms](../../../../core/src/doc/missions/components.md#acomms), modem 1 |
| Platform | [`macrura`](../../../missions/platform/auv/propellor/bluefin21/macrura/macrura.py) | a [`bluefin21`](../../../../autonomy/src/doc/missions/components.md) with `udp` acomms and the [passive sonar](../../../../sonar/src/doc/missions/components.md), modem 4 |
| Environment | [`mass_bay`](../../../missions/environment/mass_bay/mass_bay.py) | Gulf of Maine bathymetry ([data](../../../missions/environment/mass_bay/data/README.md)), 1480 m/s at the surface, 100 m for the acoustics |

| Subsystem | Processes besides gobyd, MOOSDB, the gateway, goby_coroner, goby_logger |
|---|---|
| `resolution_main` | pViewMessage, pAcommsHandler; with `gui`, goby_liaison and pMarineViewer |
| `macrura_main` | pViewMessage, pNodeReporter, pLamssMissionManager, pBasicContactMgr, pHelmIvP, iFrontSeat_bluefin, pAcommsHandler, pLamssAEL, pLamssCBF, pLamssBearingTracker, pGeoTracker, pLamssTrackManager |
| `macrura_frontseat_sim` | uMVS_Bluefin, pMarinePID, iBackSeatServer, pEchoVar, uSimGPS, iNoaaBathy |
| `macrura_sensor_sim` | uSimTargets, uSimTowedArray, uSimNoiseCovariance, uSimPassiveSonar, sonar_data_cleaner, pMOOSLink |
