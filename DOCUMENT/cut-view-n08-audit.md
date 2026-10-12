# N08 native performance, caches and lifecycle

The native manager reserves source and result envelopes before preparation.
Policies are local to each manager; candidate/time-step/session staging shares
the accounting state with the live manager. Exported geometry retains its lease
until the last external handle is released. Least-recently-used manager results
can be evicted; pinned results remain charged and can cause an explicit refusal.
Budget failures include requested, retained and allowed byte counts.

Default logical policies are 4 GiB for sources and 16 GiB for results. Estimates
include original connectivity, refinement limits, trace support and field
components. Small general sources do not reserve the global refinement maximum
for every cut. Reservation allocation is exception-safe and capped arithmetic
avoids overflow. These are logical reservation limits, not exact limits on
process private bytes, RSS, kernel allocations or GPU memory.

Cancellation tokens and request generations prevent cancelled/superseded
results from publication. Native slicing checks cancellation while visiting
cells. There is no background AIS publication: Viewer changes stay on its
thread; only proven independent original-FE trace work uses OCCT pool threads.

Performance changes include affine native-hex edge sections, BVH corner tests,
compact node incidence, flat primitive weights, bounded source-interior/inverse
cell caches, reuse of single-source traces and matching physical/display fields,
fixed-deformation translation reuse and an On-only plane presentation shortcut.
Warped/degenerate/near-node cells, interfaces and curved sections retain their
general paths. Sphere presentation retains its analytic area handling.

`test/run-cut-benchmark.ps1` creates 1,000,000 native first-order hexahedra and a
nodal linear field. Each process measures 60 crossing-cell, rotating and fixed
deformation plane frames, including original-FE mapping, native presentation
construction, V3d redraw and OpenGL completion. CSV records source, intersection,
slice, trace, FE, primitive and redraw times. Numerical provenance/value checks
and screenshots are outside the measured frame interval; cold setup is reported
separately. The steady-frame acceptance is at least 15 FPS and p95 <=67 ms.

The accepted workload is one On-only plane on this regular native volume mesh.
It does not establish the same bound for arbitrary high-order/general VTK cells,
isosurfaces, multiple-cut OR or all three retained portions. Release optimizes
this repository; the OCCT SDK remains Debug. The recorded renderer is Intel
Iris Xe, OpenGL 4.6 (32.0.101.7088).

The benchmark now computes the scalar range during setup. Earlier solid-red
images were rejected as contour evidence; corrected captures show actual field
gradients. Early failed timings and the post-build failed rotation run remain
in the build directories. The final measured runs use normal application
priority, with background C++ language-service indexing temporarily paused and
then restored. Build, CTest and codebase-memory indexing were stopped during
measurement; no frames were discarded and the 67 ms gate was unchanged.

| Run | Crossing FPS / p95 ms | Rotating FPS / p95 ms | Deformed FPS / p95 ms |
| --- | --- | --- | --- |
| 1 | 56.54 / 39.25 | 25.51 / 43.46 | 26.14 / 54.09 |
| 2 | 54.65 / 38.60 | 25.33 / 45.44 | 27.24 / 47.64 |
| 3 | 56.16 / 39.24 | 25.64 / 44.33 | 27.33 / 45.77 |

Logs: `build/mingw-release/cut-n08-million-isolated1/2/3.log`.
CSV, hardware strings and actual field screenshots are in
`build/mingw-release/test/native/draw-benchmark-100-isolated1/2/3/`.
The failed post-build run is `cut-n08-million-final1.log`; the interrupted
unisolated repeat sequence is `cut-n08-million-closure1/2/3.log` (run 3 failed).
The earlier corrected-color sequence also passed all three runs. Background
load sensitivity is retained as a practical limit, not described as a guarantee
under arbitrary machine load.

Sixty actual native display open/edit/close cycles on a 10x10x10 native hex mesh,
300 frames, measured process private bytes after ten warm-up cycles from
86,134,784 to a peak 90,755,072: growth 4.41 MiB, below the
32 MiB gate. See `cut-n08-lifecycle-acceptance.log`. The graphics driver/window
remain alive across cycles, so the measurement includes repeated AIS disposal
and GPU-context use rather than restarting the process to hide retained memory.

The cache panel covers seven controls, with 39 tools controls at the N08
baseline. Tests exercise limits, eviction, external leases, candidate policy
isolation, 200 edits, cancellation, close and interpolation hash consistency.
The original `cut_view_cache.tcl` geometry regression remains separate from
`cut_view_cache_panel.tcl`.

Drag-baseline validation also shares the live ledger and copies its geometry and
deformation options. Over-budget step staging preserves the drag, definitions,
step and generation. Automatic-cut preparation failure removes only the newly
added cut, preserving the old definitions without a second budgeted JSON validator.

## Final rebuilt validation

| Configuration | Result | Elapsed | Log |
| --- | --- | --- | --- |
| VTK ON | 71/71, each three consecutive runs | 1421.50 s | `build/mingw/cut-n05-n09-final-gate.log` |
| VTK OFF | 30/30, each three consecutive runs | 129.16 s | `build/mingw-no-vtk/cut-n05-n09-final-save-gate.log` |
| headless | 17/17, each three consecutive runs | 91.21 s | `build/mingw-import-headless/cut-n08-n09-budget-gate.log` |
| Release affected | 6/6, each three consecutive runs | 92.66 s | `build/mingw-release/cut-n05-n09-final-gate.log` |

The full suites above use binaries rebuilt after the shared-budget definition-validator,
per-step physical-validator and save-preview fixes. Two later Viewer-only fixes
share drag-baseline accounting/options and directly remove an automatic cut after
failed preparation. Those changes and their refusal/preservation regressions were
rebuilt and verified separately on the final source:

| Configuration | Result | Elapsed | Log |
| --- | --- | --- | --- |
| VTK ON affected | 11/11, each three consecutive runs | 48.58 s | `build/mingw/cut-n08-n09-rollback-final.log` |
| VTK OFF affected | 6/6, each three consecutive runs | 7.00 s | `build/mingw-no-vtk/cut-n08-n09-rollback-final.log` |
| Release Viewer affected | 4/4, each three consecutive runs | 6.80 s | `build/mingw-release/cut-n08-n09-rollback-final.log` |

The first numeric-edit fixture
was rejected as evidence because it reused an existing reservation and did not
exercise budget refusal; its corrected source-epoch fixture passes. Initial failed
and interrupted logs remain separate. These regressions do not replace the N08
million-cell performance evidence or broaden its workload.
