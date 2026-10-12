# N03 Physical Interface Traces

N03.3 is in progress. Its source foundation is separate from the selectable
display/physical trace feature; the current viewer and integration still use
the existing lowest-original-element rule.

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

## Remaining N03.3 Work

1. N03.3b: preserve both source records and their own FE weights on generated shared
   faces/lines/points before presentation ownership removes a duplicate. Native
   fast slice and general VTK geometry need the same contract. Pair trusted
   topology, not equal coordinates or equal mapped scalar values.
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
