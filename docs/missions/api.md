# Python API

*This page written by Claude*

Mission trees use a handful of names from `lamss.mission`:

| Name | Use |
|---|---|
| `Cruise`, `Fleet`, `Platform`, `Subsystem`, `Component`, `Environment` | base classes of the [concepts](concepts.md) |
| `Process` | one program a component launches |
| `optional(name)` | a component left out, with a note, when no active toolbox defines it |
| `variant(name)` | the decorator that makes a class a [variant](variants.md) |
| `replace(entries, old, new)`, `remove(entries, old)` | edit an inherited list |
| `MissionError`, `VariantConflict` | errors whose message says what is wrong |

Tools that work with missions use `Registry`, `Toolbox`, `resolve` and `Mission`:

```python
from pathlib import Path
from lamss.mission import Registry, Toolbox, resolve

root = Path("/path/to/lamss3")
registry = Registry([Toolbox(t, root / t / "missions") for t in ("core", "extra")]).load()
mission = resolve(registry, "trial", "deep", mode="sim", sets=["warp=10"])
print(mission.to_dict()["platform"]["glider"]["max_depth"])   # 1000
value, layers = mission.explain("platform.glider.max_depth")
```

Give the toolboxes' `missions/` directories as absolute paths. `lamss.mission.workspace.toolboxes(root)` gives the active toolboxes of a checkout, as the command line uses them. Only one registry is active in a process at a time: loading one replaces the `missions` package.

The reference below is generated from the docstrings.

::: lamss.mission
    options:
      show_root_heading: true
      show_submodules: true
      members_order: source
      show_source: false
      show_signature_annotations: true
      separate_signature: true
      filters: ["!^_", "!^cmd_", "!^main$"]
      heading_level: 2
