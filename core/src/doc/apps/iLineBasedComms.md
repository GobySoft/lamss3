---
title: iLineBasedComms
toolbox: core
kind: goby-moos-app
source: core/src/bin/iLineBasedComms/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# iLineBasedComms

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

iLineBasedComms connects the MOOSDB to a line-based link: a serial port, a TCP client or a TCP
server **[code]**. In one mode it passes text lines between the link and two MOOS variables; in the
other it sends and receives whole MOOS messages, hex-encoded one per line **[code]**. The second
mode lets two MOOS communities exchange chosen variables over a modem or radio link, like
pMOOSBridge **[guess]** (a 2013 commit calls it "a bit like MOOSBridge"). No mission component in
lamss3 runs it yet **[code]**.

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `cfg_.bridge(i).local_var()` (built at run time) | MOOS | any (string or double) | `handle_bridge` | each `bridge.local_var` **[code]** | sent whole (key, value, source, time) as a hex-encoded MOOS message, once per `remote_var` it maps to **[code]**; dropped with a warning while the link is down (except as TCP server) **[code]** |
| `cfg_.read_from_var()` (built at run time) | MOOS | string | `handle_out` | the `read_from_var` parameter: text to write to the link **[code]** | written as-is plus `\r\n` **[code]**; a double value is written as an empty line **[guess]** (only `GetString()` is read) |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `cfg_.write_to_var()` (built at run time) | MOOS | string | `iLineBasedComms.cpp` | the `write_to_var` parameter: each line received, whitespace-trimmed **[code]** | only when `write_to_var` is set **[code]**; a line sent to the TCP server appears here **[test]** |
| `msg` (built at run time) | MOOS | any | `iLineBasedComms.cpp` | not a variable name: `publish(msg)` posts a received hex-encoded MOOS message under the key it carries **[code]** | only when `write_to_var` is not set **[code]**; the far side chooses the variable name **[code]** |
<!-- END GENERATED publishes -->

### Configuration

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `common` | GobyMOOSAppConfig |  | no | standard Goby MOOS app settings (`app_tick`, `comm_tick`, log) **[code]** |
| `write_to_var` | string |  | no | variable for received lines **[code]**; if unset, every received line is decoded as a hex MOOS message instead **[code]** |
| `read_from_var` | string |  | no | variable whose string values are written to the link **[code]** |
| `connection_type` | ConnectionType | `CONNECTION_SERIAL` | no | `CONNECTION_SERIAL`, `CONNECTION_TCP_AS_CLIENT` or `CONNECTION_TCP_AS_SERVER` **[code]**; `CONNECTION_DUAL_UDP_BROADCAST` stops the app with "Unsupported connection type" **[code]**. The proto description says "to the GPS", copied from iSerialNMEA **[guess]** |
| `serial_port` | string |  | no | serial device for `CONNECTION_SERIAL` **[code]** |
| `serial_baud` | uint32 |  | no | baud rate for `CONNECTION_SERIAL` **[code]**; no default **[code]** |
| `tcp_server` | string |  | no | host to connect to for `CONNECTION_TCP_AS_CLIENT` **[code]** |
| `tcp_port` | uint32 |  | no | port to connect to (client) or listen on (server) **[code]** |
| `bridge` | repeated BridgePair |  | no | pairs of `local_var` (required) and `remote_var` (defaults to `local_var`) **[code]**; a `local_var` may appear in several pairs **[code]** |
<!-- END GENERATED config -->

Example (from `iLineBasedComms.moos`):

```
ProcessConfig = iLineBasedComms
{
  write_to_var: "LINE_IN"
  read_from_var: "LINE_OUT"
  connection_type: CONNECTION_TCP_AS_SERVER
  tcp_port: 50010
}
```

A bridging configuration would leave out `write_to_var` and add `bridge { local_var: "A" remote_var: "B" }`
blocks **[code]**. No example in lamss3 does this.

### Files, devices and external programs

- A serial port (`goby::util::SerialClient`), a TCP client or a TCP server (`goby::util::TCPClient`,
  `TCPServer`), chosen by `connection_type` **[code]**. The link is opened in the constructor **[code]**.
- Line format on the link: lines end in `\r\n` on write; on read, surrounding whitespace is
  trimmed **[code]**. In bridge mode each line is a MOOS message serialised by
  `goby::moos::MOOSSerializer` and hex-encoded **[code]**.

## Behaviour

- Each `loop()` (Goby `app_tick`) reads every line waiting on the link **[code]**.
  - With `write_to_var` set, each trimmed line is posted to it as a string **[code]**.
  - Without it, each line is hex-decoded, parsed as a MOOS message and posted under that
    message's own key **[code]**.
- Outgoing text: each `read_from_var` mail is written to the link with `\r\n` appended **[code]**. It is
  written even when the link is not active **[code]**.
- Outgoing bridge: each `bridge.local_var` mail is copied, renamed to its `remote_var`, serialised,
  hex-encoded and written **[code]**. The two ends are symmetric: a second iLineBasedComms without
  `write_to_var` decodes the lines and posts them as `remote_var` **[guess]**.
- Reading and bridging go on only while the link is active, except for the TCP server, which always
  tries **[code]**. The reason is probably that a server with no client yet reports itself inactive
  **[guess]**.
- The modes can be mixed: `read_from_var` text and `bridge` messages go out on the same link, but
  received lines are handled in one mode only, set by whether `write_to_var` is present **[code]**.

## Running it

- No mission component launches it yet (`launched_by: []`).
- Standalone, from `core/src/bin/iLineBasedComms/`: `MOOSDB iLineBasedComms.moos`, then
  `iLineBasedComms iLineBasedComms.moos`. Connect with `nc localhost 50010`: lines typed there appear in
  `LINE_IN`, and `LINE_OUT` posts are written back **[code]**.
- Tested by `lamss_core_moos_smoke` (`core/src/test/moos_smoke/moos_smoke.sh`): TCP server mode, a line
  written to the port appears in `LINE_IN` **[test]**. Bridge mode and `read_from_var` have no test.

## Known issues and open questions

- Bridge mode lets whatever is on the far end of the link post any MOOS variable locally, under any
  name **[code]**. Is that acceptable on a radio link, or should incoming keys be checked against a
  list?
- Bridge mode with `write_to_var` set is one-way: messages go out hex-encoded, but incoming lines are
  posted as text, not decoded **[code]**. Is that intended?
- A line that is not valid hex or not a serialised MOOS message is passed to `CMOOSMsg::Serialize`
  unchecked **[code]**. What happens then (exception, garbage message) depends on MOOS and was not
  checked.
- `read_from_var` mail is written even when the link is not active; bridge mail is not **[code]**.
  Should they behave the same?
- `serial_baud` has no default, so a serial configuration without it opens the port at 0 baud **[guess]**.
- Goby3's `goby::middleware::io` line-based threads may replace this app (plan
  [01](../plan/claude/01-source-inventory.md)) **[guess]**.

## History

- From GobySoft/lamss. The imported history shows only lamss3 paths; the first commit, 2011-04-20 by
  Toby Schneider, is "initial structure with moos-ivp-local/src/tes apps", so it probably started in
  `moos-ivp-local/src/tes` **[guess]**.
- 7 commits before the port: 6 by Toby Schneider, 1 from a shared lab account (2013, the bridge
  feature). Last change before the port: "Updates for Goby3" (2019-07-01).
- lamss3 port (GobySoft/lamss3#9–#12, 2026-10-29): clang-format, build with `lamss_add_app`, config
  proto moved to `package lamss.core`, and the example `.moos` file reduced to a runnable TCP server
  configuration. No interface change.
