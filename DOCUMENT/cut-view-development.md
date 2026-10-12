# Cut View Development

## Accepted Requirements

- Real mesh intersections while dragging, including interpolated point results and cell contours.
- Independent negative/on/positive visibility, all eight combinations, additive multiple cuts.
- Plane, cylinder, sphere, and field isosurface cuts; translation and plane rotation.
- Surface meshes produce colored intersection lines and clipped surface polygons.
- Cell contours offer original cell constants and a projected continuous mode.
- Deformation, time steps, regular/path slicing, cache control, and project persistence.
- Stress-based resultant forces/moments, mapped shell/beam section fields, heat flow.
- Mapped crack-distance and optimization-density fields can create automatic cuts.
- VTK legacy UnstructuredGrid/PolyData and XML VTU/VTP/VTM/VTM series, including binary and zlib.
- Native verification first; WASM integration requires an Emscripten/OCCT WASM SDK.
- Native target: one million volume cells at 15 fps, measuring geometry, fields, and display together.

Current execution is native only: Web/Vue/TypeScript and WASM tasks are excluded,
not completed. Project/session persistence below maps to the native session API.

Current Windows acceptance: VTK-enabled **32/32** and VTK-disabled **8/8**, each case
passing three consecutive runs. Expanded wedge/pyramid independent references and
recovery robustness are included. See [the current handoff](cut-view-native-handoff.md)
for SDK paths, logs, missing historical external fixtures and remaining N03 work.

## Verified Native Work

- [x] Native tests and DRAW batch/visual Tcl regressions under `test/`.
- [x] Legacy ASCII/big-endian binary and XML inline/appended/raw/zlib decoding.
- [x] Connectivity/tuple validation, integer narrowing checks, and reader diagnostics.
- [x] Multiple Pieces, VTM references, fractional series times, and vector/tensor roles.
- [x] Plane CRUD, enable/disable, normalized equations, and all eight portion masks.
- [x] Volume section faces, clipped sides, coplanar surface faces, and surface section lines.
- [x] Source provenance, linear nodal interpolation, constant CellData, and equal-weight nodal projection.
- [x] Native AIS contour rendering, including a dedicated textured section-line shader.
- [x] Deformation before intersection, field-step selection, and explicit cache invalidation.
- [x] Atomic JSON definition restoration and versioned definition snapshots.
- [x] Volume-section stress force/moment and heat-flux integration with explicit component maps.
- [x] Topology-preserving translation cache with full rebuild fallback.
- [x] DRAW/Tk plane manager: selection separate from activation, CRUD, eight masks, field/component/step selection.
- [x] Pointer-driven plane translation and OCCT rotation/translation manipulators with live mesh/field recomputation.
- [x] Drag cancellation restores geometry and fields; manager position follows pointer edits.
- [x] Interactive definition save/load, repeatable demo, font-aware native controls, and manager screenshots.
- [x] First-order field-isosurface cuts: below/on/above, volume faces/surface lines, live threshold,
  fixed field/component/step binding, and separate display contours.
- [x] Isosurface CellData cutting projects the selected scalar component, including magnitude/invariants,
  rather than averaging vector components before taking a magnitude.
- [x] DRAW/Tk isosurface creation, copy/edit/delete, field binding and threshold slider; JSON v2
  snapshots with atomic v1 plane migration and missing/ambiguous binding rejection.
- [x] Interactive DRAW/Tk control acceptance: 32/32 widget inventory, real menus/file dialogs,
  all buttons, editable numeric fields, errors, cancellation and empty-manager states.
- [x] Pre-release plane-drag checks for completed presentation synchronization and changing pixels,
  all eight masks, constant/projected CellData and deformed meshes beyond original bounds.
- [x] Native defaults/model replacement/close lifecycle, including active-drag cancellation.
- [x] Optional native VTK-cell backend: 48 concrete cell types, geometric planes/spheres/cylinders,
  original shape functions, rational/anisotropic metadata and arbitrary-length connectivity.
  See [the support matrix](vtk-cell-support.md) for tested scope and geometry limitations.
- [x] N01 shared source epochs, lazy OCCT shapes/BVH, planar adjacency and held-drag cache checks.
- [x] N02 numerical foundation: configurable tolerance/refinement/primitive limits,
  conforming curved reference interfaces and numerical convergence; 28/28 cases pass three times.
- [x] N02 analytic source increment: OCCT polynomial/rational Bezier boundaries, explicit
  numerical fallback, exact bowed-tetra/warped-hex/curve/polygon references and held-pointer
  clouds; 31/31 native/DRAW cases pass three times.
- [ ] G03 remaining curved-family independent reference evidence and precision limits.
- [x] N02 expanded independent references: curved wedges `26/31/32/73/80`, rational
  Bezier wedge `80`, and pyramids `27/37`, with native and DRAW geometry/FE checks.
- [x] N03 display recovery foundation: region IDs, averaging thresholds, elemental-per-node
  discontinuities, vector magnitude before/after averaging and independent original physical fields.
- [ ] N03 complete tensor invariants, physical interface traces, OR boundary/cap merging,
  recovery manager widgets and session persistence.

These are native building blocks, not completion of the entire Cut View Manager plan.

## Abaqus Feature Comparison

Comparison baseline: Abaqus/CAE 2025 Visualization. The DRAW/Tk panel is a native development
interface, not the production Vue manager. See the official [view-cut overview](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-c-cutunderstand.htm),
[display controls](https://abaqus.uclouvain.be/English/SIMACAECAERefMap/simacae-t-cuthlpdisplay.htm), and
[slicing options](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-t-cuthlpslicing.htm).

| Capability | Current Native Status | Remaining Difference |
| --- | --- | --- |
| Manager list, select/show independently, create/edit/copy/rename/delete | DRAW/Tk implemented and tested | Production Viewer/TypeScript/Vue integration |
| Default X/Y/Z planes | Native `EnsureDefaultPlanes()` and DRAW/Tk lifecycle defaults; new model remains uncut | Production Viewer lifecycle integration |
| Below/on/above combinations, multiple cuts | All eight masks, single/multiple controls, uncut source when all cuts disabled | Overlap boundary merging and shared opacity behavior |
| Translate/rotate with numeric values and sliders | Plane controls and scalar-isovalue slider implemented | Explicit datum coordinate systems, reference axes and curved-shape controls |
| Mouse positioning | Plane picking, normal arrow, rotation rings, cancellation tested | Result-node/element picking and probe tools |
| Real mesh and nodal-result sections during motion | Original FE functions; 48-cell matrix and held-pointer volume-family checks | Exact curved geometry/convergence and large-model frame budget |
| Element-result sections | Constant/projected CellData, region/threshold and element-node recovery; display/physical mapping separated | Tensor invariants, explicit physical interface traces and manager recovery widgets |
| Plane/cylinder/sphere/isosurface shapes | Geometric cuts for concrete VTK cells; native first-order field isosurfaces | General internal walls, exact curved geometry and higher-order isosurfaces |
| Per-portion/per-cut plot options | Native shared `SetStyle()` API only | Per-cut styles, manager controls, legend/contour options |
| Deformed plots and following deformation | Native intersection, deformed nodal guide/range/fit bounds and held-drag regression | Material-following reference frames and production GUI controls |
| Field time steps and animation | Individual field steps selectable | Coordinated geometry/deformation animation and topology checks |
| Result caching | Conservative translation and field caches | User policy, memory limits, eviction, full-frame acceptance |
| Free-body forces/moments and heat flow | Volume-field numerical integration only | Vector overlays, summation/group options, shell/beam mappings |
| Regular/rotational/path slicing | Not implemented | Slice distribution, path definitions, interval resultants |
| Common options and cut-specific options | Basic manager controls only | Complete options dialogs and per-cut overrides |
| Session/project persistence | Definition-only JSON save/load | Mesh/field/style/deformation bindings and project migration |
| XFEM and optimization automatic cuts | Not implemented | Explicit distance/density mapping and special display semantics |
| CAD/global clipping synchronization | Existing CAD clipping remains separate | Cross-module synchronization and CAD sections |
| VTK-based source data | Optional VTK topology and line/point cuts; real fixtures tested | Ordinary viewer/browser integration, changing topology, bounded memory and UI diagnostics |
| Browser/WASM manager | Not implemented | SDK rebuild, typed API, UI, pointer and performance acceptance |

Pointer tests are in `test/cae/cut_view_interaction.tcl`: they pass events through
`ViewerTest_EventManager`/`AIS_ViewController`, verify current plane equations and interpolated
values before release, and check rendered pixels. Small-fixture interactivity is not evidence
that million-cell rendering meets 15 fps.

## Remaining TODOs

1. N01 shared-source/BVH and planar internal-wall foundations are verified. N02 adds conforming
   curved reference interfaces and consistent dyadic refinement; 28/28 cases pass three times.
   Ambiguous/mismatching interfaces stay visible. This does not close arbitrary-geometry
   or full-frame acceptance.
2. Add configurable geometric tolerances, conforming curved-face refinement and reference convergence
   tests. Original high-order FE interpolation and quadratic fields are tested; source geometry is
   still tessellated, not exact arbitrary-order geometry. Tolerance/limit controls and reference
   convergence and supported analytic Bezier source boundaries are implemented in N02;
   wedge/pyramid native and DRAW references are now included. Remaining families and
   the general support/precision contract still require bounded acceptance.
   Higher-order field isosurfaces remain open.
3. Complete multi-cut OR boundary merging/deduplication. Current cuts are independently evaluated and
   additively displayed, not chained as AND clips; overlapping side boundaries are not Boolean-unioned.
4. Complete per-cut styles, datum positioning/reference axes, result picking/probes, contour legends,
   and synchronization with CAD/global clipping state. Native plane guides/manipulators now work;
   result selection remains separate and incomplete.
5. Add regular/range/path slicing, coordinated time/deformation animation, cache policies, and eviction.
   Native pointer-driven plane updates now work; browser dragging and material-following reference
   frames are not connected to this manager.
6. Add mapped shell/beam section forces, thickness/section semantics, units handling, resultant overlays,
   and explicit physical interface trace selection. Original physical mapping is already separated
   from display recovery. Current integration is for supplied volume-section fields;
   unsupported surface sections fail explicitly.
7. Add automatic crack/optimization cuts from explicitly mapped distance/density fields.
8. Complete VTK Lines/polyline storage and rendering, remaining legacy sections, changing-topology time
   series validation, malformed/compressed/multi-offset coverage, and bounded-memory appended decoding.
   Optional native VTK import now handles lines for the cut viewer; portable/browser and ordinary
   viewer coverage remain pending. Series geometry is reused and requires a stable mesh.
9. Propagate import diagnostics through MeshLoader, Viewer, TypeScript, and UI. Stage referenced files
   with relative paths before opening composite manifests; add the Vue manager and typed API.
10. Integrate definitions, styles, source/field bindings, and migration into ProjectStore/SavedViewStore.
    Native definition JSON does not serialize mesh sources, active fields, deformation, or styles.
11. Meet the million-cell 15 fps target for crossing-cell/rotating cuts, deformation, fields, primitive
    construction, and actual rendering together. Current geometry-only timings do not establish this.
12. Rebuild WASM and run browser acceptance after an Emscripten/OCCT WASM SDK is available.

Structured VTK, parallel XML, VTKHDF, and Abaqus ODB remain outside the first format scope.

## Prerequisites

| Area | Available | Still Missing |
| --- | --- | --- |
| Geometry | RWCae topology, MeshElementCutter, BVH, optional VTK cell evaluation and OCCT cuts | General internal walls, conforming curved geometry and convergence |
| Fields | Scalar/vector/tensor properties, node provenance, deformation | Shell/beam physical mappings and full time lifecycle |
| Rendering | Native OCCT AIS, contours, three cut portions, plane guides/manipulators | Result picking, OR boundary merging, browser bridge |
| Import | RWVtk readers, optional original VTK topology and diagnostics | Portable general cells, complete time-topology validation, UI reports |
| Integration | DRAW commands, Tk plane manager, numerical/pixel/pointer scripts | Vue controls, staging, project persistence, WASM rebuild |

MCP exploration was verified against source where the C++/Tcl index reported partial coverage.

## Execution Queue

See [cut-view-execution.md](cut-view-execution.md) for the dependency-ordered tasks and their
acceptance gates. R00, G01 and native M01 are verified; G02/G03 have a general-cell milestone
but remain partial. Current coverage includes the supplied VTK real-data cases and
the 48-concrete-cell native/DRAW matrix. N01 and N02's numerical foundation are verified.
N02 includes supported analytic source boundaries and expanded reference fixtures.
N03 region/threshold recovery and original physical mapping are implemented; physical
trace policies, tensor invariants, manager controls and multi-cut boundary/cap union
remain open. N04-N09 remain queued;
web/WASM work is excluded.
First-order isosurfaces currently rebuild scalar-cut geometry when the threshold changes; they
have not been accepted against the full-frame million-cell target. Second-order meshes are
rejected for isosurfaces. Non-planar physical integration needs local-normal mapping and is rejected.

## Performance Evidence

Release, one million regular hexahedra, nodal field, 20 updates:

| Scenario | p50 | p95 |
| --- | --- | --- |
| Same topology interval, section only | 0.86 ms | 1.17 ms |
| Same topology interval, all three portions | 5.27 ms | 6.39 ms |
| Across cell layers, section only | 211.98 ms | 214.64 ms |

These measure geometry and fields only, excluding primitive construction/rendering. The cache is
limited to first-order fixed cell types, unchanged plane normals and node-side classifications,
without active deformation or near-node merge changes. Other cases rebuild. Cold section generation
was about 321 ms; the full-frame p95 <=67 ms acceptance remains open.

See `test/README.md` for commands, Tcl coverage, screenshots, and native API usage.

## Baseline

The existing native RWVtk, ViewCaeGeom, and Viewer targets build successfully.
The current native build uses OCCT_private/install. The pre-existing root
CMakeLists.txt changes are retained. The root AGENTS.md is retained unchanged.
