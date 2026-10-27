# Splitting `lib_lamss_protobuf`

*This document written by Claude*

Detail for the "Splitting up Protobuf messages" section of the [core plan](../core.md).

## What exists

`lamss/src/lib/lib_lamss_protobuf` holds 60 `.proto` files (3.7k lines) plus `icex_groups.h` and a Goby2-era `goby_dccl_load.cpp`. Almost all are proto2 with **no `package`**, so every message lives in the global C++ namespace. Options in use: `(dccl.msg).id/max_bytes/codec_version=3|4`, `(dccl.field)` bounds and units, custom `(lamss.field)` options from `option_extensions.proto`, and two leftover Goby2 `(goby.msg).queue` options. The library links `goby`, `goby_moos`, `dccl_arithmetic` and `netsim_messages`, so every consumer drags in all of them.

Six DCCL IDs are assigned twice (variants that were never meant to be loaded together):

| ID | Messages |
|---|---|
| 16 | `MCM_TRACKS`, `SONAR_TRACKS` |
| 17 | `LAMSS_TRANSIT`, `OOI_TRANSIT` |
| 18 | `LAMSS_DEPLOY`, `OOI_DEPLOY` |
| 20 | `LAMSS_STATUS`, `OOI_STATUS` |
| 21 | `NMEA_SHIP`, `MinimalStatus`, `LegacyNodeReport`, `OOI_*` |
| 22 | `NMEA_BUOY`, `MinimalNodeStatusDifference`, `OOI_*` |

Consumers at compile time are few (gateway plugin, `pViewMessage`, `henrik_util`, `iCTD_OceanModel`); most messages are loaded at runtime by `pAcommsHandler`/`gobyd` via `dccl_load_library` and used by name from MOOS strings.

## Target

```mermaid
flowchart TB
  subgraph core_p [core: lamss.core]
    hdr[header.proto<br/>LAMSSHeader]
    opt[options.proto<br/>lamss.field / lamss.msg]
    nav[nav.proto<br/>openocean.Navigation wrapper, node reports]
    status[status.proto<br/>LAMSS_STATUS, topside status]
    view[view.proto<br/>point / seglist / polygon / circle]
    env[environment_update.proto]
    moos[moos_message.proto<br/>shim transport]
  end
  subgraph auton_p [autonomy: lamss.autonomy]
    deploy[deploy.proto<br/>DEPLOY, INITIAL_DEPLOY, SURFACE_DEPLOY, TRANSIT, SURVEY_PATH]
    pros[prosecute.proto, stovepipe.proto]
    fs[frontseat.proto<br/>hybrid_uuv, IXBlue INS]
    enums[enumerates.proto]
  end
  subgraph sonar_p [sonar: lamss.sonar]
    sc[sonar_control.proto, processing.proto]
    ct[contact.proto, track.proto, btr.proto]
    arr[array.proto<br/>ArrayNavigation, BeamformingVars]
    tgt[targetsim.proto, pinger.proto]
  end
  subgraph vo_p [virtual_ocean: lamss.virtual_ocean]
    ms[modem_sim.proto]
    mp[model_param.proto]
    samp[ctd / adcp samples]
  end
  subgraph hm_p [hydroman: lamss.hydroman]
    hm[hydroman.proto<br/>command, health, internal]
  end
  subgraph icex_p [icex: lamss.icex]
    it[icex_tracking.proto, optimizer_data.proto]
  end
  auton_p --> core_p
  sonar_p --> core_p
  vo_p --> core_p
  icex_p --> core_p
  icex_p --> auton_p
  hm_p --> core_p
  sonar_p -.-> auton_p
```

Rules:

1. **Package per toolbox**: `package lamss.<toolbox>;` and files under `src/proto/lamss/<toolbox>/`. Generated headers are `lamss/<toolbox>/<file>.pb.h`. Imports across toolboxes use the installed path (`import "lamss/core/header.proto";`), which `lamss_add_proto` resolves through `build/usr/include`.
2. **One generated library per toolbox**, `lamss_<tb>_proto`, linking only `protobuf` and `dccl` (and `goby` only where a message imports a Goby proto, which is `nav`, `status` and `modem_sim`). `netsim_messages` is linked by `virtual_ocean` only.
3. **DCCL IDs are allocated from a table** kept in `core/src/proto/lamss/DCCL_IDS.md`, one range per toolbox:

   | Toolbox | DCCL ID range |
   |---|---|
   | core | 1–99 |
   | autonomy | 100–199 |
   | sonar | 200–299 |
   | virtual_ocean | 300–399 |
   | icex | 400–499 |
   | hydroman | 500–599 |
   | user toolboxes | 1000+ |

   CI runs `dccl --analyze` over all active toolboxes' DCCL messages and fails on any duplicate ID. The old IDs are kept **only** for messages that must stay wire-compatible with un-upgraded platforms; the human plan should list those (default assumption: none).
4. **Variants go to attic**: `ooi_*`, `minimal_status`, `node_status` legacy variants, `mstiff_image`, `status_fillin` unless a live mission uses them.
5. **Nav converges on `openocean.Navigation`** (the type IvPico and goby3 frontseat expect). `LegacyNodeReport`/`NODE_REPORT` strings survive only in the pMarineViewer bridge.
6. **Custom options**: `option_extensions.proto` becomes `lamss/core/options.proto` with extension numbers registered against the in-house range (50000–99999, avoiding IvPico's 57400 and DCCL/Goby numbers).
7. **No `.proto.in`**: `@PROTOBUF_ALLOW_ALIAS@` templating goes away; protobuf ≥ 3 is a given.

## Config protos

Each app's `*_config.proto` moves with the app. For `GobyMOOSApp` apps it keeps extending `goby.moos.protobuf.GobyMOOSAppConfig` until the app is rewritten, then `goby.middleware.protobuf.AppConfig`. The config protos are **not** part of the toolbox proto library; they are private to the app target (`lamss_add_app(... PROTOS config.proto)`).

## Runtime loading

Today `pAcommsHandler.plug` lists `dccl_load_library: "$(LAMSS_LIB_ROOT)/liblamss_protobuf.so"`. The mission framework's acomms component emits one `dccl_load_library` line per active toolbox proto library it needs (`liblamss_core_proto.so`, `liblamss_autonomy_proto.so`, …), using the install prefix from `lamss env`. The Goby2 `goby_dccl_load()` hook and `lib_comms_state` are deleted.

## Migration steps

```mermaid
flowchart LR
  a[Copy all 60 protos into core/src/proto staging] --> b[Add package + move file per table above]
  b --> c[Rewrite imports; build lamss_core_proto alone]
  c --> d[Renumber DCCL IDs per range; dccl --analyze in CI]
  d --> e[Build autonomy/sonar/vo/icex proto libs]
  e --> f[Port compile-time consumers: gateway plugin, pViewMessage, henrik_util]
  f --> g[Regenerate pAcommsHandler/gobyd templates with new library names]
  g --> h[Delete staging; attic the variants]
```

Each step is one PR. Step `c` is the Phase 1 exit criterion for protobuf.

## Open questions for the human plan

- Are any platforms in the field running old DCCL IDs that must keep talking to lamss3 nodes? If yes, list the message names here and they keep their IDs.
- `lamss_status.proto` (183 lines) mixes nav, health and mission state. Split it into `nav` + `health` + `autonomy.MissionState`, or keep it monolithic for acomms efficiency? **Recommendation: keep one DCCL message for the acoustic link but build it from the split in-memory types.**
