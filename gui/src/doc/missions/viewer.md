# The viewer component

*This page written by Claude*

`viewer` ([`gui/missions/component/viewer/viewer.py`](../../../missions/component/viewer/viewer.py)) puts a graphical view of the mission on a subsystem: [goby_liaison](https://github.com/GobySoft/goby3/blob/3.0/src/doc/markdown/doc505_goby_liaison.md), Goby's web interface, and, while lamss has MOOS apps, `pMarineViewer`. It is in the `gui` toolbox so that vehicles and CI need no display packages: `core`'s [topside](../../../../core/src/doc/missions/platforms.md) lists it as `optional("viewer")`, used only when `gui` is active.

```sh
lm tb activate gui
lm dep install            # goby3-gui, for goby_liaison
lm b
lm m sim example
```

| Parameter | Default | Meaning |
|---|---|---|
| `http_address` | `localhost` | address goby_liaison serves on; `0.0.0.0` for other computers |
| `marine_viewer` | `auto` | run `pMarineViewer`: `auto`, `true` or `false` |

| Process | Command | Configuration |
|---|---|---|
| `goby_liaison` | `goby_liaison liaison.pb.cfg` | [`liaison.pb.cfg.j2`](../../../missions/component/viewer/liaison.pb.cfg.j2) |
| `pMarineViewer` | `pMarineViewer last.moos` | [`pMarineViewer.moos.j2`](../../../missions/component/viewer/pMarineViewer.moos.j2), a fragment of `last.moos` |

- It requires the [`interprocess`](../../../../core/src/doc/missions/components.md#interprocess) component and needs a port, `liaison`, [allocated](../../../../docs/missions/templates.md#ports) from the `http` port range (50000 by default, away from MOOS's 9000s); `lm m show <cruise>` prints it as the subsystem's `ports.liaison`, and goby_liaison serves on `http://<http_address>:<port>/`.
- goby_liaison answers health requests, so the [`health`](../../../../core/src/doc/missions/components.md#health) component expects it. Its access log and its commander database are in the subsystem's run directory.
- `pMarineViewer` runs only with `interprocess.moos` on. With `auto` it also needs a display (`DISPLAY` or `WAYLAND_DISPLAY`) and `pMarineViewer` on `PATH` when the mission is **generated**; `true` runs it regardless. Since this depends on the shell that generates, set `marine_viewer` explicitly where the output must not vary:

  ```sh
  lm m sim example --set platform.topside.subsystem.main.component.viewer.marine_viewer=false
  ```
