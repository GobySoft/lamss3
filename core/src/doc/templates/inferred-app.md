---
# Inferred-documentation template. Copy to <toolbox>/src/doc/apps/<app>.md (or behaviors/, libraries/)
# and fill in. Plan: core/src/doc/plan/claude/11-documentation.md.
title: <app name>
toolbox: <core | autonomy | sonar | virtual_ocean | gui | mit | ...>
kind: <moos-app | goby-moos-app | goby-app | ivp-behavior | library | script>
source: <toolbox>/src/bin/<app>/          # directory documented
inferred_by: <agent, e.g. Claude>          # who wrote the page from the code
inferred_from: <short commit hash>         # commit the page was written against
status: inferred                           # inferred -> reviewed (only a maintainer changes this)
reviewed_by:                               # GitHub username, set together with status: reviewed
reviewed_on:                               # YYYY-MM-DD
launched_by: [<mission component(s) that run it, or "none yet">]
---

# <app name>

!!! warning "Inferred by <agent>"
    Written by <agent> from the source at `<commit>`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

<!-- One short paragraph: what the app is for and where it sits in a mission (which apps feed it,
which consume its output). Tag every sentence. -->

## Interfaces

<!-- The tables between the GENERATED markers come from the extraction script
(`lamss docs extract <app>`); CI fails if they drift from the code. Edit only the
"Meaning" and "Notes" columns; the script keeps them across re-runs. -->

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `VAR_NAME` | MOOS | string: `key=value,...` | `file.cpp:LINE` | | |
| `lamss::moos::GROUP` | Goby interprocess | `lamss.core.Message` | `file.cpp:LINE` | | |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `VAR_NAME` | MOOS | double | `file.cpp:LINE` | | |
<!-- END GENERATED publishes -->

### Configuration

<!-- For Goby-config apps (GobyMOOSApp, Goby3): generated from the config proto (fields, types,
defaults, (goby.field).description) and the `-e` example. For CMOOSApp: generated from the
GetConfigurationParam / m_MissionReader calls. -->

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `name` | int32 | `1` | no | |
<!-- END GENERATED config -->

Example (from `<app>.moos` / `<app> -e`):

```
ProcessConfig = <app>
{
}
```

### Files, devices and external programs

<!-- Files read or written (paths, formats), serial ports and sockets, programs run with system()
or similar. "None" if none. -->

## Behaviour

<!-- How inputs become outputs: the main loop, filters (e.g. "ignores its own posts"), state
machine (a mermaid stateDiagram for anything with states), timing (AppTick, timeouts). Tag
every statement. -->

## Running it

<!-- Which mission component launches it, minimal standalone run (MOOSDB + this app), and the
test(s) that exercise it. -->

## Known issues and open questions

<!-- Links to GitHub issues, and questions for the authors/maintainers: things the code does that
look unintended, unused inputs or outputs, magic numbers. Each open question is a candidate
issue. -->

## History

<!-- Origin repository and path, first commit date and authors (from `git log --follow`), and
what changed during the lamss3 port (removed features, renamed variables). -->
