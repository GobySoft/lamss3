#!/bin/bash
# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Starts each sonar MOOS app with its example .moos file and checks that it connects, then runs
# pLamssAEL -> pLamssCBF -> pLamssBearingTracker on a synthetic plane wave and checks the
# beamformer's peak and the tracker's detector. Tool and target paths come from the environment
# (see CMakeLists.txt). Every MOOSDB gets this run's own port, so parallel runs and other MOOS
# communities on the machine are left alone.

set -euo pipefail

work=$(mktemp -d "${TMPDIR:-/tmp}/lamss_sonar_moos_smoke.XXXXXX")
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

base=$((20000 + ($$ % 4000) * 10 + 5))
examples_port=$base
chain_port=$((base + 1))

start() {
    local name=$1
    shift
    (cd "$work" && exec "$@" >"$work/$name.log" 2>&1) &
    pids+=($!)
}

step() { printf '\n== %s\n' "$1"; }

probe() { "$LAMSS_MOOS_PROBE" "$@"; }

write_moos() {
    local name=$1 port=$2 body=$3
    cat >"$work/$name.moos" <<EOM
ServerHost = localhost
ServerPort = $port
Community  = sonar_smoke
LatOrigin  = 42.358
LongOrigin = -71.087

$body
EOM
}

failed=1

step "Every app starts from its example and connects"
write_moos examples_db "$examples_port" ""
start MOOSDB_examples "$LAMSS_MOOSDB" "$work/examples_db.moos" --moos_port="$examples_port"
for app in pLamssCBF pLamssAEL pLamssBearingTracker pGeoTracker; do
    sed "s/^ServerPort *=.*/ServerPort = $examples_port/" "$LAMSS_SONAR_BIN/$app/$app.moos" \
        >"$work/example_$app.moos"
done
start example_pLamssCBF "$LAMSS_PLAMSSCBF" "$work/example_pLamssCBF.moos"
start example_pLamssAEL "$LAMSS_PLAMSSAEL" "$work/example_pLamssAEL.moos"
start example_pLamssBearingTracker "$LAMSS_PLAMSSBEARINGTRACKER" \
    "$work/example_pLamssBearingTracker.moos"
start example_pGeoTracker "$LAMSS_PGEOTRACKER" "$work/example_pGeoTracker.moos"
probe "$examples_port" 15 '?DB_CLIENTS~pLamssCBF' '?DB_CLIENTS~pLamssAEL' \
    '?DB_CLIENTS~pLamssBearingTracker' '?DB_CLIENTS~pGeoTracker'

step "pGeoTracker: starts out of close range"
probe "$examples_port" 10 '?CLOSE_RANGE~FALSE'

# 16 elements 0.75 m apart (half a wavelength at 1 kHz); 31 beams equally spaced in cos(angle)
# from 1 to -1, so a wave from acos(1/3) = 70.53 degrees falls on beam 10 (counting from 0)
elements=16
spacing=0.75
samples=8192
sample_rate=4000
frequency=1000
bearing=70.5288
beams=31
expected_beam=10

write_moos chain_db "$chain_port" ""
start MOOSDB_chain "$LAMSS_MOOSDB" "$work/chain_db.moos" --moos_port="$chain_port"

write_moos pLamssAEL "$chain_port" 'ProcessConfig = pLamssAEL
{
  common { app_tick: 2 comm_tick: 5 }
  array_type: ARRAY_TYPE_FIXED
  is_mooring: true
  deploy_x: 100
  deploy_y: 200
  deploy_depth: 50
  deploy_heading: 0
}'
start pLamssAEL "$LAMSS_PLAMSSAEL" "$work/pLamssAEL.moos"

write_moos pLamssCBF "$chain_port" "ProcessConfig = pLamssCBF
{
  common { app_tick: 4 comm_tick: 4 }
  array_type: DURIP
  sampling: EQUALLY_SPACED_WAVENUMBER
  array_configuration: UNIFORM_SPACING
  number_elements: $elements
  element_spacing: $spacing
  array_elements_used: \"1:$elements\"
  angular_steering_type: ARRAY_ANGLE
  number_beams: $beams
  number_samples: $samples
  sampling_frequency: $sample_rate
  fft_size: 1024
  overlap: 0.5
  integration_time: 1
  sound_speed: 1500
  processing_type: BROADBAND
  center_frequency: $frequency
  bandwidth: 200
}"
start pLamssCBF "$LAMSS_PLAMSSCBF" "$work/pLamssCBF.moos"

sed "s/^ServerPort *=.*/ServerPort = $chain_port/" \
    "$LAMSS_SONAR_BIN/pLamssBearingTracker/pLamssBearingTracker.moos" \
    >"$work/pLamssBearingTracker.moos"
start pLamssBearingTracker "$LAMSS_PLAMSSBEARINGTRACKER" "$work/pLamssBearingTracker.moos"

probe "$chain_port" 15 '?DB_CLIENTS~pLamssCBF' '?DB_CLIENTS~pLamssAEL' \
    '?DB_CLIENTS~pLamssBearingTracker'

step "pLamssAEL: moored array navigation"
probe "$chain_port" 10 '?AEL_NAV~ael_x: 100 ael_y: 200 ael_z: -50'

step "pLamssCBF: beam power peaks on the plane wave's beam"
probe "$chain_port" 10 SONAR_CONTROL=ON SONAR_PROCESS=ON '?BEAMFORMING_VARS_PB~number_beams: 31'
probe "$chain_port" 20 '?DETECTOR_STAT' >"$work/detector.out" &
detector_pid=$!
for frame in 1 2 3; do
    "$LAMSS_ARRAY_DATA" "$work/frame$frame.dat" $elements $spacing $samples $sample_rate \
        $frequency $bearing $frame
    probe "$chain_port" 10 "SONAR_FILENAME=$work/frame$frame.dat" "SONAR_FILE_NUM:=$frame" \
        '?BEAM_DATA' >"$work/beam_data$frame.out"
done
beam_data=$(sed -n 's/^BEAM_DATA \[pLamssCBF\]: //p' "$work/beam_data3.out" | tail -n 1)
peak=$(tr ',' '\n' <<<"$beam_data" | awk 'NR == 1 || $1 > max { max = $1; i = NR - 1 } END { print i }')
echo "BEAM_DATA: $beam_data"
echo "peak on beam $peak (expected $expected_beam)"
[ "$peak" -eq "$expected_beam" ]

step "pLamssBearingTracker: runs its detector on the beam data"
wait "$detector_pid"
cat "$work/detector.out"

failed=0
echo
echo "sonar MOOS smoke test passed"
