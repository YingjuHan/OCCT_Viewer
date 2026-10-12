# N04 — Native VTK Import and Ordinary Viewer

N01–N03 are verified within their documented native support contracts. N04 is
active. This stage covers V01–V05 and B01–B02; Web, Vue, TypeScript and WASM remain
excluded. Use the existing VIEWDRAW plugin. Complete and commit each acceptance
item separately, then explicitly refresh the codebase-memory index.

| Item | Scope | Acceptance gate | Status |
| --- | --- | --- | --- |
| N04.1 | B01 foundation: overall import outcome, backend attempt reports, warnings through composite/series, failed native Viewer/DRAW replacement preservation | VTK ON/OFF native reports, successful fallback without failed-overall message, failed import retains live object/fields and DRAW pixels | verified |
| N04.2 | V01: ordinary native Viewer points, Lines/polyline and general cells | Legacy/XML fixtures; original CellData and nodal mapping; visible ordinary Viewer primitives and pixels; no CutView-only substitute | queued |
| N04.3 | V02: explicit Legacy/XML/composite/series support contract and unsupported-section reports | Format-specific fixtures; no silent loss or field mapping substitution | queued |
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
