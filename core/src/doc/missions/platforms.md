# Mission platforms

*This page written by Claude*

`core` defines three base platforms in [`core/missions/platform/`](../../../missions/platform). Each has the default single subsystem, `main`, which gets the platform's components. Toolboxes subclass them for real vehicles and stations (`auv` → `propellor` → `bluefin21` → `macrura`, see [plan 06](../plan/claude/06-missions-framework.md#directory-layout)).

| Platform | `type` | Components of `main` |
|---|---|---|
| [`auv`](../../../missions/platform/auv/auv.py) | `auv` | `interprocess`, `health`, `logging`, `view_message` |
| [`topside`](../../../missions/platform/topside/topside.py) | `topside` | `interprocess`, `health`, `logging`, `view_message`, `optional("viewer")` |
| [`mooring`](../../../missions/platform/mooring/mooring.py) | `mooring` | `interprocess`, `health`, `logging` |

The [components](components.md) start in that order, so `gobyd` and `MOOSDB` are up before their clients. `modem_id` is unset on all three; a fleet sets it per platform (`Auv("unicorn", modem_id=3)`).

- **topside** lists the [`viewer`](../../../../gui/src/doc/missions/viewer.md) from the `gui` toolbox as optional: with `gui` active it adds `goby_liaison` (and `pMarineViewer` when there is a display); without it, the mission runs without and says so:

  ```
  note: platform.topside.subsystem.main: optional component 'viewer' left out (no active toolbox defines it)
  ```

- **mooring** has no MOOS app, but `interprocess` still runs `MOOSDB` and the gateway unless `moos` is turned off:

  ```sh
  lm m sim <cruise> --set platform.<mooring>.subsystem.main.component.interprocess.moos=false
  ```

## Subclassing

A vehicle class adds its own components to its base's and may add subsystems:

```python
from lamss.mission import Subsystem
from missions.platform.auv import Auv


class Propellor(Auv):
    params = dict(max_depth=100)
    components = Auv.components + ["frontseat"]
    subsystems = Auv.subsystems + [Subsystem("frontseat_sim", ["interprocess"], mode="sim")]
```

(An illustration: `propellor` and `frontseat` are not in `core`.)

A [variant](../../../../docs/missions/variants.md) can run a topside on Goby alone, dropping `view_message` and turning MOOS off:

```python
# core/missions/platform/topside/topside_goby_only.py
from lamss.mission import remove, replace, variant

from missions.component.interprocess import Interprocess
from missions.component.view_message import ViewMessage
from missions.platform.topside import Topside


@variant("goby_only")
class TopsideGobyOnly(Topside):
    components = replace(remove(Topside.components, ViewMessage), Interprocess, Interprocess(moos=False))
```

`lm m sim example goby_only` then runs `gobyd`, `goby_coroner`, `goby_logger` and, with `gui`, `goby_liaison`. (Not in `core`; an example.)
