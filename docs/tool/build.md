# lamss build

*This page written by Claude*

`lamss build` (`lm b`) configures CMake when needed, builds every active toolbox and installs them into `build/usr`.

```sh
lm b                           # configure if needed, build, install
lm b --test                    # and run ctest
lm b --toolbox sonar           # only sonar's targets
lm b --type Debug              # CMAKE_BUILD_TYPE, remembered in build/
lm b -D LAMSS_WERROR=ON        # pass a CMake definition (reconfigures)
lm b --configure               # force a reconfigure
```

- **Reconfiguring** happens by itself when `.lamss/toolboxes` is newer than `build/CMakeCache.txt`, so activating a toolbox is enough.
- **Install prefix:** `build/usr` mirrors `/usr`. The shell environment from [`lamss env`](env.md) puts its `bin`, `lib` and Python directories on the search paths.
- **Parallelism:** `-j N` sets the build jobs; the default is the number of processors. On machines with less than about 2 GB of memory per processor, lower it (GobySoft/lamss3#33).
- **Output** is CMake's and Ninja's, streamed as it comes; the exit status is CMake's.

How the toolboxes fit into one CMake build is in [Build system](../developing/build-system.md). All options: [command reference](../reference/cli.md#lamss-build).
