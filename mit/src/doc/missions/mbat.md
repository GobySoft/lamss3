# MBAT cruise

*This page written by Claude*

`mbat` simulates the Massachusetts Bay AUV Testbed, centred on the dumping area east of Boston (datum 42.40587 N, 70.5619 W). It is milestone M1 of the [plan](../../../../core/src/doc/plan/claude/10-milestone-mbat-macrura.md), which builds it up in steps. At this step it runs only the topside.

| Concept | Class | Notes |
|---|---|---|
| Cruise | [`mbat`](../../../missions/cruise/mbat/mbat.py) | datum |
| Fleet | [`mit_bf21`](../../../missions/fleet/mit_bf21/mit_bf21.py) | `resolution` |
| Platform | [`resolution`](../../../missions/platform/topside/resolution/resolution.py) | a [`topside`](../../../../core/src/doc/missions/platforms.md), modem 1 |
| Environment | [`mass_bay`](../../../missions/environment/mass_bay/mass_bay.py) | 100 m flat bottom, 1480 m/s at the surface |

```sh
lamss toolbox activate mit gui   # gui is optional: it adds goby_liaison and pMarineViewer
lamss build
lm m sim mbat
lm m test mbat                   # resolution's Goby apps report healthy
lm m stop mbat
```

The ctest `lamss_mit_mission_mbat` runs the same test.
