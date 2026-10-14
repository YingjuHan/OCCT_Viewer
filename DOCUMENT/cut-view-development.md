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

VTK file parsing, concrete-cell topology, shape functions, inverse mapping and
cutting now use the repository-owned implementation. No external VTK headers,
libraries or runtime are required. See [the implementation audit](vtk-independent-audit.md).
Migration verification: native Viewer/DRAW/Tk **72/72**, headless **25/25**;
55 complete-build and 41 headless binary artifacts have no VTK DLL imports.
Earlier ON/OFF and performance numbers below retain their original milestone scope.

Current execution is native only: Web/Vue/TypeScript and WASM tasks are excluded,
not completed. Project/session persistence below maps to the native session API.

N01–N09 are closed at their documented native scopes. N05–N09 evidence is recorded in the
[Viewer audit](cut-view-n05-audit.md), [time/slicing audit](cut-view-n06-audit.md),
[physical analysis audit](cut-view-n07-audit.md), [performance audit](cut-view-n08-audit.md)
and [full-session audit](cut-view-n09-audit.md).
Final VTK ON 71/71, OFF 30/30, headless 17/17 and Release affected 6/6
each pass three consecutive runs. Actual controls: main 48, tools 40, time 20,
physics 31, cache 7 and session 6. See [N09](cut-view-n09-audit.md) for the final logs.
The current stage table is [N05–N09](cut-view-n05-n09.md); earlier test counts
below describe their original milestones.
Curved-family geometry/FE
references, analytic precision fallback, high-order field isosurfaces, tensor/trace
policies and actual presentation OR are included. N03 is verified at
[its bounded native closure scope](cut-view-n03-audit.md).
N02 is verified at its documented bounded scope;
see [its completion queue](cut-view-n02-plan.md) and [the current handoff](cut-view-native-handoff.md)
for SDK paths, logs, missing historical external fixtures and remaining work.

## Verified Native Work

- [x] Native tests and DRAW batch/visual Tcl regressions under `test/`.
- [x] N04.2 ordinary scalar Viewer points, Lines/polyline and general VTK boundaries,
  original CellData/FE mapping and ordinary `scvreadmesh` rendered field switching.
  Full ON 51/51 and OFF 16/16 pass; scoped import tests pass three times.
  See [the Viewer contract](cut-view-viewer.md) and [the N04 queue](cut-view-n04-plan.md)
  for each acceptance gate and its original test counts.
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
- [x] Interactive DRAW/Tk control acceptance: 47/47 widget inventory, real menus/file dialogs,
  all buttons, editable numeric fields, errors, cancellation and empty-manager states.
- [x] Pre-release plane-drag checks for completed presentation synchronization and changing pixels,
  all eight masks, constant/projected CellData and deformed meshes beyond original bounds.
- [x] Native defaults/model replacement/close lifecycle, including active-drag cancellation.
- [x] Repository-owned VTK-cell backend: 48 concrete cell types, geometric planes/spheres/cylinders,
  original shape functions, rational/anisotropic metadata and arbitrary-length connectivity.
  See [the support matrix](vtk-cell-support.md) for tested scope and geometry limitations.
- [x] N01 shared source epochs, lazy OCCT shapes/BVH, planar adjacency and held-drag cache checks.
- [x] N02 numerical foundation: configurable tolerance/refinement/primitive limits,
  conforming curved reference interfaces and numerical convergence; 28/28 cases pass three times.
- [x] N02 analytic source increment: OCCT polynomial/rational Bezier boundaries, explicit
  numerical fallback, exact bowed-tetra/warped-hex/curve/polygon references and held-pointer
  clouds; 31/31 native/DRAW cases pass three times.
- [x] N02.3 original-FE numerical field isosurfaces: 37 generic chart types, six native
  second-order families, enclosed/nonmonotone crossings, independent field tolerances,
  provenance, deformation, held-threshold pixels/versions, abort and definition migration.
  See [the supported scope and limits](cut-view-isosurfaces.md).
- [x] N02.4 bounded family/interface/precision closure audit and repeated full validation:
  VTK ON 38/38 and OFF 8/8, every case three times.
- [x] N02 expanded independent references: curved wedges `26/31/32/73/80`, rational
  Bezier wedge `80`, and pyramids `27/37`, with native and DRAW geometry/FE checks.
- [x] N02.1/N02.2 remaining curve/surface/tetra and mixed-order hex references,
  original nonlinear FE values, degree-six/anisotropic charts, weight rescaling,
  numerical convergence and unsupported precision fallback; native/DRAW/visual acceptance.
- [x] N03 display recovery foundation: region IDs, averaging thresholds, elemental-per-node
  discontinuities, vector magnitude before/after averaging and independent original physical fields.
- [x] N03.1 manager recovery threshold/order/region controls and atomic JSON v5 options,
  with exact signed 64-bit region IDs and legacy migration.
- [x] N03.2 symmetric tensor Mises/pressure/principal/Tresca measures, explicit six/nine
  layouts and shear conventions, recovery order and original physical evaluation.
  See [the tensor display contract](cut-view-tensors.md).
- [x] N03.3a VTK prepared-source reciprocal volume-face adjacency and two original-FE
  traces, including curved/deformed source checks and independent element-node jumps.
  See [the verified trace contract](cut-view-traces.md).
- [x] N03.3b generated face/line/point dual-source FE binding and native fast-slice
  parity; trusted primitive ownership, disconnected topology and cached movement verified.
- [x] N03.3 independent physical/display interface traces, JSON v7 and 47/47 controls;
  ON 45/45 and OFF 12/12 each repeated three times, within the documented support contract.
- [x] N03.4 actual presentation OR boundary/cap union, source/FE provenance,
  64 mask pairs, touching/overlapping/contained regions and transparent clouds.
  See [the union contract](cut-view-union.md). Full session persistence is validated separately in N09.
- [x] N03.5 full repeated ON/OFF closure audit, repaired curved On-only caps and
  internal compsolid faces, inspected UI/cloud images and documented support limits.
- [x] N04.1 overall VTK import reports and backend attempts, successful fallback diagnostics,
  composite/series warnings, rejected pipeline outputs and failed native Viewer/DRAW replacement preservation.
  See [the import contract](cut-view-import.md).
- [x] N04.3–N04.7 format/diagnostic contract, complete independent moving/remeshed
  series snapshots, malformed/binary/compressed matrix, pre-allocation resource
  budgets and collision-free local reference staging with contextual reports.
  Ordinary Viewer/DRAW failed staging preserves old objects and pixels.
- [x] N04.8 final full repeated ON/OFF audit, static fallback step compatibility
  repair, ordinary Viewer images and kernel peak-memory gate; current support
  boundaries and handoff synchronized.

- [x] N05 datum/local editing and axes, per-cut/portion styles, contour/legend,
  original-FE picking/probes, vector/orientation plots, annotations, CAD sections,
  global clipping and ordinary native Viewer lifecycle/event/error APIs.
- [x] N06 coordinated geometry/field/deformation steps, explicit material frames,
  remesh mappings, playback and regular/range/rotational/path slicing.
- [x] N07 explicit physical groups/units/frames, shell/composite/beam section
  adapters, local-normal volume analysis, resultant overlays, interval reports,
  CSV export and mapped crack-distance/optimization-density cuts.
- [x] N09 native v3 complete sessions with immutable source bundles/digests,
  CAD/camera and all Viewer/timeline/analysis settings, legacy migration,
  relocation, shared-budget validation and failed restore/save preservation.
- [x] N08 shared source/result reservation accounting, local policies, LRU,
  externally retained leases, cancellation/generations and full-frame timing.
- [x] N08 million-cell crossing/rotating/fixed-deformation plane workload:
  three complete measured runs pass >=15 FPS and p95 <=67 ms; repeated actual
  display open/edit/close stays below the documented private-memory growth gate.

## Abaqus Feature Comparison

Comparison baseline: Abaqus/CAE 2025 Visualization. The DRAW/Tk panel is a native development
interface, not the production Vue manager. See the official [view-cut overview](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-c-cutunderstand.htm),
[display controls](https://abaqus.uclouvain.be/English/SIMACAECAERefMap/simacae-t-cuthlpdisplay.htm), and
[slicing options](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-t-cuthlpslicing.htm).

| Capability | Current Native Status | Remaining Difference |
| --- | --- | --- |
| Manager list, select/show independently, create/edit/copy/rename/delete | DRAW/Tk implemented and tested | Production Viewer/TypeScript/Vue integration |
| Default X/Y/Z planes | Native lifecycle defaults; new model remains uncut | Browser lifecycle excluded |
| Below/on/above combinations, multiple cuts | Actual OR, exposed caps, all masks, transparent clouds and per-cut styles | Million-cell performance acceptance covers one On-only plane |
| Translate/rotate with numeric values and sliders | Local/global datums, reference axes, plane/curved/iso controls | Production browser controls excluded |
| Mouse positioning | Plane guides/manipulators, cancellation, AIS result picking and FE probes | Browser pointer integration excluded |
| Real mesh and nodal-result sections during motion | Original FE, bounded curved references, 48-cell matrix and measured native plane frames | No arbitrary-order exact or general high-order frame-time promise |
| Element-result sections | Original constants/recovery/tensors, independent traces and explicit section adapters | Unsupported physical sections reject explicitly |
| Plane/cylinder/sphere/isosurface shapes | Native and VTK concrete cells; bounded high-order numerical FE isosurfaces | Unmatched interfaces remain distinct; no arbitrary-order exact geometry |
| Per-portion/per-cut plot options | Shared options, overrides, opacity/edges, contour range and legend | Production browser dialogs excluded |
| Deformed plots and following deformation | Intersections and explicitly anchored reference/updated material frames | Remeshed anchors require explicit mappings |
| Field time steps and animation | Atomic geometry/field/deformation steps, topology validation and playback | Missing remesh bindings reject; correspondence is not inferred |
| Result caching | Local logical budgets, shared accounting, leases, LRU, cancellation and generations | Logical reservations do not bound exact process/GPU memory |
| Free-body forces/moments and heat flow | Explicit groups/units/frames, shell/composite/beam mappings, overlays and CSV | Nonplanar shells, composite heat and beam heat excluded |
| Regular/rotational/path slicing | Regular/range/rotation/piecewise-linear path frames and interval reports | Bounded at 10000 slices |
| Common options and cut-specific options | Native manager/tools/time/physics/cache panels and overrides | Browser manager excluded |
| Session/project persistence | Native v3 complete source-bundle sessions, migration and staged restoration | Native local-filesystem contract; JSON and assets move together |
| XFEM and optimization automatic cuts | Explicit distance/density field, sign, threshold, range and empty/full states | No ODB parser or inferred field semantics |
| CAD/global clipping synchronization | Native independent or synchronized CAD section/global clip, lifecycle ownership | CAD session sources must be explicit BREP files |
| VTK-based source data | Ordinary native Viewer points/lines/general cells, remeshed snapshots, import limits and diagnostics | Structured/parallel XML/VTKHDF/ODB and browser integration excluded |
| Browser/WASM manager | Not implemented | SDK rebuild, typed API, UI, pointer and performance acceptance |

Pointer tests are in `test/cae/cut_view_interaction.tcl`: they pass events through
`ViewerTest_EventManager`/`AIS_ViewController`, verify current plane equations and interpolated
values before release, and check rendered pixels. Small-fixture interactivity is not evidence
that million-cell rendering meets 15 fps.

## Remaining Scope

N01/N02's bounded source/geometry/FE and high-order isosurface gates are verified.
N03 recovery/tensor/trace/actual OR gates and full closure are verified. N04 import
items and final audit are verified in [the closure report](cut-view-n04-audit.md). Mismatching
interfaces remain distinct, arbitrary-order exact geometry is not promised, and
N08 establishes the full-frame target for the measured native plane workload below.
Native N01–N09 implementation and acceptance are complete at the documented support contracts.
The full-session contract is [N09](cut-view-n09-audit.md).
Web/Vue/TypeScript/WASM rebuild and browser acceptance remain excluded for a later scope.
Unsupported physical mappings and broader performance workloads are listed in the audits.

Structured VTK, parallel XML, VTKHDF, and Abaqus ODB remain outside the first format scope.

## Prerequisites

| Area | Available | Still Missing |
| --- | --- | --- |
| Geometry | RWCae topology, BVH, original VTK FE charts, bounded curved/analytic geometry and numerical convergence | Unmatched general interfaces and arbitrary-order exact geometry |
| Fields | Scalar/vector/tensor provenance, recovery/physical separation, explicit section mappings and coordinated time/deformation | Unsupported section semantics and inferred remesh correspondence |
| Rendering | Native AIS, OR/contours, styles/legend, result picking, vectors/overlays, guides and measured plane frames | General high-order/OR/isosurface million-cell performance |
| Import | Legacy/XML/general topology, independent snapshots, limits, staging/reports and playback | Structured/parallel formats and portable general-cell backend |
| Integration | Native Viewer lifecycle/CAD coordination, DRAW/Tk and numerical/pixel/pointer scripts | Native v3 full sessions with source identities; Vue/WASM excluded |

MCP exploration was verified against source where the C++/Tcl index reported partial coverage.

## Execution Queue

See [cut-view-execution.md](cut-view-execution.md) for the dependency-ordered tasks and their
acceptance gates. R00, G01–G05 and native M01–M07 are verified at their documented
contracts. Current coverage includes the supplied VTK real-data cases and
the 48-concrete-cell native/DRAW matrix. N01 and N02's numerical foundation are verified.
N02 includes supported analytic source boundaries and expanded reference fixtures.
N02 is verified at the [bounded closure scope](cut-view-n02-audit.md).
The [N03 recovery, traces and union queue](cut-view-n03-plan.md) is closed.
N03 region/threshold recovery, symmetric tensor measures, manager controls and
original physical mapping and independent trace policies are verified.
Actual presentation OR has passed numerical, field/dimension and transparent
acceptance and the full [N03 closure audit](cut-view-n03-audit.md).
N04.1–N04.8 import/ordinary Viewer work is verified in
[its acceptance queue](cut-view-n04-plan.md) and [closure audit](cut-view-n04-audit.md).
N05–N09 are verified at their documented native scopes; web/WASM work is excluded.
First-order isosurfaces currently rebuild scalar-cut geometry when the threshold changes; they
have not been accepted against the full-frame million-cell target. Supported high-order
VTK chart cells use the bounded numerical isosurface contract recorded in N02.
Volume sphere/cylinder integration uses analytic local normals. Isosurfaces require
explicit element normals. Nonplanar shell adapters and beam/composite heat are rejected.

## Performance Evidence

The single-plane GPU clip / CPU slice optimization and self-contained large
VTK/VTU workloads are documented in [the GPU plane audit](cut-view-gpu-plane.md).
The following N08 numbers are historical baseline measurements, predating the
resident-body backend; use the new audit for current measurements and fallback limits.
The resident-plane display, gap fixtures and actual VBO upload checks pass.
Its current performance gate is partial: native million-cell fixed-deformation
p95 is not consistently <=67 ms, and multi-million-cell rotation/deformation
still needs CPU section/trace optimization. These limits are separate from the
historical N08 measurements below.

Release, one million native first-order regular hexahedra, one On-only plane,
nodal field, 60 frames per scenario in each of three complete measured processes:

| Scenario | FPS range across three runs | p95 range |
| --- | --- | --- |
| Crossing cell layers | 54.65–56.54 | 38.60–39.25 ms |
| Rotating plane | 25.33–25.64 | 43.46–45.44 ms |
| Fixed-deformation translation | 26.14–27.33 | 45.77–54.09 ms |

Each measured frame includes geometry, original-FE field mapping, contour primitives,
V3d redraw and OpenGL completion. All runs pass >=15 FPS and p95 <=67 ms.
Cold setup is reported separately. Intel Iris Xe OpenGL 4.6; this repository is
Release while its OCCT SDK remains Debug. Background C++ language-service indexing
was paused for the final measurements and restored afterward; unisolated failed
runs are retained. This result is specific to the measured workload and machine.
Sixty actual open/edit/close cycles (300 frames) grew peak private bytes by 4.41 MiB,
below the 32 MiB gate. See [N08](cut-view-n08-audit.md) for logs and limits.

See `test/README.md` for commands, Tcl coverage, screenshots, and native API usage.

## Baseline

The existing native RWVtk, ViewCaeGeom, and Viewer targets build successfully.
The current native build uses OCCT_private/install. The pre-existing root
CMakeLists.txt changes are retained. The root AGENTS.md is retained unchanged.
