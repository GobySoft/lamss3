# Missions

*This page written by Claude*

A lamss3 mission is described by Python classes, not by preprocessor macros. A **cruise** names a **fleet** of **platforms** and an **environment**; each platform is made of **subsystems**, and each subsystem of **components**. `lamss mission` finds these classes in the active toolboxes' `missions/` directories, resolves the cruise into one plain dictionary, renders each subsystem's configuration files from Jinja2 templates, and runs the processes under one `supervisord`.

```mermaid
flowchart LR
  subgraph toolboxes [active toolboxes]
    core[core/missions/]
    other[&lt;toolbox&gt;/missions/]
  end
  toolboxes --> registry[Registry<br/>classes by path and name]
  registry --> resolve["resolve<br/>cruise, variant,<br/>mode, --set"]
  resolve --> mission[mission dict<br/>show, --explain]
  mission --> render[render<br/>Jinja2 templates<br/>per subsystem]
  render --> build["missions/build/&lt;instance&gt;/<br/>configs, env.sh,<br/>supervisord.conf"]
  build --> sup[supervisord<br/>sim, run,<br/>status, tail, stop]
```

## Trying it

```sh
lm m list                         # cruises and their variants
lm m list <cruise>                # its platforms, subsystems and components
lm m show <cruise> [variant]      # the resolved mission as YAML
lm m generate <cruise>            # write the configuration only
lm m sim <cruise>                 # generate and start the whole fleet on this machine
lm m status
lm m stop
```

`lm m` is short for `lamss mission`; the [command guide](cli.md) has every option.

## Pages

| Page | What it covers |
|---|---|
| [Concepts](concepts.md) | cruise, fleet, platform, subsystem, component, environment; inheritance and composition |
| [Layout and registry](layout.md) | the `missions/` directories and how classes are found across toolboxes |
| [Parameters](parameters.md) | where a value comes from, `--set` and `--explain` |
| [Variants](variants.md) | named alternatives of a concept, and combining them |
| [Templates](templates.md) | Jinja2 templates and the context dictionary they see |
| [Output](output.md) | the generated files, and how supervisord starts and stops the processes |
| [`lamss mission`](cli.md) | the command line |
| [Python API](api.md) | the `lamss.mission` package |

The design is in [plan 06](../../core/src/doc/plan/claude/06-missions-framework.md), from the "Missions" section of the [core plan](../../core/src/doc/plan/core.md#missions).
