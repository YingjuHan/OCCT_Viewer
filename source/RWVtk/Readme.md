# RWVtk

Native VTK import into `RWCae::MeshData`: Legacy UnstructuredGrid/PolyData,
XML VTU/VTP, VTM and `.vtm.series`. The supported formats, field semantics,
fallback rules and explicit exclusions are in
[the import contract](../../DOCUMENT/cut-view-import.md).

- `VtkBaseReader` owns message policy and reader reports.
- `VtkBaseXMLReader` parses XML, byte order, inline/appended arrays and zlib.
- `VtkLegacyReader` validates ASCII/big-endian sections and normalizes numeric data.
- `VtkUnstructuredGridReader` / `VtkPolyDataReader` implement portable topology.
- Optional `VtkNativeReader` retains concrete VTK cells and original fields.
- `VtkValidationReader` preflights original numeric arrays/topology before the SDK.
- `VtkDatasetReader` shares restricted fallback policy across composite leaves.
- `VtkMultiBlockDataReader` reads referenced blocks; `VtkSeriesReader` reads manifests.
- `VtkReferenceStager` copies/relinks local reference trees into isolated attempt directories.
- `VtkResourceLimits` configures pre-allocation budgets shared by reader children.
- `VtkDataArray` validates numeric arrays and component roles.

PVD/parallel XML and on-demand streaming are not implemented. This native track
excludes Web/WASM. Do not infer format support from VTK SDK capabilities alone;
the application validates and reports its own contract.
Series snapshots retain complete per-step geometry/fields; changing geometry does
not bind later fields to the first mesh. Animation and Viewer step coordination are
separate work. Legacy polyhedron 42 requires VTU; general cells need VTK ON.

N04.8 final native closure: Final VTK ON **58/58**, OFF **22/22**, every case three times (174 / 66 executions; 547.47 / 41.50 seconds). DRAW/Viewer-disabled import/reference tests **2/2** each pass three times (0.52 seconds).
See [the audit](../../DOCUMENT/cut-view-n04-audit.md) for current support boundaries and reproduction.
