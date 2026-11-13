---
title: track
toolbox: sonar
kind: library
source: sonar/src/lib/track/
inferred_by: Claude
inferred_from: 992a171
status: inferred
reviewed_by:
reviewed_on:
launched_by: []
---

# track

!!! warning "Inferred by Claude"
    Written by Claude from the source at `992a171`, not from a design document or by the app's
    authors.
    Each statement below is tagged: **[code]** read from the code, **[test]** observed in a test or
    a run, **[guess]** inferred intent that nobody has confirmed. Corrections are welcome; a
    maintainer sets `status: reviewed` once the page has been checked.

## Purpose

`lamss_sonar_track` (`lamss::sonar_track`) is a small K&R C library of dense vector and matrix
routines on row-major `double` arrays: copy, fill, inner product, scaling, addition, matrix–vector
and matrix–matrix products, transpose, row operations, Gauss–Jordan inversion, and text
print/scan **[code]**. Its only user is [pGeoTracker](../apps/pGeoTracker.md), whose BFGS optimiser
uses the vector routines, `matrixunit` and `matrixvector` **[code]**. The header `opt.h` comes from
a larger optimisation library (simplex, Nelder–Mead, Gauss integration, Runge–Kutta), most of which
is declared but not in this library **[code]**.

## Interfaces

A library has no MOOS interface **[code]**.

### Subscribes

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
<!-- END GENERATED subscribes -->

### Publishes

<!-- BEGIN GENERATED publishes -->
| Name | Transport | Type / format | Where | Meaning | Notes |
|---|---|---|---|---|---|
<!-- END GENERATED publishes -->

### Configuration

<!-- BEGIN GENERATED config -->
| Parameter | Type | Default | Required | Meaning |
|---|---|---|---|---|
<!-- END GENERATED config -->

### Files, devices and external programs

The print/scan functions write and read the `FILE*` they are given **[code]**. `matrixinverse()`
prints to `stderr` and calls `exit(1)` on a singular matrix **[code]**.

## API

Header `lamss/sonar/track/opt.h`; types `dbl` (`double`) and `status` (`success`, `failure`,
`error`) **[code]**. A matrix of `m` rows and `n` columns is `a[n*i + j]` **[code]**.

| Function | What it does |
|---|---|
| `vectorcopy(n, x, y)`, `vectorfill(n, x, a)` | x = y; x = a for every element **[code]** |
| `vectorvector(n, x, y)` | inner product **[code]** |
| `scalarvector(n, y, k, x)`, `vectoradd(n, x, y, z)`, `vectorsub(n, x, y, z)` | y = kx; x = y + z; x = y − z **[code]** |
| `matrixvector(m, n, y, a, x)` | y = Ax **[code]** |
| `matrixrowvector`, `matrixcolumnvector` | copy row i or column j of A **[code]** |
| `matrixcopy`, `matrixunit(n, a)`, `scalarmatrix`, `matrixadd`, `matrixsub` | copy; identity; kB; B + C; B − C **[code]** |
| `matrixmatrix(m, n, l, a, b, c)` | A = BC, B is m×l, C is l×n **[code]** |
| `matrixtranspose(m, n, a, b)` | A = Bᵀ **[code]** |
| `matrixrowscalar`, `matrixrowexchange`, `matrixrowadd`, `matrixrowaddtwo` | elementary row operations **[code]** |
| `matrixsearchcolumnmaxabs` | row of the largest absolute value in a column range (pivot search) **[code]** |
| `matrixinverse(n, ainv, a, eps)` | Gauss–Jordan inverse with partial pivoting; overwrites `a`; exits on a pivot ≤ `eps` **[code]** |
| `vectorfprint`, `vectorfscan`, `matrixfprint`, `matrixfscan`, `matrixprintformat` | text I/O with a settable `printf` format (default `" %lf "`) **[code]** |
| `matrixallocfscan(fd, m, n, pa)` | `malloc` and read a matrix; defined but not declared in `opt.h` **[code]** |

Used by: [pGeoTracker](../apps/pGeoTracker.md) only **[code]**. pGeoTracker includes its own
copy of `opt.h` (with `extern "C"` declarations) instead of this one **[code]**.

## Known issues and open questions

- `matrixinverse()` ends the whole process with `exit(1)` on a singular matrix. A status return
  would let a caller recover. Nothing in lamss3 calls it at present.
- `opt.h` declares about 25 functions (`nullspace`, `svd`, `LPsimplex`, `QuasiNewtonMethod`,
  `GaussIntegral*`, `RungeKutta`, …) that this library does not define; calling one fails at link
  time. Should the header be cut down to `mat.c`'s functions, and pGeoTracker use it instead of
  its private copy?
- `matrixprintformat()` sets the format used for reading as well as writing.
- The library is built as C17 because C23 no longer accepts K&R definitions **[code]**; converting
  to prototypes would remove that constraint.

## History

- From GobySoft/lamss-shared `src/lib/lib_track` (`mat.c`, `opt.h`); first commit 2011-10-24 (Henrik
  Schmidt, with pGeoTracker); 2 commits before the port (Henrik Schmidt, Toby Schneider), the last
  on 2012-05-21. The old CMakeLists.txt named Don Eickstedt as the author.
- lamss3 port (GobySoft/lamss3#21–#23): clang-format; built as `lamss_sonar_track`, header moved to
  `lamss/sonar/track/opt.h`, compiled as C17; `(void)` casts for unused parameters and `fscanf`
  results kept in unread locals; LGPL-3.0-or-later SPDX headers added.
