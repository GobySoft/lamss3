# Approach

- Finalize toolbox segregation for binaries and applications, and get the build system working.
- Implement `lamss` tool for `toolbox`, `build` and `dependencies`
- Set up CI to build each toolbox independently, and all together.
- Build the missions framework
	+ Add the basic Macrura mission in MBAT
	+ Add some variants to the Macrura mission in MBAT
	+ Add the ICEX missions in the Arctic environment

# Source material

## LAMSS v1 (MIT) projects

These are the existing projects we want to base LAMSS 3 on:

- `missions-lamss`: Mission repository (GPP text preprocessor)
- `lamss`: Public repository
- `lamss-shared`: Laboratory + collaborator repository
- `lamss-internal`: Internal sandbox

We want to refactor the code repositories (lamss*) into application-specific "toolboxes". See "Toolboxes" section.

We want to rebuild the missions structure in a minimal way initially to use the same basic flexible mission concept as `mission-lamss` but using modern tooling. See "Missions" below.

## Source docs

See the [very early concept](sources/2025-05-07-lamss-future.md) for a potential break down of the old repos into toolboxes. This needs inspection against the existing `missions-lamss` missions for final.

# Toolboxes 

Toolboxes can either be part of the main `lamss3` git repository or added as `git submodules`.

Part of the `lamss3` git repository:
 - **core**: Basic tools.
 - **autonomy**: Related to vehicle autonomy, control, mission state machine, pHelmIvP behaviors.
 - **sonar**: Sonar processing pipelines.
 - **virtual_ocean**: Simulators for creating a simulated ocean.
 - **gui**: Graphical and web displays (Goby Liaison plugins, LAMSS Display Center, pMarineViewer). Separate because its dependencies are large and are not wanted on vehicles.
 
Git submodules
 - **mit**: MIT missions from missions-lamss.
 - **icex**: ICEX-specific applications
 - **attic**: Deprecated applications
 - *user-toolbox1*: External users can add toolboxes of their own.

When a repository is cloned, only the `core` toolbox is active (that is, builds and is used by the mission search). Users can `activate` additional toolboxes in the core library or `add` toolboxes from external git submodules (which default active when added).


# Missions

## Concepts

The `missions-lamss` structure is powerful because it provides an object-oriented extensible way to reconfigure a cruise for a particular vehicle(s) and each vehicle can have different components. We want to formalize these "concepts":

- `cruise`: A temporally and/or physically bounded deployment of a given `fleet` that takes place in a given `environment`.
- `fleet`: A group of one or more `platforms`
- `platform`: An autonomous underwater vehicle (`auv`), autonomous surface vehicle (`asv`), a `mooring`, an operator `topside`. These may be subclassed: `auv` -> `propellor` -> `bluefin21` -> `macrura`. A `platform` is composed of many `components`. Platforms can be made of one or more subsystems (main, sensor_sim, frontseat_sim) that are generated separately and may be run on different computers.
- `component`: A physical or conceptual part of a platform. A sensor driver, an autonomy engine, and a comms system are all components.
- `environment`: This only has an instantiation in software for simulation: the real world defines the runtime environment. This is a collection of configuration for simulating a given region of the world, possibly at a given time window.

Each "concept" can have an instantation that differs between `runtime` (real ops) and at `simulation` but otherwise shares as much configuration as possible.

## Output

The output of the missions configuration structure is a set of files for launching the applications, and a launch script itself using `supervisord`. This looks like a bunch of goby Protobuf TextFormat files, pHelmIvP behavior files, or other custom text files as needed.


## Implementation

This will be implemented using Python3 classes for the inheritance and composition embodied in the `concepts` above. These classes will produce a structured `dict` that maps the data for each cruise in a hierarchical manner that mirrors the directory structure (below). 

This `dict` will be used to pre-process a set of text template files using the Jinja2 templating language. The output of these processed files will be used to actually feed the applications.

The repository layout (see below under "Technical details") should mirror the Python class inheritance structure. At each level in the file tree, a Python file with the same name as the parent directory will implement the code.

At the leaf nodes, we can have "variants": These are subclasses of the core concept that change one or more things about the core concept. For example, a `vehicle` variant might add or remove a component. A `fleet` variant might add or remove a `vehicle`. Variants should be able to override data in the global `dict` as needed as well. 

# Middleware shift: MOOS -> Goby3

We want to do away with the MOOS middleware in place of using Goby3. This could be done at several levels: network protocol mimic, CMOOSApp mimic, or partial application rewrite. We will want to evalaute the pro/cons of each to minimize risk of regression and minimize the amount of new code to be written.

## Partial Application rewrite

For applications that already using GobyMOOSApp, rewrite the pub/sub to use the native Goby3 applications. When using Protobuf directly use the Goby support for protobuf.

## CMOOSApp mimic

For applications that do not use the GobyMOOSApp, recompile around a stub CMOOSApp that provides an interface to goby::middleware:SingleThreadApplication. CMOOSMsg can be converted to a single Protobuf type, with a Group (static if possible, otherwise dynamic) for each MOOS variable type.
 
# LAMSS Tool

We want a `lamss` command line tool in the style of modern CLIs that can take subtools as needed. The lamss tool should install itself as an alias to `lm` if that shortcut isn't already in use on the user's shell.

I would prefer that the `lamss` tool be built in C++ but this requires a bit of bootstrapping.

Each tool should have an implicit help (if subcommands are required) and `--help`.

## Build

- Full: `lamss build`
- Shortcut: `lm b`
- Job: Configure (CMake) and build the project (Ninja or Make)

## Dependencies

- Full: `lamss dependencies list|install`
- Shortcut: `lm dep list|install`
- Job: List or install dependencies for configured toolboxes

## Toolbox

- Full: `lamss toolbox list|add|remove|activate|deactivate`
- Shortcut: `lm tb `
- Job: List, add/remove external (git submodule) repositories to the local index, activate/deactivate toolboxes (add or remove them from the build and mission search)

## Mission

- Full `lamss mission run|sim <cruise> [--generate] [--execute] [--platform platform] [--subsystem ss]`
- Shortcut `lm m`
- Job: Generate configuration for and execute the desired cruise for all vehicles in simulation or a given platform. Runtime requires the platform. If subsystem is given, only that subsystem is run. If either --generate or --execute or given, only those stages are performed (other generate then execute is implied).
 
# Technical details

## Repository Layout

`lamss3`
 - `build`: Build artifacts
 - `build/usr`: Installed artifacts (mirrors `/usr`)
 - `<toolbox>`: core, autonomy, sonar, virtual_ocean, gui (and optionally any git submodule toolboxes)
	 + `<src>`: Source code for the toolbox
		 * `bin`: C++ Binaries (Goby applications)
		 * `lib`: C++ Libraries
		 * `python`: Python support modules
		 * `sh`: Installed shell scripts
	 + `mission`: Mission configuration for the toolbox.
		 * `build`: Mission build artifacts
		 * `log`: Mission persistant logs
		 * `run`: Mission temporary files
		 * `cruise`
			 - `cruise.py`
			 - `mbat`
				 + `mbat.py`
			 - `<subclassN or instantiationN>`
				 + 	`<subsubclassM or instantiationM>`
		 * `fleet`
			 - `fleet.py`
			 - `mit_bf21`
				 + `mit_bf21.py`
			 - `<subclassN or instantiationN>`
				 + 	`<subsubclassM or instantiationM>`
		 * `component`
			 - `component.py`
			 - `ivp`
				 + `ivp.py`
			 - `acomms`
				 + `acomms.py`
				 + `micromodem`
					 * `micromodem.py`
			 - `<subclassN or instantiationN>`
				 + 	`<subsubclassM or instantiationM>`
		 * `environment`
			 - `environment.py`
			 - `global`
				 * `global.py`
				 + `atlantic`
					 * `atlantic.py`
					 * `mass_bay`
						 - `mass_bay.py`
				 + `pacific`
				 + `...`
			 - `<subclassN or instantiationN>`
				 + 	`<subsubclassM or instantiationM>`

## Reformatting code

I suggest we settle on a clang-format style and develop the appropriate .clang-format file (e.g., https://github.com/GobySoft/goby3/blob/3.0/.clang-format), and then run clang-format on the entire project.

## Splitting up Protobuf messages

The existing monolithic lib_lamss_protobuf should be split across the new toolboxes as appropriate.

