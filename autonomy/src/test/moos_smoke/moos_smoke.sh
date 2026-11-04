#!/bin/bash
# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Runs a MOOSDB with the autonomy MOOS apps and checks that each connects and does its job.
# Tool and target paths come from the environment (see CMakeLists.txt). The MOOSDB gets this
# run's own port, so parallel runs and other MOOS communities on the machine are left alone.

set -euo pipefail

work=$(mktemp -d "${TMPDIR:-/tmp}/lamss_autonomy_moos_smoke.XXXXXX")
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

db_port=$((24000 + ($$ % 4000)))

start() {
    local name=$1
    shift
    (cd "$work" && exec "$@" >"$work/$name.log" 2>&1) &
    pids+=($!)
}

step() { printf '\n== %s\n' "$1"; }

probe() { "$LAMSS_MOOS_PROBE" "$db_port" 15 "$@"; }

write_moos() {
    local name=$1 body=$2
    cat >"$work/$name.moos" <<EOM
ServerHost = localhost
ServerPort = $db_port
Community  = smoke
LatOrigin  = 42.358
LongOrigin = -71.087

$body
EOM
}

write_moos db ""
start MOOSDB "$LAMSS_MOOSDB" "$work/db.moos" --moos_port="$db_port"

write_moos pLamssMissionManager 'ProcessConfig = pLamssMissionManager
{
  common { app_tick: 4 comm_tick: 10 log: false }
  topside_name: "topside"
  initial_deploy {
    Deploy_Mode: DEPLOY_MODE_LOITER
    Deploy_X: 0
    Deploy_Y: 0
    Deploy_Speed: 1.8
    Deploy_Upper_Depth: 15
  }
}'
start pLamssMissionManager "$LAMSS_PLAMSSMISSIONMANAGER" "$work/pLamssMissionManager.moos"

write_moos pTetraHoming 'ProcessConfig = pTetraHoming
{
  AppTick = 4
  CommsTick = 4
  RETURN_UPDATE_VAR = HOMING_CONFIG
  ABORT_UPDATE_VAR = ABORT_HEADING
}'
start pTetraHoming "$LAMSS_PTETRAHOMING" "$work/pTetraHoming.moos" --alias=pTetraHoming

write_moos pGateway 'ProcessConfig = pGateway
{
  AppTick = 4
  CommsTick = 4
  deploy_latitude = 42.36
  deploy_longitude = -71.08
  deploy_depth = 10
}'
start pGateway "$LAMSS_PGATEWAY" "$work/pGateway.moos"

write_moos uMVS_Bluefin 'ProcessConfig = uMVS_Bluefin
{
  AppTick = 10
  CommsTick = 10
  ADD_AUV = pose=[4x1]{0,0,-1,90},name=smoke,InputPrefix=,OutputPrefix=FRONTSEAT_
  TideHeight = 0
  InstantLogAcoustics = false
}'
start uMVS_Bluefin "$LAMSS_UMVS_BLUEFIN" "$work/uMVS_Bluefin.moos"

failed=1

step "Every app connects"
probe '?DB_CLIENTS~pLamssMissionManager' '?DB_CLIENTS~pTetraHoming' '?DB_CLIENTS~pGateway' \
    '?DB_CLIENTS~uMVS_Bluefin'

step "pLamssMissionManager: LAMSS_DEPLOY for this vehicle starts DEPLOY"
probe '?MISSION_STATE~ALLSTOP' '?HOMING_X'
probe 'VEHICLE_ID#=2' '?VEHICLE_ID'
probe 'LAMSS_DEPLOY_IN=@PB[lamss.autonomy.LAMSS_DEPLOY] _rcv_id: 2 _src_id: 1 Deploy_Mode: DEPLOY_MODE_LOITER Deploy_X: 100 Deploy_Y: 100 Deploy_Speed: 1.8 Deploy_Upper_Depth: 15' \
    '?MISSION_STATE~DEPLOY'

step "pTetraHoming: without a beacon fix, home on the mission manager's beacon estimate"
probe 'BEACON_UPDATE=beacon_x=30,beacon_y=40' DEPLOY_MISSION=RETURN \
    '?HOMING_STATION~point=30,40' '?HOMING_CONFIG~center_assign = 30,40' '?HOMING_LOCK~FALSE'

step "pGateway: fixed ship node report"
probe '?NODE_REPORT_NMEA_SHIP~TYPE=ship' '?NAV_DEPTH'

step "uMVS_Bluefin: vehicle state under thrust"
probe 'DESIRED_THRUST#=50' '?FRONTSEAT_DEPTH' '?FRONTSEAT_SPEED'

failed=0
echo
echo "autonomy MOOS smoke test passed"
