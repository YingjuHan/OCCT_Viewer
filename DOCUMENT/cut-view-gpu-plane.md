# Resident single-plane display

The native viewer's `Auto` backend uses a resident source surface and a mutable
CPU section for one enabled plane on a volume mesh. It does not need an external
VTK SDK. Numerical cuts, physical fields, integration, export, and multiple-cut
OR evaluation retain their existing contracts.

## Display combinations

Bits are Negative = 1, On = 2, Positive = 4.

| Mask | Display |
| --- | --- |
| 0 | Hidden body and section |
| 1 | GPU-clipped negative body, section in Negative style |
| 2 | Section only; resident body disconnected from rendering |
| 3 | GPU-clipped negative body, section in On style |
| 4 | GPU-clipped positive body, section in Positive style |
| 5 | Whole source once; no clipping or display-section evaluation |
| 6 | GPU-clipped positive body, section in On style |
| 7 | Whole source once, independent section, ordinary depth occlusion |

The independent On display for a single plane also applies to the CPU backend.
It is a display layer, so the numerical OR boundary still excludes interior caps.

`scvcutview model renderbackend auto|cpu` selects the runtime backend.
`scvcutview model renderinfo` reports the active path and fallback reason.
Backend selection is not persisted as a new session version.

Auto eligibility is one enabled plane, volume elements only, opaque FlatShaded
styles, and no vector plot. Other cases use the existing CPU path with reasons
`multiple_cuts`, `non_plane_cut`, `non_volume_mesh`, `display_mode`, `vectors`, or
`transparent_style`. Transparent styles retain CPU surface ordering: the current
OCCT connected-instance GPU path did not match the existing union opacity images.
Edge and shrink modes likewise keep their existing behavior on CPU.

## Lifetime and uploads

OCCT's instanced child rendering calls `renderGeometry`, bypassing the child's
clip and visibility state. It also supports only one instanced child. The body
shader therefore clips fragments explicitly. The section is a group on the AIS
parent; the body is the single persistent child. Hidden-body masks disconnect
that child without deleting its structures or VBOs.

Warm plane edits update plane uniforms and the section's active attribute range.
Identity indices are initialized once per capacity; stable counts upload no
indices. Count changes invalidate one reserved index so OCCT updates its draw
count, including empty sections. The source body arrays do not change. Section
buffers reserve capacity and grow when needed. Geometry/deformation changes
rebuild the source once.
Field changes preserve source positions/normals/indices and invalidate only the
contiguous UV attribute range in the deinterleaved body buffer.

`submitted_*_bytes` are CPU submission/invalidation counters, not GPU-driver
traffic measurements. The separate GDB capture checks OCCT VBO initialization,
the actual `glBufferSubData` call sites, and primitive rendering. It is not used
for timing. It requires the configured Debug OCCT SDK and its matching source.

```powershell
./test/run-cut-upload-capture.ps1 -BuildDirectory build/mingw
./test/run-cut-upload-capture.ps1 -BuildDirectory build/mingw -Grid 110
```

The capture writes `cut-upload-capture.json` and `.log` under the build directory
(with a grid suffix for non-default fixtures). The 6^3 and 110^3 fixtures cover
16-bit and 32-bit section indices respectively.
Its stable-capacity fixture requires zero warm body uploads, zero warm section
VBO initializations, zero stable-count section index uploads, one-index updates
on count changes/empty frames, only UV body uploads on a field edit, and zero
body render calls while displaying the section alone. Native refresh and plane mouse edits
use `AIS_InteractiveContext::Update` for the GPU path; `Redisplay` always marks
the parent dirty and would recreate the section primitive despite mutable arrays.
Group, structure, and layer bounds are refreshed when section positions change,
so `FitAll` and frustum culling use the current section.

## Cap/body precision investigation

The strengthened fixture reproduced 20 internal white pixels for a unit-size
cube near coordinate 1000000 before the correction. Two causes were identified:
child clip state did not execute in instanced rendering, and independently
quantized world-space section positions could diverge from the body boundary.

Body and cap now share a source-bounds origin. The body positions and plane
equation are expressed in that local frame. Cap positions are reconstructed from
original FE weights and the source's deformed nodes in double precision before
conversion to local floats. The camera transform applies the origin and local
delta separately. Physical mesh data, interpolation values, and integrals are
unchanged; no arbitrary plane offset is added.

`cut_view_gpu_plane.tcl` checks opaque convex silhouettes at offsets 0, 100000,
and 1000000, oblique normals, both retained sides, zoom, affine deformation, exact
source-face coincidence, all eight masks, and CPU/GPU mask silhouettes. This is
bounded raster evidence. Source MeshData coordinates remain Float32; information
lost before import cannot be restored. Arbitrary curved geometry or world-space
global clipping at extreme coordinate scales is not guaranteed gap-free by these
fixtures.

## Generated large files

`test/generate-large-vtk.py` streams one grid layer at a time using the Python
standard library. It writes big-endian legacy binary VTK and little-endian raw
appended VTU with UInt64 length headers and explicit offsets. These follow the
official [legacy format](https://docs.vtk.org/en/latest/vtk_file_formats/vtk_legacy_file_format.html)
and [XML format](https://docs.vtk.org/en/latest/vtk_file_formats/vtkxml_file_format.html).

```powershell
python test/generate-large-vtk.py --grids 128 150 --deformation --output build/large-vtk
```

| Grid | Cells | Points | VTK bytes | VTU bytes |
| --- | ---: | ---: | ---: | ---: |
| 128^3 | 2,097,152 | 2,146,689 | 195,117,927 | 188,827,344 |
| 150^3 | 3,375,000 | 3,442,951 | 313,490,159 | 303,366,033 |

Fields are nodal `temperature = x + 2y + 3z`, elemental `pressure = 7 + cellId/4`,
and optional affine nodal `displacement`. Use `--grids 160` for 4,096,000 cells.
The files and manifest stay in ignored build directories and are not committed.

```powershell
./test/run-cut-benchmark.ps1 -InputFile build/large-vtk/hex_128.vtu -Mask 3 -Frames 60 -RunLabel large-vtu
./test/run-cut-benchmark.ps1 -InputFile build/large-vtk/hex_150.vtk -Mask 2 -Frames 60 -Scenarios rotating -RunLabel large-vtk
```

`scvcutfilebenchmark` uses the ordinary self-contained MeshLoader, checks source
counts and original FE values, and times edit/refresh/redraw plus `glFinish`.
Import and cold setup are reported separately. CSV rows record source-body builds
and submitted bytes. File-based large runs report the 15 FPS / 67 ms target as
data; they do not silently turn correctness success into a performance claim.
The optional Python-enabled CTest smoke checks both generated binary formats,
counts, fields, masks 2/3/5/7, rotating sections, and fixed deformation.

## Verification results

The separate upload capture passes on the rebuilt Debug native viewer:
three warm plane edits upload section ranges only (6912 attribute bytes and 432
index bytes per edit in the 6^3 fixture). No source body VBO is initialized or
uploaded in those frames. The field edit uploads the body's UV range only:
offset 31104, length 10368, for 1296 vertices. Section-only frames do not render
the body and do not recreate section VBOs. Evidence is in
`build/mingw/cut-upload-capture.json` and `.log`.

Earlier N08 measurements are historical baseline evidence and should not be used
as measurements of this new backend. The current Release measurements include
OpenGL completion and are recorded separately from builds, tests, GDB and indexing.

The rebuilt native regression passed 75/75 tests in 243.31 seconds
(`build/mingw/hybrid-final-75.log`). After the benchmark's both-only validation
was moved outside all timed frames, the generator/file/benchmark smoke passed
3/3 (`build/mingw/large-fixture-final.log`), and the Release mask-5 smoke passed.
The numerical validation still runs for mask 5; its CSV display-section counts
are zero, and it does not introduce section cleanup into the next timed edit.

## Current native measurements

Measured on Intel Iris Xe, OpenGL 4.6.0, driver 32.0.101.7088. This repository
uses Release, while the OCCT SDK uses Debug. Builds, CTest, GDB and repository
index refreshes completed before timing. Ordinary desktop applications remained
running; this is a workstation measurement, not an exclusive-machine benchmark.
The view is 900 x 650, with one scalar field. Each frame includes plane edit,
refresh, redraw and `glFinish`. Import, cold setup, validation and screenshots
are outside the frame interval. Deformed runs translate a plane through a fixed
deformation; they do not animate or rebuild deformation every frame.

One million native first-order hexahedra, 60 frames per scenario in each of
three processes per mask:

| Mask | Scenario | FPS range | p95 range (ms) | Target passes / runs |
| --- | --- | ---: | ---: | ---: |
| 2 | Crossing | 57.96–66.89 | 35.01–39.08 | 3/3 |
| 2 | Rotating | 20.47–22.57 | 60.11–64.43 | 3/3 |
| 2 | Fixed deformation | 19.61–21.69 | 70.60–78.09 | 0/3 |
| 3 | Crossing | 55.46–61.61 | 36.50–40.95 | 3/3 |
| 3 | Rotating | 19.84–21.72 | 58.79–63.98 | 3/3 |
| 3 | Fixed deformation | 20.50–21.86 | 65.45–74.75 | 1/3 |
| 5 | Crossing | 398.18–427.69 | 2.50–2.83 | 3/3 |
| 5 | Rotating | 355.98–382.81 | 2.98–3.13 | 3/3 |
| 5 | Fixed deformation | 369.03–400.92 | 2.70–2.83 | 3/3 |
| 7 | Crossing | 56.06–62.32 | 37.45–40.58 | 3/3 |
| 7 | Rotating | 19.45–21.45 | 57.13–66.38 | 3/3 |
| 7 | Fixed deformation | 19.90–21.42 | 70.46–77.14 | 0/3 |

The target requires both >=15 FPS and p95 <=67 ms. Fixed-deformation workloads
do not meet that target consistently. Failed runs are retained, and the native
runner returns failure for them. Correctness and resident-body checks passed:
all warm CSV rows submit zero body bytes and keep the initial body-build count.
On-only starts with no body build; the other three masks build the body once.

For 97,336 cells (46^3), one 60-frame run per mask and scenario:

| Mask | Crossing FPS / p95 ms | Rotating FPS / p95 ms | Fixed deformation FPS / p95 ms |
| --- | ---: | ---: | ---: |
| 2 | 240.30 / 8.16 | 119.93 / 9.61 | 148.88 / 10.61 |
| 3 | 230.90 / 8.26 | 111.64 / 11.11 | 139.35 / 10.73 |
| 5 | 607.11 / 1.86 | 623.12 / 1.72 | 605.04 / 1.79 |
| 7 | 230.39 / 8.32 | 118.14 / 9.58 | 140.06 / 11.05 |

Native evidence is under `build/mingw-release/cut-full-frame-100-*-auto-isolated-r*.log`
and `cut-full-frame-46-*-auto-isolated-r1.log`; corresponding CSVs and first/last
frame PNGs are in `test/native/draw-benchmark-*-auto-isolated-r*` under that build.
The eight failed million-cell run labels are retained in
`resident-performance-failures.json`.

The first million-cell mask-3 run averages approximately 3 ms actual redraw,
versus 33.5 ms intersection/trace preparation and 8.4 ms primitive preparation
for rotation. Fixed deformation averages approximately 21.8 ms intersections,
7.8 ms edit/cache cleanup and 10.7 ms primitive preparation. `slice_ms` and
`trace_ms` partition `intersection_ms`; they must not be added to it again.
Removing body uploads therefore addresses the intended GPU traffic problem,
while CPU section construction, provenance/trace allocation and section primitive
preparation remain performance work for larger or deformed sections.

An optional CPU mask-3 comparison was stopped: 97,336 cells remained in cold
setup after several minutes, and a 1,000-cell run also had expensive full-volume
frames. These interrupted runs are retained as `isolated-cpu-r1` and
`isolated-small-compare` outputs. They are not completed performance samples and
provide no FPS or speedup claim. CPU/GPU silhouette correctness is covered by
the bounded gap/display regression instead.

## Actual multi-million-cell files

One completed 60-frame crossing run per file, mask 3, same hardware and viewport:

| File | Cells | Import (s) | Cold setup including import (s) | FPS | p95 (ms) |
| --- | ---: | ---: | ---: | ---: | ---: |
| `hex_128.vtu` | 2,097,152 | 1.839 | 5.752 | 27.25 | 80.40 |
| `hex_128.vtk` | 2,097,152 | 39.445 | 43.375 | 26.73 | 79.47 |
| `hex_150.vtu` | 3,375,000 | 3.483 | 9.155 | 19.70 | 95.90 |
| `hex_150.vtk` | 3,375,000 | 63.758 | 70.812 | 17.09 | 98.10 |

All four files passed ordinary portable import, source-cell/point counts,
nonempty section checks, original-FE interpolation sampling and real redraw.
The FE assertion checks one provenance-weighted section node per frame; it is
not an exhaustive scan of every imported field value. All warm rows retain
one source-body build and submit zero body bytes. None of these four runs meets
the joint >=15 FPS / p95 <=67 ms target. Import is measured in the current file
cache state, not as a guaranteed cold-disk throughput result. The earlier
concurrent 3-frame VTK import probe is excluded from this performance table.

Legacy binary import is much slower than appended VTU for these fixtures.
Source reading and resident-body setup remain separate from warm interaction.
For the 3,375,000-cell VTU crossing run, averages are approximately 13.1 ms
intersection/trace, 8.0 ms FE mapping, 6.7 ms edit/cache cleanup, 18.7 ms primitive
preparation, and 4.3 ms actual redraw. Maximum submitted section ranges total
4,860,000 bytes per frame; source-body ranges remain zero.

Evidence for these four imports is
`build/mingw-release/cut-full-frame-hex_{128,150}.{vtk,vtu}-3-auto-isolated-large-r1.log`.
Their `crossing-import.txt`, CSVs and first/last PNGs are under the corresponding
`test/native/draw-benchmark-hex_*-3-auto-isolated-large-r1` build directories.
The 128^3 and 150^3 crossing screenshots were inspected alongside the bounded
pixel regression. The larger legacy files are validated for crossing; rotating,
fixed deformation and all four display masks are exercised on the 150^3 VTU.
The binary-format CTest smoke exercises those combinations on both formats.

For 150^3 VTU, one completed 60-frame run per display combination and scenario:

| Mask | Crossing FPS / p95 ms | Rotating FPS / p95 ms | Fixed deformation FPS / p95 ms |
| --- | ---: | ---: | ---: |
| 2 | 22.08 / 89.53 | 10.33 / 129.61 | 9.12 / 174.67 |
| 3 | 19.70 / 95.90 | 9.84 / 138.97 | 8.87 / 181.27 |
| 5 | 276.26 / 4.09 | 243.51 / 6.29 | 281.61 / 3.75 |
| 7 | 18.94 / 93.68 | 10.16 / 133.38 | 9.21 / 156.85 |

Only mask 5 meets the joint target in all three multi-million-cell scenarios.
It displays the whole body without clipping or evaluating a display section;
all its timed rows have zero submitted section bytes and zero display-section
nodes/cells. Mask 7 likewise leaves the body unclipped, but still computes and
renders its independent section. On-only mask 2 builds/renders no source body.
Its similar rotation/deformation cost confirms the remaining CPU section cost.

The mask-3 rotation/deformation logs use `isolated-large-extra-r1` to retain
the separate crossing process. Other 150^3 masks use `isolated-large-r1`.
All 15 file scenarios pass correctness/residency checks. Together with the
48 native scenarios, 3,780 timed CSV rows pass constant body-build counts and
zero warm body submissions. The local aggregate is
`build/mingw-release/resident-performance-summary.json`; it includes unsuccessful
native performance gates and excludes the interrupted CPU samples.

Next performance work should profile section/trace allocation and cleanup,
reduce section primitive preparation, and improve the legacy binary import
path. These are open follow-ups; this audit closes the resident-body behavior,
bounded precision fixtures and large-file functional verification, not a
universal multi-million-cell frame-rate requirement. Browser/WASM remains
outside this native change.

## Plane drag preparation update (2026-10-14)

`de8bc05` batches interleaved section vertices after checking the OCCT buffer
layout, computes nodal/constant-element color coordinates once per entity,
reserves triangle staging capacity, and prepares large-section double positions
and nodal colors with four workers. Source geometry/fields are materialized on
the viewer thread before workers read them. FE position reconstruction is
unchanged. Bounds are computed during staging, avoiding an OCCT vertex readback
scan. Identity indices are initialized once per capacity; stable counts submit
only attributes, and count changes submit one index to refresh OCCT draw counts.
For the 150^3 crossing fixture this reduces a warm submitted section range from
4,860,000 to 4,320,000 bytes. These are invalidation counters, not driver traffic.

The user's `build/mingw/Release` (`-O3 -DNDEBUG`) is the measured build. Same
hardware/viewport/fields as above; one 60-frame baseline process and three
60-frame post-change processes per scenario, On-only. Builds, CTest, GDB capture
and requested index refreshes are outside these timing runs. Other desktop
activity was not suspended. AIS/primitive preparation includes presentation
setup and cannot be interpreted as just GPU buffer copying.

| Scenario | Baseline FPS / p95 ms | Updated FPS range / p95 range ms | Baseline preparation mean / updated range ms |
| --- | ---: | ---: | ---: |
| Crossing | 26.51 / 82.57 | 25.45–33.57 / 77.09–105.66 | 14.11 / 6.08–7.62 |
| Rotating | 11.56 / 109.30 | 12.05–12.45 / 96.23–105.84 | 14.89 / 6.75–7.02 |
| Fixed deformation | 10.23 / 124.75 | 10.25–11.57 / 118.84–144.34 | 20.20 / 8.95–10.77 |

Preparation cost consistently fell, while total frames still vary with CPU
slice/trace/field cost. The first updated crossing run was slower overall despite
lower preparation cost; no universal frame-rate gain or joint target pass is
claimed. All nine updated scenarios pass original-FE sampling and residency.
Logs/CSVs are under `build/mingw/Release`, with labels `drag-before` and
`drag-after-isolated-r{1,2,3}`. The earlier `drag-after` run overlapped a Debug
build and is excluded from these comparisons.

The repeatable `-MouseDrag` benchmark loads the same 3,375,000-cell VTU in the
actual Tk Manager, picks the plane and sends held mouse events through
AIS_ViewController. Each timed move includes `glFinish` and a Manager refresh.
Refresh is tested every frame, more often than normal 150 ms polling; assertions
and screenshots are outside timing. This is event-processing throughput, not a
recording of human input or a native OS message backlog. Release/abort, changed
pixels, one synchronization per move, no poll-triggered synchronization and
zero warm body submissions are required. Original-FE sampling remains in the
separate file benchmark and bounded interaction regression.

| Mask | Frames | FPS | p50 ms | p95 ms | Mean Manager refresh ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| 2 | 60 | 23.38 | 26.02 | 84.01 | 0.94 |
| 3 | 60 | 21.57 | 29.19 | 89.74 | 0.98 |
| 7 | 60 | 21.49 | 31.04 | 101.49 | 0.98 |

Evidence is `cut-full-frame-hex_150.vtu-{2,3,7}-auto-pointer-after-r1.log` and the
corresponding `drag.csv`, first/last PNGs under `test/native/draw-benchmark-*` in
that build. The mask-3 CSV separates section/FE/presentation phases. For example,
a layer-crossing frame spends 66.12 ms in slice plus trace versus 6.14 ms in FE
and 5.80 ms in presentation preparation. First/last mask-3 PNGs were inspected.
Manager polling is not the remaining primary bottleneck.

Debug affected regression: 7/7 (28.89 s), covering native/display, real pointer
commit/abort, Tk controls, cap/body offset/deformation/mask raster fixtures and
large binary-format smoke. Evidence: `build/mingw/drag-regression.log`.
The GDB capture passes on both 6^3 and 110^3 fixtures (1,331,000 source cells
in the latter). Warm stable-count frames upload no section indices. Changed
counts, the empty frame and restoration upload exactly two bytes per update
for 16-bit indices and four bytes for 32-bit indices, without section VBO
reinitialization. Evidence: `build/mingw/cut-upload-capture{,-110}.json` and `.log`;
these captures are kept separate from frame timing.

Remaining work is CPU native slice construction when crossing source vertices
or changing the normal, compact trace/provenance staging and FE mapping. Any
further optimization must retain interface classification, independent physical
trace policies and paired body/cap publication. The multi-million-cell joint
>=15 FPS / p95 <=67 ms gate remains open.

## 2026-10-14 large-data CPU pipeline verification

Only the existing 150^3 grids were used for the following new measurements:
3,375,000 hexahedra and 3,442,951 points. Release build `build/mingw-release`,
60 frames per case, actual OpenGL completion, sequential runs without compilation
or graph indexing during timing. Import and cold setup are outside frame timing.

| Actual Manager mouse drag | FPS | p50 ms | p95 ms | Prior p95 ms |
| --- | ---: | ---: | ---: | ---: |
| VTU mask 2, section only | 31.35 | 22.37 | 57.86 | 84.01 |
| VTU mask 3, one side plus section | 29.62 | 24.36 | 65.29 | 89.74 |
| VTU mask 7, both sides plus section | 29.65 | 24.19 | 64.83 | 101.49 |
| Legacy VTK mask 2 | 31.93 | 22.66 | 58.32 | — |

All four runs pass one synchronization per move, paired frames, zero warm body
uploads and mouse release/abort. Mean Manager refresh is 0.99–1.04 ms. Mask 3's
final framebuffer was inspected. Evidence is the `cpu-large-pointer-r6` logs and
`drag.csv`, `drag-first.png`, `drag-last.png` under this build's benchmark folders.
The live legacy VTK process was checked: ViewDraw and ViewCaeGeom were loaded
from `build/mingw-release/bin`.

| Full-frame VTU mask 2 | FPS | p50 ms | p95 ms | P0 serial p95 ms |
| --- | ---: | ---: | ---: | ---: |
| Translation crossing layers | 45.25 | 18.81 | 52.32 | 75.67 |
| Rotation | 18.20 | 55.16 | 60.22 | 98.39 |
| Fixed-deformation translation | 17.76 | 60.23 | 73.86 | 122.48 |
| Fixed-deformation repeat | 18.20 | 59.32 | 71.66 | 122.48 |

Evidence: labels `cpu-large-r6` and `cpu-large-r7`. Each run passes original-FE
sampling and resident-body checks. The `PASS` marker verifies correctness;
the result's `target` field separately records the 15 FPS / 67 ms gate.
Deformation has `target 0`; no complete performance closure is claimed.
These full-frame CSVs contain detailed trace phases but lack the newly added
cumulative cache columns; pointer CSVs include those columns. Their phase data
must not be presented as cache-counter evidence.

After rebuilding the benchmark command object explicitly, `cpu-large-r8`
contains the cumulative cache columns. Its crossing/rotation/deformation results
are 43.60/17.73/17.68 FPS and 51.49/63.85/84.84 ms p95. This repeat confirms
rotation/crossing improvement and exposes deformation variability; retain all
runs rather than selecting the best one. No geometry algorithm changed between
these runs. The usual `build/mingw/Release` runtime was also rebuilt and verified
with the same 3,375,000-cell VTU: actual mouse drag mask 2, 30.99 FPS, 23.47 ms
p50, 65.23 ms p95, paired frames, zero body uploads and release/abort passed.
Evidence label: `cpu-large-runtime-r8` in that build.

In the r6 deformed long frames, coordinate snapshot preparation costs roughly
14–19 ms, deterministic merge 10–16 ms, pure intersection around 2 ms, and trace
work 14–21 ms. FE interpolation is approximately 3–4 ms. A 192 ms outlier is
retained in the CSV. BVH candidate culling is already in use; increasing the
intersection worker count alone will not remove these remaining costs.

Implemented P0–P3 follow-up: direct original-root provenance, undeformed worker
preparation, touched-node deformation cache, sparse fallback sharing, generation
map pools, persistent origin scratch, native interior preparation, bounded partial
cache eviction and exact parallel projection sorting/union. Source IDs and FE
weights retain deterministic publication; near-node/general cells retain fallback.
P4 is still pending: shared mutable Manager caches cannot safely be moved onto
a worker without snapshot isolation and Viewer-thread paired publication.

Earlier-started correctness regression completed 69/69 in 275.15 s before the
large-data-only testing instruction. No new small-data tests were started after it.
