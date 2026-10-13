# Repository-owned VTK processing

The external VTK SDK has been removed from the application and its tests. VTK
remains a file-format name: no VTK headers, targets, libraries or runtime are
required. The previous `OCCTV_USE_VTK` option is removed; old cache values do not
select a reduced implementation. OCCT and zlib remain dependencies.

## Implementation

- `RWCae/MeshCellTopology.h` defines the 48 concrete format IDs, dimensions,
  fixed cardinalities and higher-order degree/cardinality checks.
- `RWVtk` implements Legacy ASCII/big-endian binary, XML ASCII/inline/appended
  base64/raw/zlib, multiple Pieces, PointData/CellData roles, mappings, VTM
  references and independent time-series snapshots. The old SDK fallback is gone.
  Dataset reports retain the public `portable` backend name and one attempt.
  `VtkNativeReader` is a compatibility adapter to the same implementation.
- Old `faces/faceoffsets` and XML 2.3 face arrays are checked for scalar shape,
  offset ordering, range, original-cell membership and complete consumption.
  Mixed old face streams retain their cursor across ordinary cells. Ambiguous
  old/modern layouts and malformed unused modern arrays reject before publication.
- `ViewCaeGeom/MeshCellKernel` supplies reference ordering, faces/edges,
  fixed/serendipity polynomial spaces, collapsed/rational pyramid maps,
  Lagrange and Bernstein bases, rational Bezier weights, polygon/polyhedron MVC,
  convex hulls, simplex seeds and inverse mapping. Existing OCCT analytic
  conversion, boolean cuts, provenance, trace policies and integration use it.
- Isosurface replacement tetrahedra now have coherent positive orientation.
  This fixes closed-region boolean unions without relaxing volume/area assertions.
- Polyhedron MVC uses cross products and scalar triple products for spherical
  angles. The signed sine avoids cancellation in `sqrt(1-cosine*cosine)` near a
  face. A translated skew cage checks partition and affine reproduction on,
  inside and outside its faces; its previous maximum coordinate error was
  `0.00286911`, with the new regression requiring less than `1e-7`.
- Tests write generated ASCII fixtures under each build directory. Immutable fixed-cell numerical references
  were captured once with VTK 9.5.2 during development; validation does not call
  or link that SDK. High-order invariants and geometric/physical references are
  checked separately from these captured values.

Format and cell ordering follow the [VTK XML format specification](https://docs.vtk.org/en/v9.5.2/design_documents/VTKFileFormats.html).
The implementation retains the application's bounded [cell contract](vtk-cell-support.md)
and [import contract](cut-view-import.md).

## Compatibility

Leaves previously routed to the SDK retain whole-dataset Generic cells and the
former Double/default-name field schema. This selection happens before cells or
fields are published, across all Pieces; cached cell types are decoded and charged
once. This preserves ordinary Viewer CellData ownership and v3 source fingerprints.

`native_session` compares the mixed polyline/quad, Float32 vector/scalar source in
`test/native/fixtures/sdk_session_general.vtu` against the immutable fingerprint
`231807589d04ac6a8ace2311ad2bccf7576ae65d7fdcba22406f105ffc31b3e2`, captured by
the old `047207f` application with VTK 9.5.2. No relaxed digest matching or file
integrity bypass is used. Ordinary previously supported leaves keep their typed
representation. Mapping requests continue using the mapping contract.

## Verification

- Viewer/DRAW/Tk and Viewer/DRAW-disabled native builds completed successfully;
  logs are `build/vtk-independent-viewer-converged.log` and
  `build/vtk-independent-headless-converged.log`.
- Headless CTest: **25/25 passed**, `130.35 s`, recorded in
  `build/mingw-import-headless/vtk-independent-converged.log`.
- Viewer/DRAW/Tk CTest: **72/72 passed**, `345.43 s`, recorded in
  `build/mingw/vtk-independent-converged.log`. This includes the 48-cell matrix,
  six official datasets, analytic/numerical cuts, high-order isosurfaces, union,
  Viewer session compatibility, Tk controls, image checks and held-pointer tests.
  Polyhedron affine-coordinate maximum error is `2.27374e-13` in the independent
  skew-cage regression. Missing-face rejection passes in both test paths.
- Binary import audit: **55** complete-build and **41** headless DLL/executable
  artifacts have no external VTK DLL imports. Each build retains
  `vtk-dependency-audit.json`.
- A fresh Release configuration succeeded with `VTK_DIR` set to a nonexistent
  directory; CMake reported this variable unused. Evidence is
  `build/vtk-independent-configure.log`. This was a configure check, not a
  verified Release build.
- Source/CMake scans found no VTK SDK includes, imported/link targets or library
  calls in the active source/test tree; `git diff --check` passes.

The migration counts above are one complete run of each final configuration.
Earlier failed logs remain in `build/` for diagnosis. The three-run ON/OFF and
million-cell performance evidence in the milestone audits predates this migration;
it is not a new performance measurement for the repository-owned cell kernel.

The upstream `Common/DataModel/Testing/Cxx/TestPolyhedron4.cxx` explicitly identifies
`nonWatertightPolyhedron.vtu` as a watertight grid with a misleading name. Its former
rejection expectation was incorrect and depended on the old implementation's
failure. It now requires successful plane/sphere/cylinder sections and affine FE
mapping. A cube missing one face independently checks rejection in the kernel and
the complete cut pipeline; manifold validation remains required.

Reproduction:

```powershell
cmake -S . -B build/mingw -U VTK_DIR -U OCCTV_USE_VTK `
  -DOCCTV_BUILD_TESTS=ON -DOCCTV_BUILD_DRAW=ON `
  -DOCCTV_BUILD_WEB_VIEWER=OFF -DOCCTV_TEST_DRAW_VISUAL=ON `
  -DOCCTV_VTK_TEST_DATA=D:/workspace/VTK/test-data-9.5.2
cmake --build build/mingw --parallel 6
ctest --test-dir build/mingw --output-on-failure -j 4
./test/native/CheckVtkIndependence.ps1 -BuildDirectory build/mingw
```

`CheckVtkIndependence.ps1` inspects native DLL/executable import tables and rejects
external VTK DLL dependencies. It writes `vtk-dependency-audit.json` in the build
directory. Test paths include the project, OCCT, compiler and zlib runtimes.

## Boundaries

This change removes an implementation dependency; it does not add structured
VTK datasets, PVD/parallel XML, HDF, unsupported Legacy sections or Legacy
polyhedron-face normalization. Generic MDB export still rejects explicitly.
Enriched 7/15/21-node elements use Lagrange nodal semantics; Bezier elements require
standard Bernstein cardinality. Rational weights must be finite and positive.
The independent degree-1024 curve test checks finite coefficients, partition,
affine reproduction and the central binomial coefficient. It does not certify
arbitrary high-order geometric intersections, which remain bounded numerical work.
Existing float output storage, analytic degree/weight restrictions, refinement
ceilings and shared memory budgets remain part of the native contract.
Web/Vue/WASM builds and packaging have not been verified in this migration.

N01–N09 milestone commits and their historical ON/OFF records are retained.
