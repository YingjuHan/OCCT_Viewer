# N04 Native Import Closure Audit

N04.1–N04.8 are verified at the native import/ordinary Viewer support contract.
The stage is closed after the final repeated native/DRAW gates. The scope is
native C++, ordinary Viewer and the existing VIEWDRAW plugin.
Web/Vue/TypeScript/WASM remain excluded.

## Accepted Scope and Evidence

| Requirement | Accepted behavior | Regression |
| --- | --- | --- |
| V01 | Ordinary Viewer points, polyline/line primitives and general surface/volume boundaries; original FE/CellData mapping | `vtk_import`, `draw_vtk_import_visual`, 48-cell native/DRAW matrix |
| V02 | Explicit Legacy/XML attributes, roles, numeric types and unsupported sections; validated polyhedron encodings | `vtk_format`, `vtk_safety`, `vtk_cell_cut` |
| V03 | Independent complete geometry/topology/fields per step; strict time/schema/tuple validation; initial display retains matching fields | `vtk_series`, `vtk_reference`, ordinary Viewer series image and failed later-stage replacement |
| V04 | Big/little endian, UInt32/64 headers, ten numeric types, inline/base64/raw appended and multiblock zlib; malformed data rejects before SDK import | `vtk_safety`, `vtk_reader`, import/DRAW preservation |
| V05 | Configurable budgets before decoded/converted/merged storage; bounded current-array appended decoding | `vtk_limits`, kernel peak-working-set gate |
| B01 | Overall outcome, rejected top-level backend attempts, contextual child errors/warnings, original/staged sources, failed publication preserves live data | `vtk_import`, `vtk_reference`, native Viewer and zero-change DRAW images |
| B02 | Collision-free local reference staging, rewritten relative paths, nested VTM/series, cycle/depth/file/byte guards and safe failed-attempt cleanup | `vtk_reference`, `draw_vtk_reference`, `draw_vtk_reference_visual` |

Final VTK ON **58/58**, OFF **22/22**, every case three times (174 / 66 executions; 547.47 / 41.50 seconds). DRAW/Viewer-disabled import/reference tests **2/2** each pass three times (0.52 seconds).

Logs: `cut-n04-closure-tests.log` in both build directories and
`build/cut-n04-headless-closure-tests.log`. The complete console logs were
checked for exactly three passes per case, not only the final CTest summary.
Repair gate: 4/4 each three times in `cut-n04-closure-repair-focused.log`.
N04.7 implementation: `dbfe66b`; static binding repair: `76d845b`. Every
acceptance commit is followed by an explicit codebase-memory refresh.

The first ON full run rejected ten older DRAW cases: native static fields had been
assigned step 0, while omitted-step bindings select -1. The reader now preserves
the portable/static sentinel unless TimeValue exists; series steps remain explicit.
Native import and DRAW tests cover static default and timed explicit bindings.
The failed run is retained as `cut-n04-closure-initial-tests.log` and does not count
as acceptance. Final repeated gates run after this repair.

Final resource rerun: all six 8/32/64 MiB payload processes remain below 256 MiB;
largest kernel peaks are 154,669,056 bytes ON and 150,351,872 bytes OFF. The metric
covers decoder temporary storage with discarded outputs, as defined in the
resource contract. It does not establish a full Viewer working-set/frame budget.

## Viewer and Earlier-Stage Preservation

Reviewed ordinary Viewer images include Legacy/VTU/VTP polylines, points,
mixed-dimensional nodal/CellData contours, a bowed quadratic edge and general
volume boundaries in `build/mingw/test/native/draw-import/ordinary-*`.
Nested composites, generic CellData and remeshed-series initial display are in
`build/mingw/test/native/draw-vtk-references/`. Failure images must match the live
reference image exactly. Native tests also compare preserved AIS handles/mesh data.

The full gate retains N02 independent polynomial/rational geometry and original-FE
references, numerical convergence and precision fallback; N03 independent
physical/display fields and two-sided traces; actual retained-region OR and curved
cap ownership; transparency; 47/47 Tk controls and real held-pointer updates.
Passing import tests alone does not replace these regressions.

## Support Boundaries

- General concrete VTK cells require the native SDK; OFF keeps portable supported
  topology and explicitly rejects unsupported general-cell leaves. Mappings never
  fall back to a backend that discards them.
- The 48-cell native matrix establishes tested VTU/concrete-cell scope. XML 2.3
  polyhedra use validated native ON import. Legacy polyhedron 42 lacks face
  normalization and requires VTU. No all-cell/all-format claim is made.
- Supported local formats are Legacy UnstructuredGrid/PolyData, XML VTU/VTP,
  nested VTM and `.vtm.series`. Structured/parallel XML, PVD/PVTU/PVTP, VTKHDF,
  ODB, remote/archive reference staging and composite FieldData are outside scope.
- Series snapshots keep later geometry and fields together. Coordinated switching,
  deformation animation and playback are N06; the primary Viewer displays the
  initial geometry and only its fields when topology changes.
- Logical import budgets include decoded/conversion/merge storage; bounded file
  buffering still precedes XML parsing. DOM/SDK overhead and snapshot copies are
  additional memory. Successful staging directories are caller-owned. Full-frame
  million-cell performance, cache/eviction and long-session budgets remain N08.
- Datum frames, per-cut styles/legends, result picking/probes, vectors and CAD
  synchronization remain N05; shell/beam physical mappings remain N07; full native
  sessions remain N09. Earlier numerical/physical support limitations still apply.

See [the import contract](cut-view-import.md), [ordinary Viewer](cut-view-viewer.md),
[resource limits](cut-view-resource-limits.md) and [cell matrix](vtk-cell-support.md).

## Reproduction

Use the MinGW SDK paths/configuration in [the handoff](cut-view-native-handoff.md).
Finish each configuration's build before running its tests. ON/OFF configurations
can run independently; do not run two CTest processes in the same build directory.

```powershell
cmake --build build/mingw --parallel 6
ctest --test-dir build/mingw --output-on-failure --repeat until-fail:3 -j 4
cmake --build build/mingw-no-vtk --parallel 6
ctest --test-dir build/mingw-no-vtk --output-on-failure --repeat until-fail:3 -j 4
./test/native/MeasureVtkMemory.ps1
```

Logs: `cut-n04-closure-tests.log` in the two build directories; resource measurements
in `cut-n04-memory-*.log` and `build/cut-n04-memory.json`. Headless importer checks
use `build/mingw-import-headless` and `build/cut-n04-headless-closure-*.log`.
