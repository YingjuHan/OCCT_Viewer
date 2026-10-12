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
- `VtkMultiBlockDataReader` reads referenced blocks; `VtkSeriesReader` reads manifests.
- `VtkDataArray` validates numeric arrays and component roles.

PVD/parallel XML and on-demand streaming are not implemented. This native track
excludes Web/WASM. Do not infer format support from VTK SDK capabilities alone;
the application validates and reports its own contract.
