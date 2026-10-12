# Native Cut View Manager Design

## Scope and Status

This is the implementation design for the remaining native work. Web, Vue,
TypeScript, browser acceptance and WASM SDK/packaging are excluded from this
execution track. B03 means the native Viewer/C++ interface only. S01 means a
native session JSON format, not edits to the TypeScript ProjectStore/SavedViewStore.
The original roadmap retains excluded work for a later, separate scope.

Design baseline: 22 native/DRAW cases passed three times, with 48 concrete VTK cell
types. N01 is now implemented and verified: the expanded 25-case suite passes
three consecutive runs per case. Design completion does not imply completion
of N02-N09 or the full native manager. N02's numerical foundation passes the
expanded 28-case suite three times; its analytic second-order geometry work
is verified at its [bounded closure scope](cut-view-n02-audit.md).
N03 is verified at [its bounded closure scope](cut-view-n03-audit.md): ON 48/48
and OFF 14/14, each case three times, with 47/47 real native controls.
[N04](cut-view-n04-plan.md) implementation gates are verified: ordinary scalar
general-cell display, validated format/series contracts, import budgets and local
reference staging/reports. Its [final audit](cut-view-n04-audit.md) is verified;
N04 is closed at that documented native scope.
N05-N09 remain scheduled.

## Existing Boundaries

MCP Tier 2 discovery identifies MeshCutViewManager as the geometry/field branch
owner, InteractiveCaeCutView as presentation owner, and ViewCaeTest as the DRAW
adapter. Partial graph ranges were checked against source. Before N01,
MeshGeometryVtkCut repeated source-cell construction per portion and drag revision;
it now shares a manager-owned MeshVtkCutSource across those branches.
MeshBvhData has OCCT BVH infrastructure but native element adaptors do not index
generic VTK cells. A general source index must therefore use prepared geometric
bounds, not pretend the existing native index covers all VTK topology.

Responsibilities stay in the existing modules:

| Module | Responsibility |
| --- | --- |
| RWCae | Original topology, entity IDs, fields and physical metadata |
| RWVtk / RWMeshConv | Import, steps/composites, mapping and structured diagnostics |
| ViewCaeGeom | Source preparation, cutting, provenance, recovery and integration |
| ViewCae | Presentations, legends, probes, vectors and native AIS interaction |
| Viewer | Model lifecycle, CAD synchronization and native session orchestration |
| source-draw / test | One VIEWDRAW plugin, Tcl manager and automated acceptance |

## Core Contracts

### Geometric Results

Plane/sphere/cylinder sections are intersections with source geometry, never
field-isosurface substitutes. Preserve negative/on/positive as independent bits
1/2/4, including mask zero. Boundary outputs remain explicitly identified as
boundary meshes; their triangles are not replacement volume cells.

Every generated point has original-cell FE weights and every primitive has
original-element provenance. Do not weld nodes across elemental discontinuities.
Display recovery and physical integration are separate bindings. Physical units,
coordinate systems and shell/beam mappings must be explicit, not inferred from
field names or visual smoothing.

### Cache Ownership and Invalidation

A manager-owned source epoch shares prepared VTK evaluators, tessellations,
topological face records, cell boxes and lazily constructed OCCT shapes across
cuts, portions and drag revisions. Prepared evaluators retain coordinates/IDs,
not pointers to replaceable presentation roots. Bind provenance to the current
branch root when publishing a result.

| Change | Invalidation |
| --- | --- |
| Plane position/normal, sphere center/radius, cylinder axis/radius | Cut result only; retain source epoch |
| Visibility, selection, style or legend | Presentation/visible result selection only |
| Contour values/components/step or recovery policy | Field mapping only, unless also used as deformation/cut field |
| Nodes/connectivity, deformation binding/scale/step, geometric tolerance | Source epoch, adjacency, BVH and all geometric cut results |
| Close/replace | Detach aids, cancel drag/jobs, release all source and result caches |

Existing callers mutating data in place must still call InvalidateGeometry or
InvalidateFields. Later time-step APIs will own this notification automatically.
No cache key may use a pointer alone to infer data freshness.

The first cache is single-threaded and scoped to one source epoch. OCCT BVH reuse
is supported; calling mutable VTK evaluators concurrently is not. Hard memory
budgets/LRU eviction follow in N08 and must precede a million-cell claim.

### Adjacency and Face Ownership

Identify shared faces by full original node IDs, edge connectivity and compatible face type, not
floating-point vertex welding. Require exactly two compatible volume owners and
opposite geometric support sides before hiding an internal planar, straight-sided face. Keep
ambiguous/nonmanifold or disconnected coincident faces visible and report them.
N02 additionally checks canonical curved tessellations and opposite interior
points before filtering conforming curved faces. Mismatching/nonconforming
faces stay visible; coordinate proximity alone never establishes adjacency.

Use OCCT boolean history to associate result fragments with source boundary
faces. Never hide a newly generated cap merely because it is coplanar with a
shared face. Side/uncut presentations omit verified interior walls and their
area contributions, but volume measures still sum complete cell solids.

A cut exactly coincident with a verified shared face emits it once. Initial
trace ownership is the lowest original element index and is deterministic.
N03 adds explicit negative/positive/material traces for discontinuous data and
physical integration. Cap deduplication between portions and different cuts
belongs to N03, not a global vertex merge that loses field provenance.

### Spatial Queries

Build an OCCT BVH from tessellated, deformed cell boundaries. Nodal boxes/signs
alone are not safe for high-order overshoot or an enclosed sphere/cylinder.
Plane interval tests, sphere box-distance intervals and conservative radial
cylinder bounds select candidates. Stable original-element ordering makes
output/provenance reproducible. Cold source preparation and warm drag work are
reported separately.

Expose source epoch, prepared cells, shape builds, BVH builds/box tests,
candidate cells, boolean operations and filtered internal faces through a
native statistics API and DRAW. Later frame instrumentation adds source,
intersection, interpolation, primitive and redraw timings plus memory estimates.

## Ordered Native Increments

| Stage | Roadmap IDs | Design and acceptance |
| --- | --- | --- |
| N01 | G02, P01 foundation | Shared source preparation, reusable conservative BVH, lazy shapes, conforming planar internal-face filtering. Compare cached results with a fresh manager; test invalidation, adjacent cells, coincident cuts, all masks and held-pointer statistics. |
| N02 | G02, G03 | Absolute/relative geometric tolerance and refinement limits; shared curved-edge/face refinement; exact supported analytic cells and documented numerical general-cell convergence. Test curved reference geometry and FE maps independently, including float-storage precision limits. |
| N03 | G04, G05 | Recovery policies with region/material boundaries and thresholds; explicit physical trace selection; OR boundary union and cap ownership without double-opacity artifacts. Preserve constant CellData jumps and test overlapping/touching cuts. |
| N04 | V01-V05, B01-B02 | Ordinary native viewer point/line/general-cell presentation; format support reports; transactional composite/series import with per-step topology; configurable decoding limits and relative reference staging. Native fallback success must not print a failed overall-import diagnostic. |
| N05 | M02-M07, B03 native | Datum frames, local/global editing, per-cut/portion styles, legends, picking/probes, vectors/annotations and CAD synchronization. Reuse existing OCCT manipulators, color mappers and native Viewer APIs; extend the existing Tcl panel rather than introduce a web UI. |
| N06 | D01-D03 | Material-following frames with explicit reference/updated configuration; atomic geometry/result/deformation step switching; animation and regular/range/rotational/path slicing. Reject incomplete steps instead of reusing stale fields on new topology. |
| N07 | I01-I03, A01-A02 | Group/units/frame-aware resultants, explicit shell/beam/composite thickness and section mappings, overlays/reports, mapped crack-distance/density cuts. Unknown conventions and missing mappings fail explicitly; never fabricate physical section results. |
| N08 | P01-P03 | Bounded source/result caches, eviction/cancellation, latest-generation publication and full native frame instrumentation. Accept million-cell crossing/rotating/deformed cuts only with fields and actual redraw at >=15 FPS / p95 <=67 ms; add long-session leak/stale-handle tests. |
| N09 | S01 native | Versioned native session JSON: source references/digests, definitions, field/deformation/step bindings, frames, styles, legends, recovery/integration settings and camera. Resolve and validate in staging, then commit atomically; invalid restoration preserves the live model. |

N04 diagnostic repairs may be pulled forward when they obstruct testing. Native
session schema is designed now but implemented after stable frame/style/step
contracts. Do not serialize OCCT shapes, raw pointers or cache entries.

## Interaction and Publication

Keep geometry, fields and presentation synchronized before mouse release. A
slider/manipulator edit uses the same validated native update path as numeric
editing. Abort restores geometry, bindings and displayed pixels. Selection never
implicitly activates a cut. Empty manager, all disabled, mask zero, invalid input,
close and replacement remain distinct, tested states.

The synchronous path remains the initial correctness reference. Any later
worker path receives an immutable source snapshot and cut generation; only the
latest successful generation may update AIS on the viewer thread. Closing or
replacing a model invalidates pending work. Failed preparation/result generation
must not publish a partial mesh as a completed result.

## Verification and Completion

Each increment includes focused native numerics and DRAW Tcl scripts under
test/. Retain the 48-cell and real-data regressions and both VTK-enabled/disabled
builds. UI additions extend the widget inventory so every new button/control has
an automated case. Image tests require meaningful geometry/field assertions as
well as nonblank pixels; held-drag checks precede lazy evaluation and release.

N01 tests include adjacent hex/voxel/polyhedron/high-order cells, disconnected
coincident nodes, source-only boundaries, interface cuts, deterministic CellData,
BVH rejection versus exhaustive reference, warm reuse, deformation/topology
invalidation and cache release. A performance test reports cold/warm work and
never substitutes a generous timeout for an interactive frame budget.

Milestone completion requires implementation, tests, build, source-qualified
evidence and an updated execution status. N01 alone does not complete G02, P01
or the native manager. Excluded web tasks are not silently marked completed.

### N01 Acceptance

- Implemented shared source epochs, lazy OCCT shapes and reusable OCCT BVH
  queries for geometric plane/sphere/cylinder cuts and source-only presentation.
- Verified conforming planar, straight-sided internal-face filtering on joined
  hex, voxel, quadratic hex, Lagrange hex and polyhedron fixtures. Coincident
  interface sections use the lowest original element index once; disconnected
  and ambiguous topology is not welded away.
- Added `MeshCutViewManager::Statistics()` and `scvcutview model stats`, plus
  `general_cut_cache`, `draw_cut_view_cache` and its held-pointer visual variant.
  Motion and field edits reuse source preparation; node/topology/deformation
  edits rebuild it. A 24-cell chain creates only one candidate OCCT shape.
- The full 25-case native/DRAW suite passes three consecutive runs per case;
  the VTK-disabled Release suite also passes all three cases three times.
  Held-pointer images are in `build/test/native/draw-cache/`.
- N02 now verifies numerical convergence and conforming curved-reference adjacency.
  Mismatching interfaces, exact second-order source geometry, bounded memory and
  full-frame performance are not closed by N01.

### N02 Implementation Contract

`MeshCutGeometryOptions` defaults to absolute tolerance `1e-6`, relative tolerance
`1e-3`, face/line subdivision ceilings `7/12`, and at most `2000000` source primitives.
The effective tolerance is the larger absolute/relative value, using the deformed
nodal diagonal. Changed options invalidate the source epoch; identical or invalid
options do not. Refine/budget failure publishes no partial source cache.

Analytic sphere/cylinder tool faceting also caps deflection at one percent of
the tool radius, independently of the model-relative source tolerance. This
preserves small-tool line/contour quality without tying all source refinement
to a moving radius. Tight absolute tolerances still take precedence.

Parametric triangle/quad faces use canonical corner seeds and uniform dyadic
subdivision, with a shared curved-volume depth for the source epoch. This avoids
adaptive T-junctions and matches shared rational/high-order face grids. Planar,
straight-sided faces retain their fast path. Matching original topology and sampled
tessellation plus opposite interior points are required for curved-wall filtering.
Mismatching faces remain distinct. N08 may optimize connected components/refinement,
but must preserve these numerical/adjacency gates.

This is a sampled, bounded numerical geometry policy, not an exact or certified
arbitrary-order error bound. Output still uses float mesh coordinates; statistics
expose storage resolution separately from requested tessellation tolerance.
Analytic affine source/tool measures remain exact within OCCT numerical precision.
The new `cut_geometry` and DRAW geometry cases verify decreasing warped-hex reference
error, curved-line intersections, joined higher-order/rational cells, limits and
held-pointer results. The full 28-case suite passes three consecutive runs per
case; the VTK-disabled native/DRAW build passes all six cases three times.

The analytic N02 increment now converts supported polynomial/rational maps through
degree six to OCCT Bezier surfaces/curves. Triangular charts use a rectangular Duffy
map; homogeneous coefficients preserve rational weights. Independent parameter
checks guard the conversion and positive-weight control hulls supplement numerical
boxes. Source shapes remain lazy and shared. Numerical/preferred/required modes
make fallback explicit; required mode rejects unsupported charts atomically.
Planar quadratic polygons use curved wires and original quadratic edge functions
on their boundary. Exact source shape measures use adaptive OCCT integration;
display meshes remain tessellated floats. All 31 native/DRAW cases pass three
consecutive runs per case, and all six VTK-disabled cases pass three times.
That milestone still needed the remaining curved-family reference evidence;
general arbitrary-order geometry is not labelled exact.

The remaining-family increment adds independent curve/surface/tetra references,
mixed-order hex 33, degree-six/anisotropic charts, original nonlinear FE values
and numerical convergence. It preflights finite positive rational weights and
returns unsupported for normalized weights `<=1e-12` before endpoint sampling,
preserving preferred-mode numerical fallback and required-mode atomic rejection.
Uniform weight scale is not the precision limit. See the N02 queue for acceptance
and the remaining generic/high-order field isosurface and closure tasks.

### N03 Recovery Increment (Partial)

`MeshCutRecoveryOptions` and `RecoverCutField()` now implement display recovery
with one signed 64-bit region ID per original element and a threshold in `[0,100]`.
For each component, the same-region spread at a shared node is compared with
the global finite component range times the threshold percentage. Rejected
averaging and missing traces stay in elemental-per-node storage. This is the
current project contract; equivalence to Abaqus averaging is not claimed.

The default scalar-component policy retains nodal projection. Custom policies
and original element-node input use `MeshNodeVecProperty`. Vector magnitude
supports evaluation before or after averaging; tensor measures now have the explicit
[component/convention contract](cut-view-tensors.md). Recovery validates every raw
component, including cached magnitude, and retains component/field/time metadata.
Finite means, global range comparisons and vector norms avoid intermediate
overflow; NaN traces are preserved and infinite recovery input is rejected.

`MeshCutResult::PhysicalFields` maps the original physical field independently
of display recovery. Integration uses it and rejects a recovered result that
has lost the physical mapping. NodeVec line/point adapters retain original
element provenance and signed FE weights. DRAW exposes recovery configuration;
manager widgets and JSON v5 recovery definitions are verified (36/36 controls).
Complete native session persistence remains N09.

### N03 Recovery, Trace and Union Contracts

Use the existing `RWCae::MeshNodeVecProperty` elemental-per-node storage for
recovered values. A shared node can retain separate values for different material
regions or rejected averaging thresholds; no global vertex duplication/welding
is required to preserve the original connectivity. Bind explicit original-element
region IDs and a normalized averaging threshold, and invalidate field mapping
only when recovery settings change. Vector/tensor recovery must define whether
invariants are evaluated before or after averaging, rather than silently mixing
those conventions. Extend line/point contour adapters to carry the same trace.

The separate physical path now supports explicit lowest/negative/positive/region
interface traces. Integration is invariant under display-policy changes. Trace
selection preserves discontinuous CellData and explicit material-side semantics;
it evaluates the selected original field and original-FE weights.

The prepared VTK source and VTK-disabled fast slice retain reciprocal trusted
topology references, bind both original-FE weight sets to generated primitives
and apply independent display/physical policies; see [the trace scope](cut-view-traces.md).

Numerical `Evaluate(id)` results remain independent. A separate presentation union
uses source-cell provenance and OCCT boundary operations for the OR of visible
side portions, followed by deterministic section/cap ownership. Do not globally
merge coincident coordinates across elemental jumps. Test overlapping, identical
and touching cuts, all masks and transparent images; numerical union measures and
actual rendered opacity pass N03.4's [acceptance gates](cut-view-union.md).
Later per-cut style overrides use an
explicit stable ownership rule for coincident visible faces.

### N02 Closure

[The bounded audit](cut-view-n02-audit.md) records the original-FE numerical
isosurface contract and independent field/model tolerances alongside analytic
and numerical geometric paths. Repeated complete native/DRAW/visual acceptance
passes in both VTK configurations. N03 now owns recovery controls/persistence,
tensor conventions, physical traces and presentation OR/cap ownership.

### N03.1 Definition Recovery Contract

JSON v5 includes threshold, before-order flag and exact signed int64 region IDs.
Restore validates the whole document in staging and applies options only after
all cuts resolve. Legacy v1-v4 restoration uses default recovery settings. Tk
threshold edits stay as drafts until Apply/Return; periodic refresh preserves
drafts, while successful Load refreshes the persisted settings. The region menu
can retain exact restored IDs without converting them through a floating scalar.

### N03.2 Symmetric Tensor Display

Tensor measures use explicit six/nine layouts and tensor/engineering shear. The
before path derives source scalar tuples then recovers; the after path recovers
components then derives source scalar tuples. Both use scalar FE mapping onto
generated cut points. Missing tuples stay missing, infinity/nonsymmetry/finite
result overflow reject; normalized Jacobi/norm intermediates cover extreme scales.
`EvaluatePhysical()` bypasses all display policies; `PhysicalFields` remain original.
JSON v6 persists conventions with v1-v5 defaults. The tensor milestone verified
39/39 real widgets and independent rotated tensor/native/DRAW/visual references.
N03.3 subsequently added trace controls and JSON v7 (47/47 widgets); N03.4 added
actual OR union. Complete native sessions remain N09.
