# Native VTK Import Outcomes

N04 covers import diagnostics, [ordinary Viewer display](cut-view-viewer.md),
format validation, independent series snapshots, [resource limits](cut-view-resource-limits.md)
and local reference staging. Acceptance evidence is tracked in [the N04 queue](cut-view-n04-plan.md).

`RWMeshConv::MeshLoader::VtkReport()` describes the most recent VTK import:

- `succeeded` identifies the overall outcome; `source` and `format` identify the input.
- `stagedSource` identifies the rewritten local entry point when explicit reference staging is enabled.
- `backend` identifies the last attempted backend (`portable` or `native-vtk`).
- `points`, `cells`, `fields` count the published mesh; failed imports publish zero counts.
- `errors` and `warnings` describe the final outcome. Successful native fallback has
  no overall errors; rejected portable errors remain in `attempts` for diagnosis.
- Each attempt records its backend, success, errors and warnings. `IsComplete()`
  means no errors **and no warnings**; use `succeeded` to distinguish success from failure.

Metadata and attempt fields are populated by MeshLoader. The standalone RWVtk
readers continue exposing their own count/error/warning reports.

VTK legacy, VTU and VTP first use the portable reader. With `OCCTV_USE_VTK=ON`,
an unsuccessful unmapped import can try the native concrete-cell backend. Mapping
requests never fall back to an implementation that would discard the mapping.
VTM and `.vtm.series` use their composite/series readers. Each VTU/VTP/Legacy leaf
uses the same restricted portable/native topology policy, including the mapping
restriction. The manifest itself never falls back to a single-dataset SDK reader.
Uppercase `.VTK` filenames use the legacy native backend correctly.

Portable reader attempts collect diagnostics before MeshLoader prints the final
outcome. Legacy's normalized XML reader and composite/series children inherit the
message policy. Mapping warnings survive successful and unsuccessful child reads.
`SetToReportMessages(false)` controls this OCCT report emission; external mapping
reader informational messages and VTK SDK parser logging remain separate.
Native VTK pipeline failure and outputs with neither points nor cells are rejected
instead of publishing a failed pipeline's empty output. A valid `vtkEmptyCell`
remains supported even though its connectivity has no nodes.

`Viewer::InteractiveCaeMeshManager::OpenMesh(..., options, &report)` optionally
returns the same VTK report. A reader failure leaves the old AIS object, mesh and
fields intact. A successful staged read constructs the replacement before removing
the old object. `scvreadmesh` also waits for a successful read before removing the
old named object. Existing callers may omit the optional report argument; native
clients must rebuild against the changed exported C++ signature.

The CutView DRAW adapter uses the same loader. Query a successful model's report:

```tcl
scvcutview model importreport
```

This returns a Tcl dictionary, including nested `attempts`, `errors` and `warnings`
lists. Failed `replace` keeps the previous generation and its report. Paths and
messages are Tcl list elements, including paths containing spaces.

Acceptance is in `vtk_import`, `draw_vtk_import` and `draw_vtk_import_visual`.
The native checks cover portable success, VTU/VTP/legacy fallback, original
Polyline connectivity/CellData, uppercase suffixes, rejected mappings, malformed
XML, missing composite children, warning propagation, report reset and native
Viewer replacement. The visual case displays an ordinary Viewer tetrahedron and
requires **zero changed pixels** after missing-file, malformed-file and missing-child
imports. It also verifies successful replacement. Images are in
`build/mingw/test/native/draw-import/`.

The native executable initializes TKDraw's linked Tcl runtime and flushes an
explicit result marker; CTest requires `VTK_IMPORT_PASS` and rejects
`VTK_IMPORT_FAIL`, independent of TKDraw's process finalization. On Linux without
an X display, it explicitly reports the Viewer lifecycle check as skipped while
running reader/report checks. The current Windows acceptance runs that lifecycle
check. When the Viewer target is disabled, the same executable runs the reader
checks and explicitly reports the lifecycle check as skipped. The N04.1 test is
complemented by the later format, series, safety, limits and reference gates.

## N04.3 Format Contract

| Format | Supported data | Boundary |
| --- | --- | --- |
| Legacy `.vtk` | UNSTRUCTURED_GRID / POLYDATA; ASCII and big-endian binary; POINTS, CELLS/CELL_TYPES, VERTICES/LINES/POLYGONS/TRIANGLE_STRIPS | Structured datasets and unknown sections reject before fallback |
| Legacy attributes | SCALARS, VECTORS, NORMALS, TENSORS, FIELD, TEXTURE_COORDINATES (1–3), COLOR_SCALARS (1–4) | Numeric fields retain original association and values; ASCII colors are 0..1, binary colors are raw 0..255 bytes |
| Legacy lookup/metadata | Validated custom RGBA LOOKUP_TABLE and dataset FIELD | Named warnings: tables are not applied to the Viewer and dataset metadata is not exposed as nodal/elemental fields |
| XML VTU/VTP | Multiple Pieces, ASCII, inline binary, appended base64/raw, zlib; scalar/vector/tensor roles | Unsupported Piece sections, missing/duplicate field names and malformed arrays reject |
| XML polyhedra | Legacy faces/faceoffsets; XML 2.3 face_connectivity/face_offsets/polyhedron_to_faces/polyhedron_offsets | XML 2.3 uses native VTK ON; incomplete/type-invalid arrays reject |
| VTM / `.vtm.series` | Nested VTM and VTU/VTP/Legacy leaves; ordered hierarchical blocks; independent moving/remeshed snapshots and fractional times | General cells require native VTK ON; unsupported composite sections and duplicate block names reject; coordinated animation remains N06 |

Legacy numeric types are float/double, signed/unsigned 8/16/32-bit integer names,
`vtkTypeInt32`, `long_long`, `vtkTypeInt64` and `unsigned_long_long`. Ambiguous
platform-dependent `long` and unknown types reject. Values must fit the declared
type; Legacy integers beyond ±(2^53−1) reject rather than lose precision during
normalization. Coordinates must be finite and representable as internal float;
field storage uses double. XML Int64/UInt64 fields beyond the exact double range
also reject. XML integer connectivity is validated before narrowing.
Dataset FieldData is validated and warned by name; TimeValue is reserved for time.
Native nonnumeric attributes issue a warning instead of disappearing silently.

Fallback is permitted only for recognized portable topology gaps: unsupported
concrete cells, PolyData Lines, native empty cells and modern polyhedron encoding.
Malformed syntax, unknown formats/sections or invalid mapping do not trigger it.
Warnings from both attempts survive successful fallback. Mapping requests keep
their explicit portable outcome. Structured/parallel XML, PVD/PVTU/PVTP, VTKHDF
and Abaqus ODB are outside this format contract.
Legacy polyhedron cell 42 does not have a supported face normalization path;
use a validated VTU polyhedron. The 48-cell SDK matrix establishes its tested
VTU/native scope, not every concrete cell in every format.

`vtk_format` covers Legacy texture/color/normal values, binary raw colors, custom
lookup tables, metadata, invalid types/sections/attributes, and XML duplicate names
or unsupported sections. The native import and DRAW replacement regressions verify
that rejected imports keep the live model. Full ON 52/52 and OFF 17/17 pass; focused
format/import cases each pass three times on the N04.3 source. Later full-suite
counts are recorded in the N04 queue.

## Series and Recursive References

Each `MeshData::SeriesSteps()` entry retains its own geometry, original topology,
fields, manifest time and source. All steps pass before publication. Manifest times
are finite and strictly increasing; matching field identity, association, role,
component count/names and tuple counts are required. Stable geometry also exposes
multi-step fields on the primary mesh. Changing geometry exposes only initial
fields there, with a warning; later fields belong to their own snapshots.
Single static datasets without TimeValue preserve the existing field step `-1`
for default DRAW binding. A valid dataset TimeValue supplies time with step `0`;
series manifest time and step override either default.
XML FieldData should declare NumberOfTuples (1 for TimeValue) for SDK import.

VTM reads XML children in order, preserves named block paths, offsets original
generic node/face indices and promotes merged scalar storage to double. Fields
missing in some blocks produce a named warning and NaN values. Incompatible roles
or component schemas reject. The legacy `SetMeshDataTree()` mutation mode rejects;
create a fresh reader for a complete time step. Failed reads expose no merged tree
while previously returned snapshots remain intact.

By default readers resolve references beside each manifest without copying files.
Enable physical local staging explicitly:

```cpp
loader.SetVtkStagingDirectory("D:/workspace/vtk-imports");
// Viewer OpenMesh appends optional stagingDirectory and VtkResourceLimits arguments.
```

```tcl
scvreadmesh model source.vtm -vtkstaging D:/workspace/vtk-imports -display 1
```

`VtkReferenceStager` creates a fresh `vtk-import-*` attempt directory, copies each
canonical file into a separate numbered subdirectory and rewrites VTM/series
references relative to the staged manifest. Duplicate basenames cannot overwrite
each other. Staged references remain usable after the source tree is moved away.
Cycles, excessive file/depth/byte budgets, missing references and invalid staging
destinations reject with source, parent reference and resolved-path context.

Staging failure cleans only its newly created attempt directory. Successful staging
is retained, including when subsequent dataset validation fails; `stagedSource`
lets the caller inspect it. Callers own successful staging-directory cleanup.
Staging traversal and dataset decoding each use their configured logical budgets;
the source files must remain stable during this local copy operation. Remote and
archive reference trees are outside this local-filesystem staging API.

The ordinary Viewer optional report receives original `source`, `stagedSource`,
counts, warnings and contextual errors. DRAW `scvreadmesh` prints the loader's
diagnostics and retains its old object on failure; the CutView `importreport`
dictionary includes `stagedSource` (empty for its current unstaged read adapter).
`vtk_reference`, `draw_vtk_reference` and its visual variant verify isolated nested
references, mixed numeric precision, NaN gaps, changing counts, general/empty cells,
failure cleanup and unchanged live objects/pixels.
