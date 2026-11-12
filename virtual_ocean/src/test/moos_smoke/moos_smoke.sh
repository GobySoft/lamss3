#!/bin/bash
# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Starts every virtual_ocean MOOS app against a private MOOSDB and checks that each connects; the
# environment services (iNoaaBathy, uSimCTD, uSimGPS) also get one input and must answer. Paths
# come from the environment (see CMakeLists.txt); LAMSS_VO_NETSIM_APPS lists the netsim-only apps
# that were built. Ports and the community are this run's own.

set -euo pipefail

work=$(mktemp -d "${TMPDIR:-/tmp}/lamss_vo_moos_smoke.XXXXXX")
pids=()
cleanup() {
    for pid in "${pids[@]}"; do kill "$pid" 2>/dev/null || true; done
    wait 2>/dev/null || true
    if [ "${failed:-1}" -eq 1 ]; then
        for log in "$work"/*.log; do
            echo "==== $(basename "$log")"
            tail -n 40 "$log"
        done
    fi
    rm -rf "$work"
}
trap cleanup EXIT

base=$((20005 + ($$ % 4000) * 10))
db_port=$base
phins_port=$((base + 1))
netsim_manager_port=$((base + 2))
modem_port=$((base + 3))
lat0=42.358
lon0=-71.087

# MOOS-IvP template apps register under argv[0]
start() {
    local name=$1 exe=$2
    shift 2
    (cd "$work" && exec -a "$name" "$exe" "$@" >"$work/$name.log" 2>&1) &
    pids+=($!)
}

step() { printf '\n== %s\n' "$1"; }

probe() { "$LAMSS_MOOS_PROBE" "$db_port" 20 "$@"; }

"$LAMSS_MAKE_FLAT_BATHY" "$work/flat.nc" "$lat0" "$lon0" 100
# lon lat depth sound-speed temperature salinity
printf -- '-71.10 42.35 0 1490 10 31\n-71.10 42.35 50 1485 8 32\n-71.08 42.36 0 1491 11 31\n' \
    >"$work/ctd.xyzcts"
mkdir -p "$work/out"

cat >"$work/smoke.moos" <<EOF
ServerHost = localhost
ServerPort = $db_port
Community  = vo_smoke
LatOrigin  = $lat0
LongOrigin = $lon0

ProcessConfig = iNoaaBathy
{
  nc_bathy_file = $work/flat.nc
  publish_interval = 1
}

ProcessConfig = uSimCTD
{
  CTD_Txt_File = $work/ctd.xyzcts
}

ProcessConfig = uSimGPS
{
  gps_depth = 2.0
  gps_interval = 1
  min_gps_time = 0
}

ProcessConfig = uSimDVL
{
  dvl_update_frequency = 1
}

ProcessConfig = uSimPhinsINS
{
  phins_ip = 127.0.0.1
  phins_port = $phins_port
}

ProcessConfig = uSimAcoBeaconRngBrg
{
  BEACON_X = 0
  BEACON_Y = 0
}

ProcessConfig = uSimNoiseCovariance
{
  water_depth = 100
  noise_level = 60
  minimum_frequency = 800
  maximum_frequency = 1000
  isotropic_noise = true
  array_type = DURIP
  num_array_elements = 4
  sampling_frequency = 4000
  num_samples = 8000
  base_dir_name = $work/out/
}

ProcessConfig = uSimTowedArray
{
  cond_check = 1
  start_speed = 0.5
  filter_length = 40
  array_type = 2
  number_sectors = 3
  sector_1 = 20, 20.0, .0000, .0095, .001, 0.2, 1.8e9
  sector_2 = 30, 36.0, .0000, .035, .0024, 0.5, 1.8e8
  sector_3 = 20, 20.0, .0000, .0095, .001, 0.2, 1.8e9
  tow_body = 0
  towbody_parameters = -10.0, 1.0, 0.01
  array_section_number = 2
  first_phone_offset = 3.0
  array_definition_type = 0
  number_elements = 4
  element_spacing = 1.5
}

ProcessConfig = uSimPassiveSonar
{
  num_array_elements: 4
  sampling_frequency: 4000
  num_samples: 4000
  base_dir_name: "$work/out/"
}

ProcessConfig = uSimTargets
{
}

ProcessConfig = uSimModemNetwork
{
  base_dir_name: "$work/out/"
}

ProcessConfig = iNetSimManager
{
  netsim_manager_ip_addr: "127.0.0.1"
  netsim_manager_tcp_port: $netsim_manager_port
  modem_tcp_port: $modem_port
  nav_source: NODE_REPORT
}
EOF

failed=1

step "GobyMOOSApps print their configuration"
for app in uSimPassiveSonar uSimTargets ${LAMSS_VO_NETSIM_APPS:-}; do
    exe_var="LAMSS_$(echo "$app" | tr '[:lower:]' '[:upper:]')"
    example=$("${!exe_var}" -e)
    grep -q "ProcessConfig = $app" <<<"$example"
    echo "$app -e: ok"
done

(cd "$work" && exec "$LAMSS_MOOSDB" "$work/smoke.moos" --moos_port="$db_port" >"$work/MOOSDB.log" 2>&1) &
pids+=($!)

apps=(iNoaaBathy uSimCTD uSimGPS uSimDVL uSimPhinsINS uSimAcoBeaconRngBrg uSimNoiseCovariance
    uSimTowedArray uSimPassiveSonar uSimTargets ${LAMSS_VO_NETSIM_APPS:-})
for app in "${apps[@]}"; do
    exe_var="LAMSS_$(echo "$app" | tr '[:lower:]' '[:upper:]')"
    start "$app" "${!exe_var}" "$work/smoke.moos"
done

step "Every app connects"
expect=()
for app in "${apps[@]}"; do expect+=("?DB_CLIENTS~$app"); done
probe "${expect[@]}"

step "iNoaaBathy: depth of the flat grid"
probe '?BATHY_Z~-100'

step "uSimCTD: sound speed at the vehicle"
probe NAV_LAT=42.35 NAV_LONG=-71.10 NAV_DEPTH=0 '?CTD_SOUND_VELOCITY~1490'

step "uSimGPS: a fix at the surface"
probe NAV_DEPTH=0 TRUE_X=10 TRUE_Y=20 '?GPS_X'

failed=0
echo
echo "virtual_ocean MOOS smoke test passed"
