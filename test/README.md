# Cut View Tests

## Build

The configured native build uses `/home/hyj/Desktop/workspace/OCCT_private/install`.

```bash
cmake -S . -B build -DOCCTV_BUILD_TESTS=ON -DOCCTV_BUILD_DRAW=ON
cmake --build build --target ViewDraw occtviewer_tests cut_view_tests quadric_cut_tests cut_view_benchmark --parallel 8
ctest --test-dir build --output-on-failure
```

`OCCTV_TEST_DRAW_VISUAL` defaults to OFF. Enable it when a working desktop X display and
its `DISPLAY`/`XAUTHORITY` environment are available:

```bash
cmake -S . -B build -DOCCTV_BUILD_TESTS=ON -DOCCTV_TEST_DRAW_VISUAL=ON
ctest --test-dir build --output-on-failure
```

CTest checks explicit PASS/FAIL markers because DRAW can return exit code zero after a Tcl failure.
The graphical test also checks nonblank/colorful image pixels, not merely successful PNG creation.

## Run DRAW Directly

```bash
OCCT=/home/hyj/Desktop/workspace/OCCT_private/install
export LD_LIBRARY_PATH="$PWD/build/libd:$OCCT/lib:${LD_LIBRARY_PATH:-}"
export CSF_OCCTResourcePath="$OCCT/share/opencascade/resources"
export DRAWHOME="$OCCT/share/opencascade/resources/DrawResources"
export DRAWDEFAULT="$DRAWHOME/DrawDefault"
export CSF_VIEWDRAWDefaults="$PWD/source-draw/ViewDraw"
"$OCCT/bin/DRAWEXE" -b -f test/cae/cut_view.tcl
"$OCCT/bin/DRAWEXE" -b -f test/cae/cut_view_visual.tcl
"$OCCT/bin/DRAWEXE" -b -f test/cae/cut_view_interaction.tcl
"$OCCT/bin/DRAWEXE" -b -f test/cae/cut_view_iso.tcl
"$OCCT/bin/DRAWEXE" -b -f test/cae/cut_view_quadric.tcl
env CUTVIEW_QUADRIC_VISUAL=1 "$OCCT/bin/DRAWEXE" -b -f test/cae/cut_view_quadric.tcl
"$OCCT/bin/DRAWEXE" -i -f test/cae/cut_view_controls.tcl
```

`cut_view.tcl` requires no graphics window. It checks all eight masks, areas/volumes, continuous
plane updates, point interpolation, both cell modes, surface intersection lines, coplanar cells,
CRUD/snapshots, deformation, stress resultants, heat flow, series fields, multiple Pieces,
polyhedra, and generated binary/base64/raw/zlib fixtures.

`cut_view_visual.tcl` checks cloud rendering, drag frames, constant/projected CellData, multiple
sections, and colored surface lines. Direct invocation writes to `build/cut-view-images/`;
CTest writes to `build/test/native/draw-images/`. Set `CUTVIEW_OUTPUT_DIR` to change the location.
Use the desktop session's X authorization, not a hardcoded session-specific cookie filename.

Use the existing `VIEWDRAW` resource for all viewer commands, including cuts; no separate
`CUTVIEW` resource is needed. The build's VS Code launch configuration loads it automatically.
Restart an existing debug session after changing its environment or debugger setup.

For an interactive demonstration, run this single command in DRAW:

```tcl
source /home/hyj/Desktop/workspace/OCCT_Viewer/test/cae/cut_view_demo.tcl
```

The demo resolves fixture paths relative to the script, can be sourced repeatedly, and leaves DRAW
running. It opens a Cut View Manager and nodal contours at x=0.25. Select a row without activating it;
toggle Show/Below/On/Above separately. Drag the plane region or blue normal arrow to translate;
drag a rotation ring to rotate. Numeric entries and the Translate/Rotate slider also update sections.
The panel includes CRUD, single/multiple cuts, result/component/step selection, CellData modes, and
definition-only JSON save/load. Closing it removes positioning aids, not the cut model.
Disabling all cuts displays the complete source model. An enabled cut with mask zero remains blank.
The launch configuration does not install `catch throw`;
handled input errors return Tcl errors without a default exception breakpoint.

For an already-loaded model, source `test/cae/cut_view_manager.tcl` and run `scvcutmanager model`.
Opening an empty manager creates inactive central X/Y/Z definitions without clipping the source.
`scvcutview model replace file.vtu` imports before replacing the current model; failures preserve
the old model. The panel rebuilds its fields/steps after replacement and closes when the model is cleared.
This is a native plane/isosurface/curved-cut manager, not full Abaqus parity;
see `DOCUMENT/cut-view-development.md` and the ordered queue in `DOCUMENT/cut-view-execution.md`.

For an interactive isosurface colored by a separate field:

```tcl
source /home/hyj/Desktop/workspace/OCCT_Viewer/test/cae/cut_view_iso_demo.tcl
```

Choose Plane or Isosurface next to Create. Isosurfaces have a separate Cut Field/component/step
binding and a live Threshold slider; changing Result does not change that binding. They do not
use the plane's translation arrow or rotation rings. Elemental cut fields project the selected
scalar component with equal-weight averaging, including a selected magnitude/invariant.
Isosurface cutting currently uses first-order linear edge interpolation; second-order meshes
are rejected explicitly. Definition snapshots are version 3 and still restore version-1 planes
and version-2 plane/isosurface definitions.

For analytic sphere/cylinder cuts, run in DRAW:

```tcl
source /home/hyj/Desktop/workspace/OCCT_Viewer/test/cae/cut_view_quadric_demo.tcl
```

Sphere/Cylinder creation, center coordinates, cylinder axis, numeric radius, held radius-slider
motion and three-dimensional manipulation are available in the same manager. Curved cuts use
OCCT boolean intersections, including enclosed intersections with same-sign original vertices;
they are not field isosurfaces. The cylinder is infinite, and construction caps are excluded.
Without `OCCTV_USE_VTK`, only linear triangles and tetrahedra are accepted for curved cuts.
With VTK enabled, a common plane/sphere/cylinder backend evaluates original VTK shape functions;
see [cell support and limits](../DOCUMENT/vtk-cell-support.md).
Generated portions are tessellated boundary meshes, not new volume cells. Nodal values use original
FE weights (barycentric for linear triangles/tetrahedra); elemental values retain source provenance
or use the existing projection.
`eval` identifies these meshes with `meshKind boundary` and reports OCCT area/volume measures.
Snapshots still contain definitions only, not source data, styles or deformation.

`quadric_cut_tests` checks enclosed/partial/outside/tangent cuts, exact measures, adjacent-cell
internal-wall removal, provenance/interpolation, CellData, deformation, invalid meshes and JSON.
`cut_view_quadric.tcl` adds DRAW acceptance; `CUTVIEW_QUADRIC_VISUAL=1` checks all rendered masks,
held pointer translation/rotation, display synchronization before lazy evaluation, changing pixels,
drag rollback, close and replacement during dragging. CTest images are in
`build/test/native/draw-quadric/`. Fixture passes do not establish large-mesh interactive performance.

`cut_view_iso.tcl` checks all masks, live values, independent cut/contour fields, CellData,
surface lines, CRUD, invalid bindings and JSON migration. Set `CUTVIEW_ISO_VISUAL=1` to test
rendered masks, the Tk threshold slider and manager lifecycle. CTest's graphical variant writes
to `build/test/native/draw-iso-images/`, including `iso_manager_panel.png` when ImageMagick is available.

`cut_view_interaction.tcl` tests real AIS mouse-event routing, plane/arrow/ring dragging, cancellation,
live nodal/element/surface interpolation, Tk controls, selection versus activation, and save/load.
It checks completed presentation revisions before lazy `eval`, asserts button-held updates and
changing rendered pixel checksums, tests every portion mask, constant CellData, and large-deformation
guide/range bounds. This is plane/mesh intersection acceptance, not an isosurface test.
It writes scene frames and (when ImageMagick is installed) a native manager screenshot to
`build/test/native/draw-interaction/` under CTest, or `build/cut-view-interaction/` directly.

`cut_view_controls.tcl` runs in interactive `-i` mode because batch initialization does not expose
the Tk/DRAW `grid` collision. It inventories all 32 controls and tests actual popdowns/file dialogs,
button/checkbutton events, held slider motion, numeric validation, all rotation axes, field components/
steps, cancellation, empty states, model replacement and close during dragging. It writes
`build/test/native/draw-controls/manager_controls.png` when ImageMagick is installed.
Sphere/cylinder creation, center editing, radius validation and held slider motion, cylinder
rotation, manipulator toggles, copies and real save/load dialogs are included.

## General VTK Cells and External Fixtures

The local optional SDK is under `build/deps/vtk/sdk/usr`. Configure a different SDK using
`VTK_DIR`; the VTK source tree alone is not an installed SDK.

```bash
cmake -S . -B build -DOCCTV_USE_VTK=ON \
  -DVTK_DIR="$PWD/build/deps/vtk/sdk/usr/lib/x86_64-linux-gnu/cmake/vtk-9.5" \
  -DCMAKE_PREFIX_PATH="$PWD/build/deps/vtk/sdk/usr" \
  -DOCCTV_VTK_TEST_DATA=/home/hyj/Desktop/workspace/vtk/build/ExternalData/Testing/Data
cmake --build build --target ViewDraw vtk_cell_cut_tests general_cut_cache_tests cut_view_tests quadric_cut_tests --parallel 8
ctest --test-dir build --output-on-failure
```

`vtk_cell_cut` generates the 48 concrete-cell fixtures and tests original-cell field weights,
plane/sphere/cylinder intersections, import round trips, rational/anisotropic metadata,
343-node connectivity, quadratic fields, generic integration and malformed metadata.
`draw_cut_view_vtk_cells` tests all eight masks and point/CellData contours. Its visual variant
checks point and line rendering, uncut source views and actual held-pointer plane recomputation.
CTest automatically generates required fixtures; screenshots go to
`build/test/native/draw-vtk-cells/`. Direct DRAW entry:

```tcl
source /home/hyj/Desktop/workspace/OCCT_Viewer/test/cae/cut_view_vtk_cells.tcl
```

This is a batch acceptance script and exits DRAW. For an interactive high-order model:

```tcl
source /home/hyj/Desktop/workspace/OCCT_Viewer/test/cae/cut_view_vtk_demo.tcl
```

It defaults to the generated Bezier hexahedron. To use an external dataset, first set
`vtk_cut_demo_file` to its absolute path. It leaves the manager open for actual dragging.
For standalone native probes, run
`build/bind/vtk_cell_cut_tests --external /absolute/file.vtu` with the configured library paths.
External tests are registered only when the optional data directory contains their fixtures.
The external files are not modified. Reserved/abstract cell IDs, exact arbitrary-order geometry,
curved/nonconforming internal-wall removal, and full-frame performance are not accepted by this matrix.

## Shared Native Source Cache

`general_cut_cache` verifies planar, straight-sided adjacency for hex/voxel/quadratic-hex/
Lagrange-hex/polyhedron cells, side/source area and volume, deterministic coincident-interface
CellData, all masks, cached-versus-fresh cuts and node/topology/deformation invalidation.
Warm motion and field edits retain source evaluators, lazy shapes and the OCCT BVH;
a 24-cell chain builds one candidate shape for section-only evaluation.
N01 did not accept curved/nonconforming interfaces; see the N02 tests below.
Bounded-memory eviction remains pending.

CTest generates `build/test/native/general-cache-fixtures/two_lagrange_hex.vtu`.
`draw_cut_view_cache` checks the same shared-source behavior through DRAW; its visual
variant checks actual held-pointer movement, cache counts and completed display revisions
before lazy evaluation, original FE values, changed cloud pixels and abort rollback.
Screenshots are in `build/test/native/draw-cache/`.

```bash
ctest --test-dir build -R 'general_cut_cache|draw_cut_view_cache' --output-on-failure
ctest --test-dir build --output-on-failure --repeat until-fail:3
```

Supply the active desktop's `DISPLAY`/`XAUTHORITY` for visual tests. After building and running
`general_cut_cache_tests` to generate its fixture, direct batch acceptance is:

```tcl
source /home/hyj/Desktop/workspace/OCCT_Viewer/test/cae/cut_view_cache.tcl
```

This script exits DRAW. `scvcutview model stats` returns cumulative counters for the current
source epoch: `epoch`, `preparations`, `preparedCells`, `shapeBuilds`, `bvhBuilds`, `boxTests`,
`candidates`, `booleans`, `filteredFaces`, `unmergedGroups`. It does not trigger lazy evaluation.
The native equivalent is `MeshCutViewManager::Statistics()`.
Final N01 acceptance is 25/25 native/DRAW cases, each passing three consecutive runs;
the VTK-disabled Release build's three native cases also pass three times. This is correctness
acceptance, not a full-frame FPS benchmark.

## Geometry Tolerance and Curved Interfaces

Build `cut_geometry_tests` with VTK enabled. `cut_geometry` generates
`build/test/native/cut-geometry-fixtures/joined_curved_hex.vtu` and checks warped-hex
reference convergence, joined quadratic/triquadratic/Lagrange/rational-Bezier hex
interfaces, curved lines, relative model scaling and float-coordinate resolution.
It also checks invalid options, unsatisfied refinement/primitive ceilings, no partial
source publication and recovery after failure. Effective geometric tolerance and output
float resolution are different quantities, exposed by `stats`.

```tcl
scvcutview model geometry
scvcutview model geometry 0.007 0 7 12 2000000
scvcutview model stats
```

Arguments are absolute tolerance, relative tolerance, face/line refinement ceilings and
maximum source primitives. Changed valid options invalidate the geometric epoch;
invalid or identical options preserve it. The ceiling bounds geometric preparation,
not all session memory or output/shape cache allocation.

`draw_cut_view_geometry` checks shared curved walls and numeric validation. Its visual
variant verifies actual held-pointer updates, FE interpolation, completed display
revisions, stable prepared primitives/cache counters and changing cloud pixels before
release. Images are in `build/test/native/draw-geometry/`.

```bash
cmake --build build --target cut_geometry_tests ViewDraw --parallel 8
ctest --test-dir build -R 'cut_geometry|draw_cut_view_geometry' --output-on-failure
```

After generating fixtures, direct DRAW batch acceptance is
`source /home/hyj/Desktop/workspace/OCCT_Viewer/test/cae/cut_view_geometry.tcl`; it exits DRAW. Supply the
active desktop's X environment for visual acceptance. Numerical convergence and
conforming reference fixtures do not prove exact arbitrary-order geometry or an
interactive million-cell frame budget. N03-N09 remain scheduled.
N02 numerical acceptance is 28/28 native/DRAW cases, each passing three times;
the VTK-disabled Release build's full six-case native/DRAW suite also passes three
times. Analytic second-order source boundaries remain the next N02 increment.

## Analytic Source Geometry

`cut_analytic_tests` generates `build/test/native/cut-analytic-fixtures/bowed_tetra.vtu`.
`cut_analytic` checks exact bowed-tetra volume/section area and quadratic FE fields,
warped quadratic/triquadratic/Lagrange/rational-Bezier hexahedra, independently integrated
rational reference volume, enclosed spheres, exact quadratic curves and planar quadratic
polygons. It also checks source reuse, unsupported analytic order rejection and numerical
fallback recovery. Original FE values and geometric measures are separate assertions.

```tcl
scvcutview model geometrymode required
scvcutview model geometry 0.03 0 7 12 2000000
scvcutview model eval cut
scvcutview model stats
```

Modes are `numerical`, `preferred` (default), and `required`. Supported original polynomial/
rational charts through degree six become OCCT Bezier source boundaries; unsupported charts
fall back only in preferred mode. `eval` reports `sourceGeometry`; `stats` adds `analyticCells`,
`numericalCells`, and the off-grid `analyticResidual`. These do not upgrade float display storage
or imply exact arbitrary-order geometry. Native affine fast paths remain available; explicit
required mode routes non-simplex native cells to the general backend when VTK is enabled.

`draw_cut_view_analytic` checks actual mode switching, exact source measures, quadratic and
constant CellData. Its visual variant requires held-pointer synchronization before lazy eval,
unchanged source caches, interpolated values, changing pixels and abort rollback. Images are in
`build/test/native/draw-analytic/`. Direct batch script (exits DRAW):

```tcl
source /home/hyj/Desktop/workspace/OCCT_Viewer/test/cae/cut_view_analytic.tcl
```

```bash
cmake --build build --target cut_analytic_tests ViewDraw --parallel 8
ctest --test-dir build -R 'cut_analytic|draw_cut_view_analytic' --output-on-failure
```

The new adapter also restores original Bezier-wedge face-node weights explicitly because some
VTK versions index those weights with the face number instead of the face-node number. The
existing six-family rational regression remains mandatory. All 31 native/DRAW cases pass
three consecutive runs per case; the VTK-disabled native/DRAW build passes all six cases
three times. G03's remaining reference evidence and N03-N09 are not completed by this increment.

## OCCT SDK Fixes

The local OCCT SDK was rebuilt to preserve Tk's `grid` command alongside DRAW's numeric grid,
avoid blocking `XNextEvent` from Tcl callbacks, and reset detection after drag termination.
Restart DRAW/debugger sessions to load the updated libraries. The patch is recorded in
`DOCUMENT/occt-cut-view-sdk.patch`; apply it to a compatible OCCT checkout before rebuilding:

```bash
cmake --build /home/hyj/Desktop/workspace/OCCT_private/build --target TKDraw TKViewerTest --parallel 8
cmake -P /home/hyj/Desktop/workspace/OCCT_private/build/src/TKDraw/cmake_install.cmake
cmake -P /home/hyj/Desktop/workspace/OCCT_private/build/src/TKV3d/cmake_install.cmake
cmake -P /home/hyj/Desktop/workspace/OCCT_private/build/src/TKViewerTest/cmake_install.cmake
```

Run the full suite with a working session's `DISPLAY`/`XAUTHORITY`. Unit/pixel test passes do not
establish million-cell 15 FPS performance or completion of the production Vue/WASM manager.

Individual commands (initialize a viewer with `vinit` before `display`):

```tcl
pload -VIEWDRAW
scvcutview model read test/native/fixtures/tetra.vtu
scvcutview model add cut1 1 0 0 -0.25
scvcutview model visibility cut1 3
scvcutview model field point temperature 0
scvcutview model eval cut1
scvcutview model display
scvcutview model move cut1 1 0 0 -0.5
```

Masks are negative=1, on=2, positive=4. `display` requires `vinit`; other operations work in batch.
`field` accepts an optional timestep index. `deform` uses the repository's model-size-normalized
scale convention. `snapshot`/`restore` cover definitions only.

## Native Interface

`ViewCaeGeom::MeshCutViewManager` owns independent cut branches. For fast native planes/isosurfaces, `MeshCutResult::Roots` retain
accepted original cells; `Portions` contain generated cut cells/faces. `Fields` index those generated
meshes; `SourceField` describes retained source data. Surface `LineField` indexes section nodes for
nodal data or section lines for elemental data.
For spheres/cylinders and general VTK planes, `BoundaryMesh` is true; `Portions` contain boundary
triangles, section lines or points. `Volumes`/`Areas` carry OCCT measures of the original affine
source or tessellated high-order boundary. Do not treat triangle counts as volume-cell counts.

```cpp
Handle(ViewCae::InteractiveCaeCutView) cut = new ViewCae::InteractiveCaeCutView(mesh);
cut->Cuts().AddPlane("cut1", Graphic3d_Vec4d(1, 0, 0, -0.25));
cut->Cuts().SetVisibility("cut1", 3);
cut->SetField(temperature, 0);
cut->Synchronize();
context->Display(cut, false);
```

After editing cuts, synchronize and redisplay the object. Returned geometries are live until the
next update, not immutable snapshots. Mutating source nodes/connectivity requires `InvalidateGeometry()`;
mutating fields in place requires `InvalidateFields()`. The caller controls visibility of the source object.
Projected CellData uses equal-weight averaging at shared nodes, not volume-weighted recovery.

`MeshCutIntegration` integrates supplied volume-section stress/flux fields, using the positive plane
normal and an explicit component map. It rejects missing values, unsupported surface sections and
isosurfaces (which need a separate local-normal mapping).
Tensor maps index physical components, excluding generated vector magnitude.

## Performance

```bash
cmake --build build/cut-release --target cut_view_benchmark --parallel 8
export LD_LIBRARY_PATH="$PWD/build/cut-release/lib:$OCCT/lib:${LD_LIBRARY_PATH:-}"
build/cut-release/bin/cut_view_benchmark 100 2 20
build/cut-release/bin/cut_view_benchmark 100 7 20
build/cut-release/bin/cut_view_benchmark 100 2 20 20
```

Arguments are grid size, visibility mask, frame count, and movement span. Size 100 creates one million
hexahedra. The default span stays in one topology interval; span 20 crosses cell layers. Output explicitly
excludes rendering. See `DOCUMENT/cut-view-development.md` for measured results and outstanding acceptance.
