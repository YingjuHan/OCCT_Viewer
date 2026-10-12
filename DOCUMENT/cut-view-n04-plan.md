# N04 — Native VTK Import and Ordinary Viewer

N01–N03 are verified within their documented native support contracts. N04 is
active. This stage covers V01–V05 and B01–B02; Web, Vue, TypeScript and WASM remain
excluded. Use the existing VIEWDRAW plugin. Complete and commit each acceptance
item separately, then explicitly refresh the codebase-memory index.

| Item | Scope | Acceptance gate | Status |
| --- | --- | --- | --- |
| N04.1 | B01 foundation: overall import outcome, backend attempt reports, warnings through composite/series, failed native Viewer/DRAW replacement preservation | VTK ON/OFF native reports, successful fallback without failed-overall message, failed import retains live object/fields and DRAW pixels | verified |
| N04.2 | V01: ordinary native Viewer points, Lines/polyline and general cells | Legacy/XML fixtures; original CellData and nodal mapping; visible ordinary Viewer primitives and pixels; no CutView-only substitute | verified |
| N04.3 | V02: explicit Legacy/XML/composite/series support contract and unsupported-section reports | Format-specific fixtures; no silent loss or field mapping substitution | verified |
| N04.4 | V03: staged composite/time-series import retaining each step's geometry/topology | Moving/remeshed fixtures, time ordering, field/deformation/tuple validation, failed import preserves live model | queued |
| N04.5 | V04: malformed/endian/binary/compressed/appended regression matrix | Truncation, bad types/lengths/connections/offsets and compression blocks; all readers reject safely | queued |
| N04.6 | V05: configurable limits checked before allocation and bounded appended decoding | Arrays/nodes/cells/decompressed-byte limits and measured peak memory | queued |
| N04.7 | B02 and B01 closure: relative reference staging and contextual reports | Nested references, duplicate basenames, missing files, staging failures and ordinary Viewer report delivery | queued |
| N04.8 | Stage closure audit | Full repeated VTK ON/OFF regression, ordinary Viewer images, support boundaries and updated handoff | queued |

N04.1 is pulled forward because current fallback diagnostics obscure the existing
general-cell acceptance. It does not establish general-cell rendering, changing
topology, pre-allocation budgets or recursive reference staging.

N04.1 acceptance: full ON **51/51**, OFF **16/16**, every case three times
(153 / 48 executions, 604.78 / 50.79 seconds). The reduced Viewer-disabled import
test passes three times, and the conditional Viewer target checks were rerun.
Logs are `cut-n04-diagnostics-tests.log` in both build directories and
`build/cut-n04-headless-tests.log`; images are `draw-import/`. See
[the import contract](cut-view-import.md) and handoff section 24 for details.

Preserve the N03 independent numerical/physical evaluations, original FE/source
provenance, actual retained-region OR, curved cap ownership and transparency
regressions. Single-model publication after reader success is only the first
transaction boundary; series still need immutable per-step staging in N04.4.

N04.2 starting points are `MeshGeometryFactory::Create()` in
`source/ViewCaeGeom/MeshGeometry.cpp`, the ordinary `InteractiveCaeMesh` owner
and `MeshPresentationScalar`'s part list. The latter currently lists surface/volume
triangles and edges, nodes and 0D elements; inspect the primitive/BVH/property
path before adding dedicated 1D/general-cell output. Verify original element
identity and FE weights as well as visible pixels. Graph paths have partial C++
coverage; the factory and part list were checked directly against source.

N04.2 closure: ordinary scalar presentations now contain `MeshPartGeneralCells`,
with original element IDs and FE vertex weights, point/line/surface/volume arrays,
visibility and field invalidation. See [the Viewer contract](cut-view-viewer.md).
Full ON **51/51** (188.80 seconds) and OFF **16/16** (12.84 seconds) pass once;
focused import/ordinary Viewer ON **3/3** and OFF **2/2** each pass three times
(10.15 / 0.65 seconds). Logs are `cut-n04-viewer-full.log` and
`cut-n04-viewer-focused.log` in the build directories. Pure-point default colors
were corrected after the first full ON gate; that failed log is preserved.
Reviewed `ordinary-*` images include pure points, Legacy/VTU/VTP polylines,
mixed-dimensional CellData, bowed quadratic nodal fields and a general voxel.
N04.3 format/unsupported-section closure is next; N04 remains active.

N04.3 closure: explicit Legacy attributes/types, metadata/LUT warnings, XML section
validation and restricted native fallback pass full ON **52/52** (190.58 seconds)
and OFF **17/17** (18.12 seconds). Focused ON **5/5**, OFF **4/4**, each three
times (5.32 / 0.53 seconds), pass on the final source. Logs: `cut-n04-format-full.log`
and `cut-n04-format-focused.log`. The first ON failure exposed XML 2.3 polyhedron
face encoding; its repaired native path passes the complete concrete-cell suite.
See [format boundaries](cut-view-import.md). N04.4 per-step staging is next.
