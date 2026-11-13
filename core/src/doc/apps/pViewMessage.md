---
title: pViewMessage
toolbox: core
kind: goby-moos-app
source: core/src/bin/pViewMessage/
inferred_by: Claude
inferred_from: d5d5346
status: inferred
reviewed_by:
reviewed_on:
launched_by: [core view_message component]
---

# pViewMessage

!!! warning "Inferred by Claude"
    Written by Claude from the source at `d5d5346`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

pViewMessage converts the MOOS-IvP drawing variables `VIEW_POINT`, `VIEW_SEGLIST` and `VIEW_POLYGON`
into compact protobuf-encoded strings (`OUT_VIEW_*`) and converts received ones (`IN_VIEW_*`) back
into drawing variables **[code]**. Its purpose is to let a topside display draw a vehicle's waypoints,
tracks and operating box after they have crossed the acoustic link **[guess]**: the `OUT_VIEW_*`
strings are what `pAcommsHandler` would queue, and the `destination` field is a modem ID **[guess]**.
It runs on vehicles and on the topside **[guess]** (missions-lamss has `pViewMessage.plug` in
`auv`, `topside` and `kayak`).

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `VIEW_POINT` | MOOS | string: XYPoint spec | `handle_view_point` | point to send **[code]** | only if `generate_view_objects`; ignored unless its label contains `waypoint` **[code]** |
| `VIEW_POLYGON` | MOOS | string: XYPolygon spec | `handle_view_polygon` | polygon to send **[code]** | only if `generate_view_objects` |
| `VIEW_SEGLIST` | MOOS | string: XYSegList spec | `handle_view_seglist` | segment list to send **[code]** | only if `generate_view_objects` |
| `VEHICLE_NAME` | MOOS | string | `handle_vehicle_name` | own vehicle name, lower-cased into `vehicle_name` **[code]** | default `unknown` |
| `MISSION_STATE` | MOOS | string | `handle_mission_state` | mission state, used to tag seglists **[code]** | values come from pLamssMissionManager **[guess]** |
| `IN_VIEW_SEGLIST` | MOOS | string: `key=value,...` (ViewSegList) | `handle_in_view_seglist` | received seglist **[code]** |  |
| `IN_VIEW_POLYGON` | MOOS | string: `key=value,...` (ViewSegList) | `handle_in_view_polygon` | received polygon **[code]** |  |
| `IN_VIEW_POINT` | MOOS | string: `key=value,...` (ViewPoint) | `handle_in_view_point` | received point **[code]** |  |
| `LAT_ORIGIN` | MOOS | double | `handle_lat_origin` | new local-grid origin latitude **[code]** | NaN ignored; geodesy re-initialised in `loop()` |
| `LONG_ORIGIN` | MOOS | double | `handle_lon_origin` | new local-grid origin longitude **[code]** | NaN ignored |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `OUT_VIEW_POINT` | MOOS | string: `key=value,...` (ViewPoint) | `ViewMessage.cpp` | point to send over acomms **[code]** | only when the point is active **[code]**; coordinates are lon/lat **[test]** |
| `VIEW_POINT` | MOOS | string: XYPoint spec | `ViewMessage.cpp` | received point, for pMarineViewer **[code]** | label `<vehicle>_<TYPE>` |
| `OLD_VIEW_POINT` | MOOS | string: `active,…:label,…:type,…:source,…:x,y` | `ViewMessage.cpp` | received point in an older drawing format **[code]** | consumer unknown, see open questions |
| `OUT_VIEW_POLYGON` | MOOS | string: `key=value,...` (ViewSegList) | `ViewMessage.cpp` | polygon to send **[code]** | also used for seglists of more than 13 points, replaced by their convex hull **[code]** |
| `OUT_VIEW_SEGLIST` | MOOS | string: `key=value,...` (ViewSegList) | `ViewMessage.cpp` | seglist to send **[code]** | only when active and non-empty |
| `VIEW_POLYGON` | MOOS | string: XYPolygon spec | `ViewMessage.cpp` | received polygon **[code]** |  |
| `VIEW_SEGLIST` | MOOS | string: XYSegList spec | `ViewMessage.cpp` | received seglist **[code]** |  |
| `OLD_VIEW_POLYGON` | MOOS | string, same older format | `ViewMessage.cpp` | received polygon, older format **[code]** | consumer unknown |
| `OLD_VIEW_SEGLIST` | MOOS | string, same older format | `ViewMessage.cpp` | received seglist, older format **[code]** | consumer unknown |
<!-- END GENERATED publishes -->

### Configuration

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `common` | GobyMOOSAppConfig |  | no | standard Goby MOOS app settings (`app_tick`, `comm_tick`, log, origin) **[code]** |
| `generate_view_objects` | bool | `true` | no | subscribe to `VIEW_*` and produce `OUT_VIEW_*` **[code]**; set false on a topside that only receives **[guess]** |
| `destination` | int32 | `1` | no | written into every outgoing message's `destination` **[code]**; modem ID of the receiver **[guess]** |
<!-- END GENERATED config -->

Example (from `pViewMessage.moos`):

```
ProcessConfig = pViewMessage
{
  common {
    app_tick: 4
    comm_tick: 4
  }
  generate_view_objects: true
  destination: 1
}
```

### Files, devices and external programs

None **[code]**.

## Behaviour

- Ignores `VIEW_*` posts made by itself, so the `VIEW_*` it publishes from received messages are not sent back out **[code]**.
- Outgoing points: only `VIEW_POINT` labels containing `waypoint` are converted, and only active points are published **[code]**.
- Outgoing seglists and polygons:
  - a label `OpRegion` makes the polytype `OPBOX`, a label containing `stvp` makes it `OBSTACLE`, anything else `NORMAL` **[code]**;
  - more than 13 points are replaced by their convex hull and sent as a polygon **[code]**.
- Coordinates are converted from the local grid to longitude/latitude with the current origin, and back on receipt **[code]**. A VIEW_POINT at x=10, y=20 survives the round trip in `lamss_core_moos_smoke` **[test]**.
- The local origin follows `LAT_ORIGIN`/`LONG_ORIGIN` at run time ("dynamic UTM", 2021) **[code]**.

## Running it

- Launched by core's `view_message` mission component (GobySoft/lamss3#20) **[code]**.
- Standalone: `MOOSDB` and `pViewMessage pViewMessage.moos` in `core/src/bin/pViewMessage/`.
- Tested by `lamss_core_moos_smoke` (VIEW_POINT out and back) **[test]**.

## Known issues and open questions

- Who reads `OLD_VIEW_*`? Nothing in lamss3 subscribes to them, yet the gateway plugin also posts `OLD_VIEW_POLYGON` (`core/src/lib/moos_gateway_plugin/moos_gateway_plugin.cpp`). Are they for GEOV or an old topside display, and can they be removed?
- A code comment says seglists of "more than 9 points" become a polygon, but the limit is 13. Which is intended?
- `ViewPoint_GEO` and `ViewSegList_GEO` are defined in `view_message.proto` but not used by this app. Are they used elsewhere?
- Points whose label lacks `waypoint` are silently dropped. Is that intended for every point type (`TRAILPOINT`, `ATTRACTOR`, `STATION` exist in the proto)?

## History

- From GobySoft/lamss `src/moos/pViewMessage`; first commit 2011-04-26 (Toby Schneider); 24 commits by Henrik Schmidt and Toby Schneider before the port.
- lamss3 port (GobySoft/lamss3#11): ICEX 25 kHz destination switching removed (subscription `ICEX_VEHICLE_TRANSMIT_SELECTION_IN` and config field `icex_25khz_id`); config proto moved to `package lamss.core`.
