#!/bin/bash
# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
#
# usage: behaviors_load.sh <bhv file>
# Runs pHelmIvP with lamss_autonomy_behaviors (LAMSS_BEHAVIOR_DIR) on the given .bhv file and checks
# that the helm builds every behavior listed in LAMSS_BEHAVIORS (';'-separated) and starts driving.

set -euo pipefail

bhv=$1
work=$(mktemp -d "${TMPDIR:-/tmp}/lamss_autonomy_behaviors_load.XXXXXX")
pids=()
cleanup() {
    for pid in "${pids[@]}"; do kill "$pid" 2>/dev/null || true; done
    wait 2>/dev/null || true
    if [ "${failed:-1}" -eq 1 ]; then
        for log in "$work"/*.log; do
            echo "==== $(basename "$log")"
            tail -n 60 "$log"
        done
    fi
    rm -rf "$work"
}
trap cleanup EXIT

db_port=$((28000 + ($$ % 1000)))

# drop behaviors this build does not have (BHV_OptAcommsDepth needs netsim)
awk -v have=";$LAMSS_BEHAVIORS;" '
    /^Behavior *=/ { kind = $3; skip = index(have, ";" kind ";") == 0 }
    !skip { print }
    /^}/ { skip = 0 }' "$bhv" >"$work/smoke.bhv"

cat >"$work/helm.moos" <<EOM
ServerHost = localhost
ServerPort = $db_port
Community  = smoke

ProcessConfig = pHelmIvP
{
  AppTick = 4
  CommsTick = 4
  behaviors = smoke.bhv
  ivp_behavior_dir = $LAMSS_BEHAVIOR_DIR
  domain = course:0:359:360
  domain = speed:0:4:41
  domain = depth:0:500:501
  domain = buoyancy:-10:10:21
}
EOM

(cd "$work" && exec "$LAMSS_MOOSDB" --moos_port="$db_port" >"$work/MOOSDB.log" 2>&1) &
pids+=($!)
(cd "$work" && exec -a pHelmIvP "$LAMSS_PHELMIVP" helm.moos >"$work/pHelmIvP.log" 2>&1) &
pids+=($!)

failed=1
"$LAMSS_MOOS_PROBE" "$db_port" 20 MOOS_MANUAL_OVERRIDE=false '?IVPHELM_STATE~DRIVE'

grep -a -q "all_builds_ok: true" "$work/pHelmIvP.log"
IFS=';' read -ra behaviors <<<"$LAMSS_BEHAVIORS"
for b in "${behaviors[@]}"; do
    grep -a -q "load behavior library: .*$b.* SUCCESS" "$work/pHelmIvP.log" || {
        echo "pHelmIvP did not load $b"
        exit 1
    }
    grep -q "^Behavior = $b\$" "$work/smoke.bhv" || {
        echo "smoke.bhv does not use $b"
        exit 1
    }
done

failed=0
echo "pHelmIvP built ${#behaviors[@]} behaviors: ${behaviors[*]}"
