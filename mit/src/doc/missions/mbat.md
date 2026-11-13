# MBAT cruise

*This page written by Claude*

`mbat` simulates the Massachusetts Bay AUV Testbed, centred on the dumping area east of Boston (datum 42.40587 N, 70.5619 W). It is milestone M1 of the [plan](../../../../core/src/doc/plan/claude/10-milestone-mbat-macrura.md), which builds it up in steps. So far it runs the topside and macrura, which loiters at its deploy point and talks to the topside over simulated acomms (UDP).

| Concept | Class | Notes |
|---|---|---|
| Cruise | [`mbat`](../../../missions/cruise/mbat/mbat.py) | datum, `op_box` (±4 km), `gateway_buoy`, `acomms` (`lamss_status`, `lamss_deploy`; 12 s slots for resolution and macrura) |
| Fleet | [`mit_bf21`](../../../missions/fleet/mit_bf21/mit_bf21.py) | `resolution`, `macrura` (topside `resolution`) |
| Platform | [`resolution`](../../../missions/platform/topside/resolution/resolution.py) | a [`topside`](../../../../core/src/doc/missions/platforms.md) with [`udp` acomms](../../../../core/src/doc/missions/components.md#acomms), modem 1 |
| Platform | [`macrura`](../../../missions/platform/auv/propellor/bluefin21/macrura/macrura.py) | a [`bluefin21`](../../../../autonomy/src/doc/missions/components.md) with `udp` acomms, modem 4 |
| Environment | [`mass_bay`](../../../missions/environment/mass_bay/mass_bay.py) | Gulf of Maine bathymetry ([data](../../../missions/environment/mass_bay/data/README.md)), 1480 m/s at the surface |

```sh
lamss toolbox activate mit gui   # gui is optional: it adds goby_liaison and pMarineViewer
lamss build
lm m sim mbat
lm m test mbat                   # every subsystem healthy; macrura reaches 15 m; acomms both ways
lm m stop mbat
```

The ctest `lamss_mit_mission_mbat` runs the same test.

| Subsystem | Processes besides gobyd, MOOSDB, the gateway, goby_coroner, goby_logger |
|---|---|
| `resolution_main` | pViewMessage, pAcommsHandler; with `gui`, goby_liaison and pMarineViewer |
| `macrura_main` | pViewMessage, pNodeReporter, pLamssMissionManager, pBasicContactMgr, pHelmIvP, iFrontSeat_bluefin, pAcommsHandler |
| `macrura_frontseat_sim` | uMVS_Bluefin, pMarinePID, iBackSeatServer, pEchoVar, uSimGPS, iNoaaBathy |

Thirty seconds after its first GPS fix, macrura's mission manager carries out the initial deploy: a loiter of radius 200 m around (0, 0) at 15 m depth and 1.8 m/s.

Every 24 s cycle, macrura's `LAMSS_STATUS` reaches resolution, where it becomes a `NODE_REPORT`. The gateway plugin publishes that to Goby, so macrura appears on resolution's goby_liaison map. Resolution can command macrura from goby_liaison's commander, or from anything that publishes a `lamss.autonomy.LAMSS_DEPLOY` with `_rcv_id: 4` on `lamss::goby::pAcommsHandler_multiplex`. The test does this to send macrura to 25 m.
