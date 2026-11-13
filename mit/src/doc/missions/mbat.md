# MBAT cruise

*This page written by Claude*

`mbat` simulates the Massachusetts Bay AUV Testbed, centred on the dumping area east of Boston (datum 42.40587 N, 70.5619 W). It is milestone M1 of the [plan](../../../../core/src/doc/plan/claude/10-milestone-mbat-macrura.md), which builds it up in steps. So far it runs the topside and macrura, which loiters at its deploy point, talks to the topside over simulated acomms (UDP), and tracks a simulated target with its towed array.

| Concept | Class | Notes |
|---|---|---|
| Cruise | [`mbat`](../../../missions/cruise/mbat/mbat.py) | datum, `op_box` (±4 km), `gateway_buoy`, `acomms` (`lamss_status`, `lamss_deploy`, `lamss_track`; 12 s slots for resolution and macrura), `target` (broadband 500–1500 Hz, 1.8 km north-east) |
| Fleet | [`mit_bf21`](../../../missions/fleet/mit_bf21/mit_bf21.py) | `resolution`, `macrura` (topside `resolution`) |
| Platform | [`resolution`](../../../missions/platform/topside/resolution/resolution.py) | a [`topside`](../../../../core/src/doc/missions/platforms.md) with [`udp` acomms](../../../../core/src/doc/missions/components.md#acomms), modem 1 |
| Platform | [`macrura`](../../../missions/platform/auv/propellor/bluefin21/macrura/macrura.py) | a [`bluefin21`](../../../../autonomy/src/doc/missions/components.md) with `udp` acomms and the [passive sonar](../../../../sonar/src/doc/missions/components.md), modem 4 |
| Environment | [`mass_bay`](../../../missions/environment/mass_bay/mass_bay.py) | Gulf of Maine bathymetry ([data](../../../missions/environment/mass_bay/data/README.md)), 1480 m/s at the surface |

```sh
lamss toolbox activate mit gui   # gui is optional: it adds goby_liaison and pMarineViewer
lamss build
lm m sim mbat
lm m test mbat                   # healthy; macrura reaches 15 m; acomms both ways; a track (about 10 min)
lm m stop mbat
```

The ctest `lamss_mit_mission_mbat` runs the same test.

| Subsystem | Processes besides gobyd, MOOSDB, the gateway, goby_coroner, goby_logger |
|---|---|
| `resolution_main` | pViewMessage, pAcommsHandler; with `gui`, goby_liaison and pMarineViewer |
| `macrura_main` | pViewMessage, pNodeReporter, pLamssMissionManager, pBasicContactMgr, pHelmIvP, iFrontSeat_bluefin, pAcommsHandler, pLamssAEL, pLamssCBF, pLamssBearingTracker, pGeoTracker, pLamssTrackManager |
| `macrura_frontseat_sim` | uMVS_Bluefin, pMarinePID, iBackSeatServer, pEchoVar, uSimGPS, iNoaaBathy |
| `macrura_sensor_sim` | uSimTargets, uSimTowedArray, uSimNoiseCovariance, uSimPassiveSonar, sonar_data_cleaner, pMOOSLink |

Thirty seconds after its first GPS fix, macrura's mission manager carries out the initial deploy: a loiter of radius 200 m around (0, 0) at 15 m depth and 1.8 m/s.

Every 24 s cycle, macrura's `LAMSS_STATUS` reaches resolution, where it becomes a `NODE_REPORT`. The gateway plugin publishes that to Goby, so macrura appears on resolution's goby_liaison map. Resolution can command macrura from goby_liaison's commander, or from anything that publishes a `lamss.autonomy.LAMSS_DEPLOY` with `_rcv_id: 4` on `lamss::goby::pAcommsHandler_multiplex`. The test does this to send macrura to 25 m.

The sonar starts off, as on the real vehicle. A deploy with `Sonar_Control: SONAR_CONTROL_ON` and `Sonar_Process: SONAR_PROCESS_ON` turns it on; the test sends one from resolution. Within a few minutes macrura tracks the target, posts it as `priority_trk`, and sends a `LAMSS_TRACK` that shows on resolution's map as `macrura_track_<n>`.
