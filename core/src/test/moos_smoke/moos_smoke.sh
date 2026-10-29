#!/bin/bash
# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Runs MOOSDB, gobyd, goby_moos_gateway with the lamss plugin and the core MOOS apps together, and
# checks that data passes through each of them. Tool and target paths come from the environment
# (see CMakeLists.txt). Every process gets this run's own ports and Goby platform, so parallel runs
# and other MOOS communities on the machine are left alone.

set -euo pipefail

work=$(mktemp -d "${TMPDIR:-/tmp}/lamss_moos_smoke.XXXXXX")
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

base=$((20000 + ($$ % 4000) * 10))
db_port=$base
remote_db_port=$((base + 1))
line_port=$((base + 2))
platform="lamss_moos_smoke_$$"
interprocess="platform: \"$platform\" socket_name: \"$work/gobyd.manager\""

start() {
    local name=$1
    shift
    (cd "$work" && exec "$@" >"$work/$name.log" 2>&1) &
    pids+=($!)
}

step() { printf '\n== %s\n' "$1"; }

probe() { "$LAMSS_MOOS_PROBE" "$db_port" 10 "$@"; }

write_moos() {
    local name=$1 port=$2 body=$3
    cat >"$work/$name.moos" <<EOF
ServerHost = localhost
ServerPort = $port
Community  = smoke
LatOrigin  = 42.358
LongOrigin = -71.087

$body
EOF
}

write_moos db "$db_port" ""
write_moos remote_db "$remote_db_port" ""
start MOOSDB "$LAMSS_MOOSDB" "$work/db.moos" --moos_port="$db_port"
start MOOSDB_remote "$LAMSS_MOOSDB" "$work/remote_db.moos" --moos_port="$remote_db_port"
start gobyd "$LAMSS_GOBYD" --interprocess "$interprocess"

cat >"$work/gateway.pb.cfg" <<EOF
interprocess { $interprocess }
moos { server: "localhost" port: $db_port }
[lamss.core.goby_gateway_config] { }
EOF
GOBY_MOOS_GATEWAY_PLUGINS="$LAMSS_GATEWAY_PLUGIN" \
    start goby_moos_gateway "$LAMSS_GOBY_MOOS_GATEWAY" "$work/gateway.pb.cfg"

write_moos iSerialNMEA "$db_port" 'ProcessConfig = iSerialNMEA
{
  vehicle_name: "ship"
  vehicle_type: "ship"
  input: INPUT_MOOSDB
  raw_var_to_read: "SHIP_NMEA_IN"
  out_status_var: "NODE_REPORT_NMEA_SHIP"
}'
start iSerialNMEA "$LAMSS_ISERIALNMEA" "$work/iSerialNMEA.moos"

write_moos pViewMessage "$db_port" 'ProcessConfig = pViewMessage
{
  generate_view_objects: true
  destination: 1
}'
start pViewMessage "$LAMSS_PVIEWMESSAGE" "$work/pViewMessage.moos"

write_moos iLineBasedComms "$db_port" "ProcessConfig = iLineBasedComms
{
  write_to_var: \"LINE_IN\"
  connection_type: CONNECTION_TCP_AS_SERVER
  tcp_port: $line_port
}"
start iLineBasedComms "$LAMSS_ILINEBASEDCOMMS" "$work/iLineBasedComms.moos"

write_moos pMOOSLink "$db_port" "ProcessConfig = pMOOSLink
{
  AppTick    = 4
  CommsTick  = 4
  remote_server = localhost
  remote_port = $remote_db_port
  out = SMOKE_OUT:SMOKE_REMOTE_IN
  in = SMOKE_REMOTE_OUT:SMOKE_IN
}"
start pMOOSLink "$LAMSS_PMOOSLINK" "$work/pMOOSLink.moos"

failed=1

step "Every app connects"
probe '?DB_CLIENTS~iSerialNMEA' '?DB_CLIENTS~pViewMessage' '?DB_CLIENTS~iLineBasedComms' \
    '?DB_CLIENTS~pMOOSLink'
"$LAMSS_MOOS_PROBE" "$remote_db_port" 10 '?DB_CLIENTS~pMOOSLink'

step "iSerialNMEA: NMEA sentence to node report"
# shellcheck disable=SC2016 # a literal NMEA sentence
probe 'SHIP_NMEA_IN=$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47' \
    '?NODE_REPORT_NMEA_SHIP~NAME=ship'

step "pViewMessage: view objects out and back"
out_view_point=$(probe MISSION_STATE=DEPLOY VEHICLE_NAME=smoke \
    'VIEW_POINT=x=10,y=20,label=smoke_waypoint' '?OUT_VIEW_POINT~vehicle_name=smoke')
echo "$out_view_point"
probe "IN_VIEW_POINT=${out_view_point#*]: }" '?VIEW_POINT~x=10,y=20'

step "iLineBasedComms: TCP line to LINE_IN"
for _ in $(seq 50); do
    if exec 3<>"/dev/tcp/127.0.0.1/$line_port" 2>/dev/null; then break; fi
    sleep 0.1
done
probe '?LINE_IN~lamss smoke line' &
probe_pid=$!
sleep 1
printf 'lamss smoke line\r\n' >&3
wait "$probe_pid"
exec 3>&-

step "pMOOSLink: both directions between two MOOSDBs"
"$LAMSS_MOOS_PROBE" "$remote_db_port" 10 '?SMOKE_REMOTE_IN~outbound' &
probe_pid=$!
sleep 1
probe SMOKE_OUT=outbound
wait "$probe_pid"
probe '?SMOKE_IN~inbound' &
probe_pid=$!
sleep 1
"$LAMSS_MOOS_PROBE" "$remote_db_port" 10 SMOKE_REMOTE_OUT=inbound
wait "$probe_pid"

step "goby_moos_gateway: NODE_REPORT from MOOS to Goby"
"$LAMSS_GOBY" zeromq subscribe --interprocess "$interprocess" \
    -l "$LAMSS_CORE_PROTOBUF" -g '^lamss::moos::NODE_REPORT$' >"$work/subscribe.log" 2>&1 &
pids+=($!)
sleep 1
probe 'NODE_REPORT=NAME=smoke,TYPE=auv,UTC_TIME=1793277319,X=10,Y=20,SPD=1,HDG=90,DEPTH=5'
for _ in $(seq 50); do
    grep -q 'name: "smoke"' "$work/subscribe.log" && break
    sleep 0.1
done
grep 'name: "smoke"' "$work/subscribe.log"

step "goby_moos_gateway: NODE_REPORT from Goby to MOOS"
probe '?NODE_REPORT~=topside' &
probe_pid=$!
sleep 1
"$LAMSS_GOBY" zeromq publish --interprocess "$interprocess" -l "$LAMSS_CORE_PROTOBUF" \
    'lamss::moos::NODE_REPORT::out' PROTOBUF/lamss.core.LegacyNodeReport 'name: "topside" x: 1 y: 2'
wait "$probe_pid"

failed=0
echo
echo "MOOS smoke test passed"
