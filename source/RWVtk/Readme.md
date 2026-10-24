# RWVtk

Native VTK import into `RWCae::MeshData`: Legacy UnstructuredGrid/PolyData,
XML VTU/VTP, VTM and `.vtm.series`. The supported formats, field semantics,
validation rules and explicit exclusions are in
[the import contract](../../DOCUMENT/cut-view-import.md).

- `VtkBaseReader` owns message policy and reader reports.
- `VtkBaseXMLReader` parses XML, byte order, inline/appended arrays and zlib.
- `VtkLegacyReader` validates ASCII/big-endian sections and normalizes numeric data.
- `VtkUnstructuredGridReader` / `VtkPolyDataReader` implement all supported concrete topology without the VTK SDK.
- `VtkNativeReader` is a compatibility adapter to the repository-owned parser.
- `VtkValidationReader` validates original numeric arrays/topology.
- `VtkDatasetReader` routes dataset leaves through the same self-contained parser.
- `VtkMultiBlockDataReader` reads referenced blocks; `VtkSeriesReader` reads manifests.
- `VtkReferenceStager` copies/relinks local reference trees into isolated attempt directories.
- `VtkResourceLimits` configures pre-allocation budgets shared by reader children.
- `VtkDataArray` validates numeric arrays and component roles.

PVD/parallel XML and on-demand streaming are not implemented. This native track
excludes Web/WASM. Do not infer format support from VTK SDK capabilities alone;
the application validates and reports its own contract.
Series snapshots retain complete per-step geometry/fields; changing geometry does
not bind later fields to the first mesh. Animation and Viewer step coordination are
separate work. Legacy polyhedron 42 requires VTU; general cells are always available.

The repository-owned `ViewCaeGeom::MeshCellKernel` supplies topology, finite-element
shape functions, inverse maps and triangulation. There is no external VTK build or
runtime dependency. Current validation is in [the audit](../../DOCUMENT/vtk-independent-audit.md).

N04.8 final native closure: Final VTK ON **58/58**, OFF **22/22**, every case three times (174 / 66 executions; 547.47 / 41.50 seconds). DRAW/Viewer-disabled import/reference tests **2/2** each pass three times (0.52 seconds).
This N04 record is historical; see [its audit](../../DOCUMENT/cut-view-n04-audit.md)
for the original support boundaries and reproduction.
