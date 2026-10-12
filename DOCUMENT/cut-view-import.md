# Native VTK Import Outcomes

N04.1 establishes the import diagnostic/publication boundary. N04.2 verifies
[ordinary Viewer display](cut-view-viewer.md). The remaining
format matrix, changing-topology series, resource limits
and reference staging work is tracked in [the N04 queue](cut-view-n04-plan.md).

`RWMeshConv::MeshLoader::VtkReport()` describes the most recent VTK import:

- `succeeded` identifies the overall outcome; `source` and `format` identify the input.
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
VTM and `.vtm.series` use their composite/series readers; they do not fall back to
the single-dataset native backend. Their current stable-geometry limitations remain
until N04.4. Uppercase `.VTK` filenames use the legacy native backend correctly.

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
checks and explicitly reports the lifecycle check as skipped. This item does not establish complete per-step staging or general-cell
ordinary Viewer rendering.

## N04.3 Format Contract

| Format | Supported data | Boundary |
| --- | --- | --- |
| Legacy `.vtk` | UNSTRUCTURED_GRID / POLYDATA; ASCII and big-endian binary; POINTS, CELLS/CELL_TYPES, VERTICES/LINES/POLYGONS/TRIANGLE_STRIPS | Structured datasets and unknown sections reject before fallback |
| Legacy attributes | SCALARS, VECTORS, NORMALS, TENSORS, FIELD, TEXTURE_COORDINATES (1–3), COLOR_SCALARS (1–4) | Numeric fields retain original association and values; ASCII colors are 0..1, binary colors are raw 0..255 bytes |
| Legacy lookup/metadata | Validated custom RGBA LOOKUP_TABLE and dataset FIELD | Named warnings: tables are not applied to the Viewer and dataset metadata is not exposed as nodal/elemental fields |
| XML VTU/VTP | Multiple Pieces, ASCII, inline binary, appended base64/raw, zlib; scalar/vector/tensor roles | Unsupported Piece sections, missing/duplicate field names and malformed arrays reject |
| XML polyhedra | Legacy faces/faceoffsets; XML 2.3 face_connectivity/face_offsets/polyhedron_to_faces/polyhedron_offsets | XML 2.3 uses native VTK ON; incomplete/type-invalid arrays reject |
| VTM / `.vtm.series` | Portable VTU/VTP references, hierarchical blocks and fractional times | Immutable changing-topology staging and recursive references are subsequent N04 gates |

Legacy numeric types are float/double, signed/unsigned 8/16/32-bit integer names,
`vtkTypeInt32`, `long_long`, `vtkTypeInt64` and `unsigned_long_long`. Ambiguous
platform-dependent `long` and unknown types reject. Values must fit the declared
type; Legacy integers beyond ±(2^53−1) reject rather than lose precision during
normalization. Coordinates must be finite and representable as internal float;
field storage uses double. XML integer connectivity is validated before narrowing.
Dataset FieldData is validated and warned by name; TimeValue is reserved for time.
Native nonnumeric attributes issue a warning instead of disappearing silently.

Fallback is permitted only for recognized portable topology gaps: unsupported
concrete cells, PolyData Lines, native empty cells and modern polyhedron encoding.
Malformed syntax, unknown formats/sections or invalid mapping do not trigger it.
Warnings from both attempts survive successful fallback. Mapping requests keep
their explicit portable outcome. Structured/parallel XML, PVD/PVTU/PVTP, VTKHDF
and Abaqus ODB are outside this format contract.

`vtk_format` covers Legacy texture/color/normal values, binary raw colors, custom
lookup tables, metadata, invalid types/sections/attributes, and XML duplicate names
or unsupported sections. The native import and DRAW replacement regressions verify
that rejected imports keep the live model. Full ON 52/52 and OFF 17/17 pass; focused
format/import cases each pass three times on the final source.
