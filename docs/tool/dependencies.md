# lamss dependencies

*This page written by Claude*

`lamss dependencies` (`lm dep`) lists and installs the system packages the active toolboxes need. Each toolbox declares them in `dependencies.yml`:

```yaml
apt:
  all: [libgoby3-dev, libdccl5-dev, protobuf-compiler]   # every release
  jammy: [libnetcdf-cxx-legacy-dev]                       # added on 22.04
  noble: [libnetcdf-c++4-dev]
apt_repositories:
  - name: gobysoft
    url: http://packages.gobysoft.org/ubuntu/continuous/
    key: cmake/keys/gobysoft.asc
source:
  - name: rtd-dm7820-dkms
    script: src/sh/lamss_install_dm7820.sh
```

```sh
lm dep list                      # the union for the active toolboxes on this release
lm dep list --all-distros        # every release
lm dep list --format apt         # one package per line, for scripts
lm dep install                   # add repositories, apt-get install, run source scripts
lm dep install --dry-run         # print the commands only
lm dep install --toolbox sonar   # only sonar and what it requires
```

`install` is the only command that uses `sudo`, and it prints each command first. Repositories are added with their key under `/etc/apt/keyrings/`. The GobySoft repository is the `continuous` one, which carries current Goby3 and DCCL builds.
