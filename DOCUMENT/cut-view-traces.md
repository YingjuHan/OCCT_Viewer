# N03 Physical Interface Traces

N03.3a-e are verified. Generated sections retain selectable independent display
and physical traces; lowest remains the default.

## N03.3a Prepared Source Foundation

`MeshVtkCutSource::Prepare()` now retains reciprocal `NeighborElement` and
`NeighborFace` references after the existing topology, geometric compatibility
and opposite-support checks have verified a two-sided volume face. `Owner`
remains the lowest original element ID. Neighbor references are valid only when
`Internal` is true. Disconnected coincident coordinates, same-side duplicate
cells and non-manifold face groups do not acquire a trusted pair.

`InterfaceTraces(element, face, position)` returns two records ordered by original
element ID. Each includes its own face index, deformed interior point and
original-FE interpolation weights. It checks source readiness, reciprocal
topology, finite coordinates and distance to the prepared face, then evaluates
both original cells separately. The face-distance allowance is
`max(1e-7, 2 * prepared geometry tolerance)`, in model units. This is the existing
bounded geometry tolerance, not an exact point-containment certificate.
Analytic Bezier faces use projection within their `[0,1]` parameter domain;
other prepared faces use distance to their boundary triangles. Neither path
creates adjacency from coordinate welding or scalar-field values.

The returned records own their weights, which reference original mesh node IDs;
they do not reference a temporary scalar projection. An element-node result can
therefore evaluate the same shared position against each original element tuple
and retain a jump, even when the node IDs are shared. Geometry/source mutation
still requires the manager's existing invalidation contract.

The native `cut_interface` test exercises adjacent hexahedron/voxel, quadratic
and Lagrange hexahedron, polyhedron and native tetrahedron source preparations.
Independent affine/quadratic references verify each side's weights and
element-node jump. Curved analytic and numerical faces, deformation, reverse-side
lookups, all visibility masks and the unchanged default physical integral are
included. Exterior/invalid/unprepared faces, nonfinite/off-face points,
disconnected topology, duplicate support and three-owner ambiguity are rejected.
Surface, line and point cells cannot masquerade as volume-face interfaces.

This preparation API is compiled only with VTK. The native tetrahedron check
uses that prepared-source API; it does **not** implement trace selection in the
VTK-disabled fast slice path. Generated section primitives still retain the
existing single display owner, and integration has no new policy option yet.

Acceptance: full VTK ON **42/42** and OFF **10/10**, each case three times,
126 / 30 successful executions in 444.71 / 4.70 seconds. Logs are
`build/mingw/cut-n03-interface-tests.log` and the corresponding OFF build log.
The focused `cut_interface` gate also passes three times in 1.89 seconds.

## N03.3b Generated Primitive Binding

`MeshGeometryTraceSection` retains ordered source records on generated faces,
lines and points. `MeshCutTraceSource` pairs original-node boundary topology
with opposite support; prepared volume boundaries additionally retain their
verified reciprocal adjacency. Each side owns original FE weights. Coincident
disconnected nodes do not pair. Untrusted multiple owners remain marked ambiguous.
The generated primitive is retained once using the trusted source pair and FE
weight fingerprints, with no coordinate welding. Lowest ID remains the default.

VTK ON uses original cell inverse mapping for each side, including quadratic
and Lagrange cells. OFF native conforming faces/edges retain their original
weights. Source/deformation invalidation clears the topology cache. Fast plane
translation retains output connectivity where the backend reused it; recovery
edits retain section geometry. Element/nodal subset mapping remains supported.

Acceptance: ON full **43/43** in 145.75 seconds; OFF **11/11**, each three times,
in 3.65 seconds; focused ON **4/4**, each three times, in 2.43 seconds.
Logs: `cut-n03-binding-tests.log` in both build directories and ON
`cut-n03-binding-focused.log`. The new test covers all masks, connected and
disconnected volume/surface sections, deformed source weights, FE jumps,
default flux integrals and VTK line endpoints. The prepared-interface test now
checks generated source binding for hex/voxel/quadratic/Lagrange/polyhedron.
Manager and projected-cloud images were inspected. Selection remains N03.3c;
this binding and duplicate ownership do not constitute an OR union.

## N03.3c Independent Selection API

Each definition now has `DisplayTrace` and `PhysicalTrace`. `SetTraceOptions`
preflights the requested policy on a materialized section, including a hidden
section, and rejects unresolved or ambiguous choices without altering definitions
or retained display selection. Lowest preserves legacy behavior. Negative and
positive compare the deformed source interior to the cut; region must resolve
exactly one side of a trusted interface. A non-interface single source is retained.
Untrusted multiple-owner records reject explicit selection.

Region assignments and the selected ID are signed int64. Display/physical
assignment vectors are independent of recovery regions. Geometry retains display
selection for subsequent renderer property calls; `EvaluatePhysical` explicitly
maps original fields with physical options and never changes that display state.
`PhysicalLineField` provides the independent shell-line mapping. Existing
shell/beam integration and non-planar local-normal limitations still apply.

Plane side follows its oriented normalized equation. Sphere/cylinder use signed
radial distance. Iso side uses the fixed original cutting scalar (or its existing
equal-weight nodal projection), evaluated with each cell's original-FE interior
weights. Side values within the applicable absolute geometry/field tolerance are
ambiguous. This interior-side rule does not certify an arbitrary nonlinear field
as one-signed throughout an entire element.

Acceptance: focused ON **11/11**, each three times, 5.20 seconds; OFF full
**11/11**, each three times, 5.73 seconds. `cut-n03-selection-tests.log` in both
builds covers all masks, reversed normals/iso fields, volume/surface/line traces,
lowest/negative/positive/exact extreme int64 regions, independent original flux
integrals, hex/voxel/quadratic/Lagrange/polyhedron positive/negative sources,
sphere line-side selection and atomic invalid-region/non-manifold rejection.
Native controls and versioned persistence are the next separate N03.3d item.

## N03.3d Controls and Persistence

DRAW `trace id display/physical` reports the independent policy, chosen region and
assignments. Set with `lowest`, `negative`, `positive`, or
`region regionId {elementRegionIds}`. Assignments follow original element ID
order. Decimal signed int64 parsing never converts through double. The manager
has two policy menus, two region inputs, two assignment inputs and two Apply
buttons; invalid edits report an error and restore the prior values.

JSON v7 requires both trace objects, with strict types/counts and exact int64 IDs.
Source-policy resolution is preflighted on isolated caches before replacing live
definitions. Versions 1-6 migrate to lowest with empty assignments. Recovery
regions remain independent. Native tests verify round-trip, fractional/overflow/
missing-region/count rejection and v6 migration. DRAW verifies a 7/9 interface
jump, independent original flux, reversed normal and failed-edit/restoration
atomicity. Visual negative/positive clouds change blue/red as expected.

Acceptance: focused ON **5/5**, each three times, 53.30 seconds; OFF full
**12/12**, each three times, 4.66 seconds. Logs are
`cut-n03-trace-controls-focused.log`. Actual Tk inventory is **47/47**.
After reducing the list height, the final layout gate passed in 16.08 seconds
(`cut-n03-trace-controls-layout.log`); inspected manager images show all controls
and Close within the window. Both trace cloud images were inspected.

## N03.3e Full Matrix Closure

Final ON **45/45**, every case three times, **135 successful executions** in
501.41 seconds; OFF **12/12**, every case three times, **36 executions** in 7.81
seconds. Logs: `cut-n03-trace-matrix-tests.log` in both build directories.
The selection matrix now crosses deformation, plane/iso orientation, all masks
and volume/surface policies. VTK line endpoints cross all four policies/masks;
OFF explicitly rejects VTK-only line topology. DRAW independently checks selected
original stress force and moments while tensor display conventions change.
All native/DRAW/visual suites and the **47/47** Tk inventory pass repeatedly.

The full family gate found and fixed one uncovered case: empty/polyvertex 0D
cells have no FE interior chart and must not attempt an interior inverse mapping.
They cache available original node weights and remain non-interface cells.
The fixed VTK family test and its dependent native/visual DRAW cases pass.

Nonmatching interfaces are unpaired; explicit selection of ambiguous multiple
owners is rejected. Side classification follows the documented source-interior
rule. VTK-only curve topology remains unavailable in OFF. Non-planar physical
integration and shell/beam section adapters remain the existing later-stage
work. Million-cell full-frame performance remains N08. OR union is separate.

## Historical N03.3 Queue

1. N03.3b is verified above. Nonmatching topology does not acquire a trusted pair.
2. N03.3c: introduce explicit lowest/negative/positive/region policies. Determine the
   geometric side from the cut definition and deformed source interiors; reject
   an ambiguous choice. A region request must resolve exactly one available
   source and retain signed int64 IDs.
3. Bind display and physical selection separately in N03.3c. Physical region assignments
   must be explicit and independent of display recovery regions. Changing a
   contour measure, averaging threshold or color region must not alter force,
   moment or heat flow.
4. Map each chosen trace using its own original source field and weights in N03.3c.
   Surface/line/volume intersections, deformation and all masks need acceptance.
   Preserve existing shell/beam and non-planar integration limitations until
   their explicit section/local-normal adapters are implemented.
5. N03.3d adds native/DRAW controls and atomic definition migration for the policy;
   verify invalid edits/restoration, region jumps, side orientation and both
   VTK configurations. N03.3e repeats the full acceptance matrix. N03.3 completion
   requires all of these checks; each subitem is committed separately after acceptance.

Presentation OR boundary/cap union remains N03.4. Double-source provenance and
duplicate removal alone do not satisfy that separate acceptance gate.
