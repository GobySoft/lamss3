---
title: uSimCTD
toolbox: virtual_ocean
kind: moos-app
source: virtual_ocean/src/bin/uSimCTD/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# uSimCTD

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

uSimCTD simulates a CTD sensor: it loads a table of CTD casts (longitude, latitude, depth, sound
speed, temperature, salinity) and publishes the sound speed, temperature and salinity of the table
entry nearest the vehicle's `NAV_LONG`/`NAV_LAT`/`NAV_DEPTH` **[code]**. The table comes from
historical casts compiled by MIT MSEAS (MODS files) for the operating area and season
(`README.md`) **[code]**. Its output stands in for a real CTD in simulation, for apps that need the
local sound speed (beamforming, acoustic ranging) **[guess]**.

## Interfaces

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `NAV_LONG` | MOOS | double, deg | `uSimCTD.cpp`, `OnNewMail` | vehicle longitude **[code]** | default -71.0 until received **[code]** |
| `NAV_LAT` | MOOS | double, deg | `uSimCTD.cpp`, `OnNewMail` | vehicle latitude **[code]** | default 41.0 until received **[code]** |
| `NAV_DEPTH` | MOOS | double, m | `uSimCTD.cpp`, `OnNewMail` | vehicle depth, positive down **[code]** | default 10.0 until received **[code]** |
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
| `CTD1` | MOOS | string: `LongLat(<lon>,<lat>)deg, Depth(<d>)m, C=<c>m/s, T=<t>degC, S=<s>PSU` | `uSimCTD.cpp` | human-readable summary: vehicle position and the matched C, T, S **[code]** | printed with 2 to 5 significant digits, so C=1490 prints as `1.5e+03` **[code]**; consumer unknown |
| `CTD_SOUND_VELOCITY` | MOOS | double, m/s | `uSimCTD.cpp` | sound speed of the nearest table entry **[code]** | every `Iterate` **[code]**; `1490` for the smoke test's first row **[test]** |
| `CTD_TEMPERATURE` | MOOS | double, °C | `uSimCTD.cpp` | temperature of the nearest table entry **[code]** | every `Iterate` |
| `CTD_SALINITY` | MOOS | double, PSU | `uSimCTD.cpp` | salinity of the nearest table entry **[code]** | every `Iterate` |
<!-- END GENERATED publishes -->

### Configuration

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
| `CTD_Txt_File` | string |  | one of the two | XYZCTS text file: six whitespace-separated columns per line, lon lat depth c t s **[code]** |
| `CTD_Bin_File` | string |  | one of the two | XYZCTS binary file: six float64 per point, native byte order **[code]**; used only when the text file is not given or cannot be opened **[code]** |
<!-- END GENERATED config -->

Example (from `uSimCTD.moos`):

```
ProcessConfig = uSimCTD
{
  AppTick   = 5
  CommsTick = 5

  // define either CTD_Txt_File (in XYZCTS text format) or CTD_Bin_File (in XYZCTS binary format)
  // if specified file is missing, uSimCTD returns error

  CTD_Txt_File = CTD.xyzcts
  // CTD_Bin_File = CTD.xyzcts.bin

  // if both files are defined here,
  //   uSimCTD first searches for CTD_Txt_File & uses it if it is available
  //   else it continues searching for CTD_Bin_File & uses it if is available
  //   in event that both specified files are missing, uSimCTD returns error
}
```

### Files, devices and external programs

- Reads `CTD_Txt_File` or `CTD_Bin_File` once at start-up into memory and closes it **[code]**.
  `CTD.xyzcts` in the source directory is a four-line example **[code]**.
- The README names a MATLAB converter from MODS files (`tools/uMODSReadConvert.m` in the old lamss
  repository), which is not ported **[code]**.

## Behaviour

- `OnStartUp` chooses the file: text if only `CTD_Txt_File` is set, binary if only `CTD_Bin_File`
  is set, and if both are set the text file when it opens, else the binary file **[code]**. With
  neither, or when the chosen file does not open, start-up fails and the app exits **[code]**.
- The text file's point count is its line count; the binary file's is its size divided by 48 bytes
  **[code]**.
- Every `Iterate` (`AppTick`) finds the table entry with the smallest squared distance
  (Δlon² + Δlat² + Δdepth²) to the vehicle and publishes its C, T and S **[code]**. Degrees and
  metres are mixed without scaling and there is no interpolation, as the code comment says
  **[code]**; in practice depth dominates and the nearest depth is chosen almost regardless of
  position **[guess]**.
- It publishes on every iteration whether or not navigation has been received, using the
  constructor's default position until then **[code]**.

## Running it

- No mission component launches it yet.
- Standalone, in `virtual_ocean/src/bin/uSimCTD/`: run `MOOSDB uSimCTD.moos`, then
  `uSimCTD uSimCTD.moos`, and post `NAV_LAT`, `NAV_LONG`, `NAV_DEPTH`.
- Tested by `lamss_virtual_ocean_moos_smoke`: with a three-row table, posting `NAV_LAT=42.35`,
  `NAV_LONG=-71.10`, `NAV_DEPTH=0` gives `CTD_SOUND_VELOCITY` 1490 **[test]**.

## Known issues and open questions

- `NDataPoint` and `dfCTD_DB` are not initialised in the constructor, and the destructor loops over
  them **[code]**. When `OnStartUp` fails (no file configured or file missing), the app exits
  through the destructor with uninitialised values (undefined behaviour) **[code]**.
- An empty table (empty text file, or binary file under 48 bytes) gives `NDataPoint` 0, and
  `GetCtsfrLongLatDepth` then reads `dfCTD_DB[0]` out of bounds **[code]**.
- Blank lines in the text file are counted as points; the failed `>>` leaves those rows
  uninitialised **[code]**.
- `CTD1` uses `setprecision` without `std::fixed`, so values are printed with that many
  significant digits (`C=1.5e+03m/s`) **[code]**. Is anything reading `CTD1`, or can it go?
- The distance mixes degrees and metres (see Behaviour). Should the horizontal terms be scaled to
  metres, or the nearest cast chosen first and then the nearest depth?
- `dfLongMatch`/`dfLatMatch`/`dfDepthMatch` are only used in a trace message, and the constructor's
  defaults for them have latitude and longitude swapped **[code]**.

## History

- From GobySoft/lamss `src/moos/uSimCTD`; first commit 2011-04-26 (Toby Schneider), when the MOOS
  processes used in missions-lamss were added. Raymond Lum rewrote it to read XYZCTS text and
  binary files in August 2011. 5 commits before the port (Toby Schneider, Raymond Lum); the last,
  2013-10-11, updated it for MOOS V10.
- lamss3 port (GobySoft/lamss3#23): `readme.txt` became `README.md`; the `data/` directory (example
  MODS/XYZCTS files and plots) and the MATLAB `tools/` were left out, and a small `CTD.xyzcts` and
  `uSimCTD.moos` were added; builds without Goby (`NO_GOBY`).
