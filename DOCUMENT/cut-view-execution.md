# Cut View Execution Roadmap

This is the execution queue for the complete TODO list in
[cut-view-development.md](cut-view-development.md). Tasks run in dependency order;
an implementation is complete only after its listed acceptance checks pass.
Native acceptance uses the existing `VIEWDRAW` plugin and Tcl scripts under `test/`.
Do not add a separate `CUTVIEW` plugin or modify the existing `AGENTS.md`.

The Windows continuation now has VTK 9.5.2/MinGW SDK support, expanded native/DRAW
curve/surface/tetra/hex/wedge/pyramid independent references, analytic precision
fallback repair, and N03 recovery/tensor/trace/OR implementation. N02/G03 and
N03 are verified at their documented bounded scopes.
Current environment, test counts and remaining work are recorded
in [cut-view-native-handoff.md](cut-view-native-handoff.md); older milestone counts
below describe their original validation runs.

N04.1–N04.8 are verified at [the closure contract](cut-view-n04-audit.md).
Final VTK ON **58/58**, OFF **22/22**, every case three times (174 / 66 executions; 547.47 / 41.50 seconds). DRAW/Viewer-disabled import/reference tests **2/2** each pass three times (0.52 seconds).
Current evidence replaces older milestone counts below. Two historical external fixtures remain absent from the
current official data set; their old counts are not reused.

## Baseline

- Existing plane manager, pointer dragging, eight portion masks, VTK import,
  linear interpolation, constant/projected CellData and volume integration are retained.
- Baseline: 5/5 native/DRAW tests pass, including rendered pixels and pointer events.
- Production Vue/WASM integration and full Abaqus parity are not complete.
- Historical geometry-only crossing-layer p95 was about 214 ms. N08 now verifies
  the complete measured native-hex plane workload at >=15 FPS / p95 <=67 ms;
  see [its audit](cut-view-n08-audit.md) for machine, background load and limits.

## Ordered Queue

Current execution scope is native only, per the user's request. The implementation
design is [cut-view-native-design.md](cut-view-native-design.md). B04/W01/W02 and
the TypeScript portion of B03 are excluded from this track; S01 is mapped to
native session persistence. Original web tasks remain recorded, not completed.

Status: **active**, **queued**, **partial**, **environment pending**, or **verified**.
The plane-interaction repair gate R00 takes priority over new cut shapes.

N02 closure passed: [queue and evidence](cut-view-n02-plan.md),
[bounded audit](cut-view-n02-audit.md). The [N03 queue](cut-view-n03-plan.md) is now
verified at [the closure scope](cut-view-n03-audit.md); all earlier increments
remain in the regression suite. [N04](cut-view-n04-plan.md) is closed at its
documented native contract; N05 is closed at [its native audit](cut-view-n05-audit.md); N06 is closed at [its native audit](cut-view-n06-audit.md); N07 is closed at [its native audit](cut-view-n07-audit.md); N08 is closed at [its native audit](cut-view-n08-audit.md); N09 is in final validation.

| ID | Task | Depends On | Acceptance | Status |
| --- | --- | --- | --- | --- |
| R00 | Plane held-drag recomputation, every existing control, interactive DRAW/Tk crashes and invalid-input handling | Baseline | Interactive (-i) menus/dialogs, current 47/47 controls; pre-release geometry/field/pixel assertions; all regressions | verified |
| G01 | Field isosurface: fixed field/component/step binding, below/on/above, surface lines, live threshold, definition migration | Baseline | Native numerical tests, all masks, independent contour changes, DRAW pixels and manager controls | verified |
| G02 | Cylinder and sphere: position/axis/radius, enclosed intersections, three portions | G01 | Analytic volume/area checks, same-sign vertex cases, DRAW interaction | verified at bounded native geometry scope; N08 performance applies only to its measured plane workload |
| G03 | Exact second-order cell geometry and FE result interpolation | G02 | Curved-cell reference solutions and convergence tests; explicit unsupported-cell diagnostics | verified at bounded N02 scope: independent family references, analytic/precision limits and original-FE higher-order isosurfaces; exact arbitrary-order intersections not claimed |
| G04 | Cell recovery/averaging thresholds, region boundaries and discontinuity policies | G03 | Constant and recovered results verified across material boundaries; physical fields unchanged | verified at N03 contract: recovery/tensor/trace policies, original physical mapping, JSON v7 and 47/47 controls; full sessions remain N09 |
| G05 | Multi-cut OR boundary merging, deduplication and shared opacity | G02 | Overlapping/touching cuts, no duplicate faces or darkened overlap, all masks | verified at N03.4 OR contract; no general million-cell OR performance claim |
| M01 | Native model lifecycle, defaults X/Y/Z, selection independent of activation | R00 | Open/replace/close model tests and deterministic defaults | verified |
| M02 | Datum positioning, reference axes and numeric editing for all shapes | G02, M01 | Local/global transforms, numeric and pointer edits agree | verified at N05 |
| M03 | Per-cut/per-portion styles and common options with overrides | M01, G05 | Color/opacity/edges/visibility persistence and DRAW images | verified at N05 |
| M04 | Contour options and legends with consistent source/section ranges | M03 | Scalar/vector/tensor ranges, empty/constant results, screenshots | verified at N05 |
| M05 | Result node/element picking and section probes | G03, M04 | Source provenance, interpolated values and pointer selection | verified at N05 |
| M06 | Vector/orientation plots and annotations | M04, M05 | Component/coordinate-system correctness and image checks | verified at N05 |
| M07 | CAD sections and synchronization with global clipping | M02, M03 | CAD/mesh toggles, independent cuts, viewport lifecycle | verified at N05 |
| D01 | Deformed guide/range bounds and material-following reference frames | G03, M02 | Guides and intersections follow deformation consistently | verified at N06: explicit reference/updated material anchors |
| V01 | VTK Lines/polyline storage, import and rendering | G01 | Legacy/XML line fixtures, CellData mapping and DRAW pixels | verified at N04.2 native contract: ordinary scalar Viewer and Cut View; VTK-disabled general cells reject explicitly; browser excluded |
| V02 | Remaining supported Legacy sections and explicit scope diagnostics | V01 | Format-specific fixtures and unsupported-section reports | verified at N04.3 format contract |
| V03 | Changing geometry/topology series and time validation | V02 | Moving and remeshed fixtures, mismatched fields rejected | verified at N04.4/.7: complete independent snapshots, strict time/schema/tuple validation; playback remains N06 |
| V04 | Malformed/binary/compressed/multi-offset import regression coverage | V03 | Truncation, endian/offset/type errors, safe rejection | verified at N04.5 safety contract |
| V05 | Bounded-memory appended decoding and configurable resource limits | V04 | Oversized payload rejection and measured peak memory | verified at N04.6/.7 logical import-budget contract; full-frame/session budgets remain N08 |
| D02 | Coordinated geometry/result/deformation time steps and animation | D01, V03 | Step switching, remeshing, pause/restart and no stale contours | verified at N06: coordinated snapshots, explicit remesh bindings and playback |
| D03 | Regular/range/rotational/path slicing | G02, M02, D02 | Slice placement/count, path frames and interval results | verified at N06: regular/range/rotation/piecewise-linear path and interval bounds |
| P01 | Cache policies, memory budgets and eviction | D02, D03 | Cache correctness after edits; memory bounded across repeated motion | verified at N08: logical reservations, local policies, shared leases and LRU |
| I01 | Integration groups, summation modes, units and coordinate systems | G04, M02 | Analytic forces/moments/heat flow and explicit unit maps | verified at N07: explicit groups, SI factors, frames and sums |
| I02 | Shell/beam/composite section mappings, thickness and section semantics | I01 | Physical reference solutions; unsupported mappings fail explicitly | verified at N07 supported section contract; unsupported mappings reject |
| I03 | Resultant overlays, distributed resultants and exportable reports | I02, M06, D03 | Arrow directions, group sums and interval report checks | verified at N07: distributed totals, overlays, interval reports and CSV |
| A01 | Automatic crack-distance cuts from explicitly mapped fields | G01, G03 | Sign/threshold semantics, provenance and special display states | verified at N07: explicit distance/sign/threshold mapping |
| A02 | Automatic optimization-density cuts | G01, G04 | Mapping/range validation and reproducible thresholds | verified at N07: explicit density/range/threshold mapping |
| B01 | Import diagnostics through MeshLoader and Viewer | V04 | Structured warnings/errors reach caller without data loss | verified at N04.1/.3/.7: contextual reports, original/staged sources and old live-model preservation |
| B02 | Relative-file staging for composite manifests and series | V03, B01 | Nested relative paths, missing files and duplicate names | verified at N04.7 local-filesystem contract |
| B03 | Native Viewer cut API; TypeScript excluded | M07, D02, B02 | Lifecycle, field bindings, drag events and error contracts | verified at N05/N06 native Viewer facade; TypeScript excluded |
| B04 | Production Vue Cut View Manager and all shape controls | B03, M03, M04, M05 | Full user workflows, desktop/mobile layout and real pointer tests | excluded: web |
| S01 | Native session persistence and schema migration | B03, M03, D02 | Source/fields/styles/deformation round trip; atomic invalid-load rejection | active: N09 final validation/documentation |
| P02 | Full-frame million-cell optimization and instrumentation | P01, B03 | Crossing-cell/rotating/deformed cuts, fields, primitives and rendering: >=15 FPS and p95 <=67 ms | verified at N08 measured million native-hex On-only plane workload, three full-frame runs |
| P03 | Long-session memory, cancellation and lifecycle acceptance | S01, P02 | Repeated open/close/edit/drag; no growth, stale handles or rollback errors | verified at N08: cancellation/leases/200 edits and 60 actual display cycles, 4.41 MiB growth |
| W01 | OCCT/Emscripten WASM SDK build and dependency packaging | B03 | Native-compatible API and successful WASM build | excluded: WASM |
| W02 | Browser functional, pointer, memory and performance acceptance | B04, S01, P03, W01 | Playwright interactions/screenshots, nonblank canvas and measured frame budget | excluded: web/WASM |

## Per-Task Completion Gate

1. Verify indexed structure with MCP; read source where index coverage is partial.
2. Implement the smallest complete behavior, preserving existing worktree changes.
3. Add focused numerical tests and a runnable DRAW Tcl regression in `test/`.
4. Build affected native targets, run the full CTest suite and inspect images when relevant.
5. Update this queue with evidence and remaining limitations before advancing.

## Completed G01

The milestone sections below preserve historical validation counts and the next
steps at that time. Current stage status is in the ordered queue above and the
latest closure audits.

- Added native plane/isosurface definitions and independent cut-field bindings.
- Reused `MeshElementCutter` for both volume portions and surface intersection lines.
- Added DRAW `isoadd`, `isomove`, `isovalue`; the existing Tk manager now supports
  creation, fixed field/component/step selection and live threshold controls.
- Added JSON v2 definitions with atomic v1 plane migration and field resolution.
- Acceptance: **7/7 CTest tests pass**, including the original plane pointer tests
  and new `draw_cut_view_iso` / `draw_cut_view_iso_visual` regressions.
- Native tests additionally cover raw vector components/magnitude projection,
  fixed timesteps, deformation, coincident faces, ambiguous bindings and explicit
  second-order rejection. Image evidence is in `build/test/native/draw-iso-images/`.
- No full-frame performance claim: scalar geometry is rebuilt on threshold changes;
  memory policy, exact higher-order interpolation and non-planar integration remain queued.

## Plane Repair and Lifecycle Evidence

- Current acceptance: **11/11 CTest tests pass**, including interactive `draw_cut_view_controls`;
  `ctest --repeat until-fail:3` also passes all three consecutive runs of every case.
- The control test inventories widgets and requires **39/39** coverage. It opens actual
  combobox popdowns and file dialogs, tests Save/Load/cancel/invalid files, numeric editing,
  every button/checkbutton, three rotation axes, held slider movement and empty-manager states.
- Pointer tests assert geometry and field values plus completed display synchronization
  **before mouse release**, compare changing pixel checksums and test all eight portion masks.
  Surface lines, constant/projected CellData, rotation rings, normal arrows and cancellation
  are covered. These are geometric plane sections, not isosurface substitutes.
- Fixed Tk/DRAW `grid` collision/null access, blocking X11 events, stale repeated-drag detection,
  incorrect tree hit-testing, numeric-entry/slider variable conflicts, stale focused entries,
  disappearing input errors, partial invalid field changes and floating-point JSON drift.
- M01 adds inactive default XYZ planes, transactional model replacement, field/time rebuilding,
  timer/handle cleanup and close/replace cancellation during an active drag.
- Deformed nodal bounds now update plane guides/ranges/fit; out-of-original-bounds pointer
  movement and interpolation are tested. Material-following frames are not implemented.
- OCCT SDK fixes were built/installed locally; the reproducible source patch is
  [occt-cut-view-sdk.patch](occt-cut-view-sdk.patch). Existing unrelated OCCT edits are retained.

This proves the listed native behaviors on fixtures, not absence of all bugs or completion
of the remaining production, geometry, VTK, integration and performance tasks.

## G02 Simplex Milestone

- Added analytic sphere/infinite-cylinder definitions, center/axis/radius editing, copying,
  and JSON v3 persistence with v1/v2 migration. Restored non-plane cuts can now be copied
  even though they have no plane coefficients.
- OCCT Common/Cut/Section operations find intersections enclosed inside tetrahedra or
  triangles even when every original vertex is outside. Cylinder construction caps are
  excluded. Tessellated nodes carry barycentric source weights and element provenance;
  nodal, constant CellData and projected CellData contours are verified.
- Three portions and all eight masks are available. Side meshes are boundary triangles,
  not replacement volume cells; `MeshCutResult::Volumes/Areas` and DRAW `eval` use OCCT
  analytic measures. Shared interior tetrahedron faces are removed from side boundaries.
- Added center entries, radius-slider held motion, curved wireframe guides and AIS
  manipulators. Actual pointer tests require synchronization before lazy evaluation,
  changing held-drag pixels, translation/rotation, abort and close/replace cancellation.
- Added `quadric_cut`, `draw_cut_view_quadric`, and `draw_cut_view_quadric_visual` acceptance.
  Numerical cases include full/partial/outside/tangent/containing cuts, oblique axes,
  adjacent cells, deformation, invalid cells and recovery. All 11 tests pass three times.
  Images are in `build/test/native/draw-quadric/`; demo:
  `source /home/hyj/Desktop/workspace/OCCT_Viewer/test/cae/cut_view_quadric_demo.tcl`.
- At the simplex milestone G02 was **partial**: curved cutting rejected quads, pyramids,
  prisms, hexahedra, polygons/polyhedra and second-order cells. The remaining first-order
  families need validated geometry and FE interpolation; higher-order work belongs to G03.
  The general VTK milestone below supersedes that cell-family restriction.
  OCCT operations still rebuild per portion/cell, without reusable curved-cut BVH pruning or
  a large-mesh frame-budget claim. WASM/browser acceptance remains pending.

## General VTK Milestone

- Optional `OCCTV_USE_VTK=ON` adds original VTK-cell storage, native import and a common
  plane/sphere/cylinder backend. First-order native fast plane paths are retained.
  Native VTK fallback preserves types, unbounded-by-8-bit connectivity, polyhedron faces,
  HigherOrderDegrees, RationalWeights and numeric PointData/CellData.
- All 48 concrete VTK 9.5.2 types have canonical native and DRAW matrix coverage.
  Additional native tests cover anisotropic quad/wedge degrees, non-unit weights for all
  six Bezier families, 343-node hexahedra, quadratic fields and generic-volume integration.
  The precise matrix and remaining limits are in [vtk-cell-support.md](vtk-cell-support.md).
- Fixed empty face/plane intersections by using section operations for surface/line cells;
  added convex-point-set boundaries, consistently oriented triangular polyhedron MVC cages,
  stable cage-edge interpolation, numerical inverse maps for anisotropic/rational cells,
  and colored 0D points. Lines render for every portion and uncut sources.
- Box rejection operates on the tessellated source cell, avoiding unsafe nodal-sign rejection
  of enclosed cuts. Sewn solids are validated. Invalid connectivity, open polyhedra,
  inconsistent high-order cardinalities and invalid rational weights fail explicitly.
- Tests under `test/cae/cut_view_vtk_cells.tcl` cover all masks, affine point and constant
  CellData, source views, rendered points/lines and held-pointer recomputation on six
  representative volume families. They check display revisions before lazy evaluation,
  nonempty geometry, FE values and changing pixels before release.
- The supplied VTK ExternalData directory is registered with `OCCTV_VTK_TEST_DATA`.
  Six valid real fixtures have nonempty plane/sphere/cylinder numerical checks; two malformed
  fixtures test safe rejection. External fixtures are not modified.
- Acceptance: **22/22 CTest cases pass three consecutive runs per case**, including all
  48 concrete-cell native/DRAW checks, actual held-pointer images and the original 32/32
  control inventory. The VTK-disabled Release build's three native cases also pass three times.
  Screenshots are in `build/test/native/draw-vtk-cells/`.
- After box rejection, `hexa.vtk` (10,648 cells) completes nonempty plane/sphere/cylinder
  numerical probes within the 90-second diagnostic budget. This is not an interactive
  full-frame benchmark. The repeatable interactive demo is
  `source /home/hyj/Desktop/workspace/OCCT_Viewer/test/cae/cut_view_vtk_demo.tcl`.

This was the general-cell implementation milestone, not closure of G02/G03. Geometry has an
explicit fixed tessellation tolerance, not exact arbitrary-order intersection. N01 below
adds reusable broad-phase caches and conservative planar adjacency. Arbitrary curved-face
conforming refinement, composite/time-series fallback and full-frame performance remain
open; portable/WASM parity is excluded from the current native track.

## N01 Native Cache and Adjacency Milestone

- `MeshVtkCutSource` shares prepared original-cell evaluators, tessellated/deformed
  bounds, adjacency and an OCCT BVH across cuts, portions and source-only views.
  OCCT shapes are built lazily for selected candidates, not every cell on every motion.
- Full original face node IDs, edge connectivity and compatible types identify
  conforming planar, straight-sided interfaces. Exactly two opposite-side owners
  are required. OCCT boolean history filters their side-boundary fragments without
  changing complete-solid volume measures. Disconnected/ambiguous faces remain visible.
- A coincident shared-face section is emitted once with deterministic lowest-element
  ownership. Explicit physical trace policies and multi-cut cap deduplication await N03.
- Source epochs survive cut motion and field-only edits, and reset after node,
  topology or deformation edits and clear. The cache is single-threaded and retains
  prepared evaluators; this is not a hard memory budget or LRU implementation.
- `scvcutview model stats` reports preparation, shape/BVH builds, candidates,
  box tests, boolean work and filtered/ambiguous face counts without lazy evaluation.
- New native tests cover joined types 11/12/25/42/72, source/side measures, interface
  CellData, eight masks, cached-versus-fresh results and invalidation. A 24-cell chain
  requires one candidate shape for section-only evaluation. DRAW tests verify actual
  held-pointer interpolation, completed display revisions, cache reuse and changing
  pixels before release, plus abort rollback.
- Acceptance: **25/25 CTest cases pass three consecutive runs per case**; the final
  repeated suite takes 285.67 seconds. The VTK-disabled Release suite passes its
  three native cases three times. Screenshots: `build/test/native/draw-cache/`.

N01 is verified at its planned foundation scope. G02 and P01 remain partial;
no million-cell full-frame/FPS claim follows from this acceptance.

## Historical Next Task after N01

Finish the N02 queue in [cut-view-n02-plan.md](cut-view-n02-plan.md), including
generic/high-order field isosurfaces and the final geometry/FE/precision audit.
Only then resume N03 recovery/physical trace policies and multi-cut OR boundary/cap merging.
Preserve all 48-cell, real-fixture and held-pointer gates. Do not substitute field isosurfaces
or corner-only interpolation for geometric sections.

Native design is complete at the stage/contract level, and N01 is implemented and
verified. N02's numerical foundation is verified; analytic second-order geometry
remains active in N02. N03-N09 remain scheduled.

## N02 Analytic Source Increment

- Added numerical/preferred/required geometry modes. The default preferred mode
  converts supported original VTK polynomial/rational maps through degree six into
  OCCT Bezier surfaces/curves; unsupported charts retain an explicitly identified
  numerical fallback. Required mode rejects unsupported conversion without source
  publication. Explicit native non-simplex requirements route to the general backend.
- Homogeneous conversion preserves rational weights. Triangular charts use a Duffy
  map with a degenerate rectangular edge. Independent parameter probes verify the
  map, positive-weight control hulls provide conservative source bounds, and the
  manager reuses analytic shapes during motion.
- Planar/straight boundaries keep canonical planar geometry for reliable coplanar
  OCCT operations. Planar quadratic polygons use exact curved wires and original
  quadratic edge weights at boundary intersections. Adaptive OCCT measure integration
  avoids confusing integration error with geometric refinement error.
- Fixed the VTK Bezier-wedge face-weight indexing defect in the adapter by copying
  each original face-node weight explicitly. Existing six-family rational acceptance
  is retained, not bypassed by replacing fields or silently dropping cell types.
- Native tests independently check exact bowed-tetra section/volume and quadratic
  fields, warped hex volumes/areas, rational reference integration, enclosed spheres,
  curves/polygons, unsupported-order rejection, fallback recovery and source reuse.
  DRAW tests compare numerical/analytic paths, scalar/CellData and held-pointer
  geometry/field/revision/pixel changes with abort rollback.
- Full acceptance: **31/31 native/DRAW cases pass three consecutive runs per case**
  (188.67 seconds), including the new analytic held-pointer images and 32/32 control
  inventory. VTK-disabled native/DRAW Release acceptance passes all six cases three
  times. G03 remains partial pending remaining curved-family reference evidence;
  N03-N09 were queued at that historical milestone; current status is recorded above.
  Images from that run: `build/test/native/draw-analytic/`.

## N02 Geometry Controls and Curved Refinement

- `MeshCutGeometryOptions` supplies absolute/relative geometric tolerance,
  face/line refinement ceilings and an explicit source primitive ceiling. Invalid
  or unchanged options preserve the source epoch; valid changes invalidate geometry.
  DRAW exposes `scvcutview model geometry [absolute relative faceLimit lineLimit primitives]`.
- The effective tolerance uses deformed nodal bounds. Statistics report effective
  tolerance, common face depth, prepared primitive count and estimated float-coordinate
  resolution separately; requesting tighter tessellation does not upgrade float storage.
- Corner-based dyadic grids with original-ID canonical quad diagonals avoid adaptive
  face T-junctions. Curved volume faces use a common epoch depth, then matching shared
  tessellations and opposite interior-side checks identify removable curved walls.
  Ambiguous/mismatching interfaces remain visible and counted. This synchronous reference
  deliberately prioritizes correctness; connected-component/local refinement optimization
  and memory policy belong to N08.
- New numerical acceptance checks analytic warped-hex section convergence, joined
  quadratic/triquadratic/Lagrange/rational-Bezier hex interfaces, curved lines, model
  scaling, float resolution, rejected options, failed refinement/budget publication
  and successful retries. DRAW checks numeric options plus held-pointer interpolation,
  cache reuse, completed presentation revisions and changing pixels before release.
- On the quadratic warped reference, section area errors are 0.00416665, 0.00104165
  and 0.000260401 for absolute tolerances 0.025, 0.007 and 0.002 respectively.
  This demonstrates numerical convergence, not exact arbitrary-order geometry.
- Regression found over-refined simplex sphere rendering and a coarse small-circle
  length mismatch. Source-relative tolerance now uses deformed model bounds in both
  backends, with a one-percent-radius faceting cap for analytic curved tools. Added
  native tighter-tolerance checks and entry tracing in the existing control inventory.
  Full acceptance is rerunning after these repairs, without extending its timeout.
- Final acceptance: **28/28 native/DRAW cases pass three consecutive runs per case**,
  including 32/32 existing controls and held-pointer cloud checks. The full repeated
  run takes 316.98 seconds. The VTK-disabled Release build now includes its DRAW
  plugin; **6/6 cases pass three times**, not just its three native executables.
  Images: `build/test/native/draw-geometry/`.
- This verifies numerical refinement/convergence and matching curved reference
  interfaces, not exact second-order source geometry. Next: OCCT analytic source
  surfaces for supported polynomial second-order faces, exact curved source measures
  and independent FE-map acceptance. General numerical fallback remains explicit.

## N02.3 Original FE Isosurfaces

The [numerical FE contract](cut-view-isosurfaces.md) documents supported chart
cells, independent field/model tolerances, sampling/storage limits, budget
failures, provenance and definition version 4. Native/DRAW/held-Tk acceptance
passed alongside first-order compatibility (5/5, 67.02 seconds); VTK-disabled
8/8 passed three times (6.24 seconds). N02.4 is the next gate.

## N02.4 Closure and N03 Start

The [audit](cut-view-n02-audit.md) closes the planned N02 gates. Full VTK ON
38/38 and OFF 8/8 passed every case three times, with 114 and 24 pass executions
respectively (610.23 / 6.24 seconds). N03.1 controls and recovery persistence are
next; tensor measures, physical interface traces and general presentation OR
union/cap ownership remain separate acceptance items.

## N03.1 Recovery Controls and Definitions

The native Tk panel now edits recovery threshold, magnitude ordering and elemental
region fields (including their timestep), with Apply/Return, validation and real
save/load dialogs. Widget inventory is **36/36**. The Windows capture helper
requires fresh output and a successful nonblank manager capture;
`draw-controls/manager_controls.png` has been visually reviewed.

Definition JSON v5 stores recovery options and exact signed 64-bit region IDs;
v1-v4 use recovery defaults. Invalid restoration preserves live definitions and
options. Original physical fields and fixed isosurface bindings remain independent.
Full ON 38/38 and OFF 8/8 pass every case three times; logs are
`build/mingw/cut-n03-recovery-tests.log` and the OFF counterpart.
N03.2 tensor conventions/measures are next; N03 remains partial.

## N03.2 Tensor Measures and Original Physical Evaluation

Six/nine component conventions, symmetric tensor measures, before/after recovery
and the numerical/missing-value limits are recorded in [the contract](cut-view-tensors.md).
Native references use 30 rotations of known eigenvalues across four layouts; DRAW
checks a standard nine-component tensor fixture, original force and all masks.
The Tk inventory is now **39/39** and tensor/source scalar cloud images are reviewed.
JSON v6 persists conventions. `EvaluatePhysical()` and DRAW integration bypass display
recovery/derivation, including invalid unrelated display layouts. Full VTK ON
**41/41** and OFF **10/10**, every case three times, are recorded in
`cut-n03-tensor-tests.log` in the respective builds (424.30 / 8.22 seconds;
123 / 30 successful executions). The full ON gate covers both late batch Tk
initialization and interactive Tk with the portable `pack` layout. N03.3 physical
traces is next.

## N03.3a Prepared Interface Sources

The prepared VTK source retains reciprocal trusted volume-face references and
returns two independent original-FE weight sets at a shared point. Independent
affine/quadratic and element-node references, deformation, curved analytic and
numerical faces, ambiguous/disconnected rejection, all masks and unchanged
default physical integration pass; see [the bounded scope](cut-view-traces.md).
Full ON **42/42** and OFF **10/10**, each three times (126 / 30 successful
executions; 444.71 / 4.70 seconds), are recorded in `cut-n03-interface-tests.log`
in the respective builds. The UI remains **39/39**.

N03.3 remains partial. N03.3b binds both sources to generated primitives and
the native fast slice; N03.3c adds independent selection/integrals, N03.3d adds
controls/migration, and N03.3e completes interface acceptance. Presentation OR
union/cap ownership remains N03.4.

## N03 Closure

N03.1 recovery controls, N03.2 tensor conventions, N03.3 independent interface
traces and N03.4 actual presentation OR are verified. The full gate found and
repaired degenerate float sphere-pole BRep seeds and shared internal curved faces
of compsolids. Curved On-only caps keep OCCT surfaces; original-cell topology
controls exposed boundaries. Warm sphere masks have independent analytic references.

Final ON 48/48 and OFF 14/14 each pass three times (144 / 42 executions,
538.76 / 47.00 seconds), including 47/47 real Tk controls, held pointer updates,
the 48-concrete-cell matrix and transparent cloud comparisons. Source repair:
`3a1fd44`; diagnostics/transparent gate: `eba2a37`. Logs and explicit support
limits are recorded in [the N03 audit](cut-view-n03-audit.md).
At that N03 milestone, N04 became active. Current N04 status is in
[its queue](cut-view-n04-plan.md); N05 is closed at [its native audit](cut-view-n05-audit.md); N06 is closed at [its native audit](cut-view-n06-audit.md); N07 is closed at [its native audit](cut-view-n07-audit.md); N08 is closed at [its native audit](cut-view-n08-audit.md); N09 is in final validation. Full-frame
performance, complete native sessions and excluded Web/WASM work remain open.

## N04 Closure


N04.8 closure: Final VTK ON **58/58**, OFF **22/22**, every case three times (174 / 66 executions; 547.47 / 41.50 seconds). DRAW/Viewer-disabled import/reference tests **2/2** each pass three times (0.52 seconds). The final gate preserves 47/47 Tk controls, the 48-cell matrix, independent N02 geometry/FE references, N03 physical/display fields, actual OR, curved caps and transparency. Ordinary Viewer images and all six kernel peak-memory gates were reviewed. Static field-step repair: 76d845b; first failed ON log retained. All current documents reflect the native support contract; N05 is closed at [its native audit](cut-view-n05-audit.md); N06 is closed at [its native audit](cut-view-n06-audit.md); N07 is closed at [its native audit](cut-view-n07-audit.md); N08 is closed at [its native audit](cut-view-n08-audit.md); N09 is in final validation. See cut-view-n04-audit.md.

N05 closure: ON **61/61**, OFF **24/24**, headless **2/2**, every case three times. Real main/tools inventory **48+36**, original-FE pointer picking, HUD legend/vector/annotation/CAD images verified. See cut-view-n05-audit.md. N06 is active.
