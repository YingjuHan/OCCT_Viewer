# N04 — Native VTK Import and Ordinary Viewer

N01–N04 are verified within their documented native support contracts.
N04 is closed; see [the final audit](cut-view-n04-audit.md).
This stage covers V01–V05 and B01–B02; Web, Vue, TypeScript and WASM remain
excluded. Use the existing VIEWDRAW plugin. Complete and commit each acceptance
item separately, then explicitly refresh the codebase-memory index.

| Item | Scope | Acceptance gate | Status |
| --- | --- | --- | --- |
| N04.1 | B01 foundation: overall import outcome, backend attempt reports, warnings through composite/series, failed native Viewer/DRAW replacement preservation | VTK ON/OFF native reports, successful fallback without failed-overall message, failed import retains live object/fields and DRAW pixels | verified |
| N04.2 | V01: ordinary native Viewer points, Lines/polyline and general cells | Legacy/XML fixtures; original CellData and nodal mapping; visible ordinary Viewer primitives and pixels; no CutView-only substitute | verified |
| N04.3 | V02: explicit Legacy/XML/composite/series support contract and unsupported-section reports | Format-specific fixtures; no silent loss or field mapping substitution | verified |
| N04.4 | V03: staged composite/time-series import retaining each step's geometry/topology | Moving/remeshed fixtures, time ordering, field/deformation/tuple validation, failed import preserves live model | verified |
| N04.5 | V04: malformed/endian/binary/compressed/appended regression matrix | Truncation, bad types/lengths/connections/offsets and compression blocks; all readers reject safely | verified |
| N04.6 | V05: configurable limits checked before allocation and bounded appended decoding | Arrays/nodes/cells/decompressed-byte limits and measured peak memory | verified |
| N04.7 | B02 and B01 closure: relative reference staging and contextual reports | Nested references, duplicate basenames, missing files, staging failures and ordinary Viewer report delivery | verified |
| N04.8 | Stage closure audit | Full repeated VTK ON/OFF regression, ordinary Viewer images, support boundaries and updated handoff | verified |

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
transaction boundary; series received complete independent per-step staging in N04.4.

The milestone paragraphs below preserve each item's original test counts and
next-step notes. Current acceptance is the N04.8 closure at the end.

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

N04.4 closure: Each step now owns independent geometry/topology and matching fields in MeshData::SeriesSteps(). Finite strictly increasing manifest times override valid dataset TimeValue; role/component/name/tuple mismatches reject before publication. Stable geometry retains the existing multi-step field API; changing geometry exposes only initial fields on the initial Viewer mesh, with an explicit warning. Generic/empty cells survive merges. Focused ON 5/5 and OFF 4/4 each pass three times (5.96 / 0.84 seconds), including failed later-stage Viewer replacement and zero changed DRAW pixels. Logs: cut-n04-series-focused.log. Coordinated animation remains N06.

N04.5 closure: Strict typed ASCII values and base64 padding, inline trailing-byte rejection, original XML topology/field preflight before the VTK SDK, modern polyhedron face membership and duplicate/unknown topology rejection are verified. The matrix covers both byte orders, UInt32/UInt64 headers, inline/base64/raw appended storage, multiple zlib blocks, truncation, invalid types/offsets/tuples and reader reset. Focused ON 8/8, OFF 6/6 each pass three times (45.36 / 1.20 seconds), including the 48-cell native matrix and ordinary Viewer images. Logs: cut-n04-safety-focused.log. Pre-allocation budgets remain N04.6.

N04.6 closure: Configurable stream/array/component/node/cell/decoded budgets propagate across composite/series children and native preflight. Legacy normalization, dense merged fields and PolyData expansion/conversion are bounded before allocation. Appended decoding now reads only the requested header/payload. Final focused ON 9/9, OFF 7/7 each pass three times (52.19 / 1.87 seconds). Six kernel peak-memory measurements for 8/32/64 MiB payloads stay below 256 MiB; largest ON/OFF peaks are 154632192 / 150278144 bytes. Evidence and API limits: cut-view-resource-limits.md; logs cut-n04-limits-focused.log and cut-n04-memory-*.log. Recursive reference depth/cycle/staging closure is N04.7.

N04.7 closure: Nested VTM and Legacy/XML leaves now share restricted native fallback; explicit local staging rewrites relative references into collision-free attempt directories. Tests move the original source tree away, preserve Float64 values across Float32 blocks and NaN field gaps, reject cycles/depth/file limits and incompatible schemas, retain generic/empty cell IDs and complete 4-node/1-cell to 5-node/2-cell series snapshots. Ordinary Viewer receives original/staged sources; staging failures preserve old AIS objects and zero changed DRAW pixels. Focused ON 12/12 and OFF 9/9 each pass three times (45.93 / 1.47 seconds). Logs: cut-n04-reference-focused.log. Reviewed nested/general-cell/series images: draw-vtk-references. N04.8 full closure audit remains.


N04.8 closure: Final VTK ON **58/58**, OFF **22/22**, every case three times (174 / 66 executions; 547.47 / 41.50 seconds). DRAW/Viewer-disabled import/reference tests **2/2** each pass three times (0.52 seconds). The final gate preserves 47/47 Tk controls, the 48-cell matrix, independent N02 geometry/FE references, N03 physical/display fields, actual OR, curved caps and transparency. Ordinary Viewer images and all six kernel peak-memory gates were reviewed. Static field-step repair: 76d845b; first failed ON log retained. All current documents reflect the native support contract; N05-N09 remain queued. See cut-view-n04-audit.md.
