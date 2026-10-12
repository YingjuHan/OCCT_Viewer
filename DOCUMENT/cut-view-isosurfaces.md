# Original FE numerical isosurfaces

Plane, sphere and cylinder cuts keep their geometric OCCT intersection paths.
The scalar isosurface is a separate cut definition with its own fixed field,
raw component, time step and threshold. Changing the displayed contour or its
recovery policy does not rebind that definition. Elemental cutting fields use
an equal-weight projection of the selected scalar to original nodes, independent
of display recovery; the original CellData remains available for physical results.

## Representation and acceptance

The established first-order native scalar cutter is retained. With VTK enabled,
generic chart cells and native second-order cells use `MeshGeometryVtkIso`.
It evaluates geometry, scalar values and provenance with the original cell's
FE functions in parameter space, including rational weights and deformation.
Interpolation weights are retained without clamping negative coefficients.
Boundary preparation and analytic OCCT geometry modes do not substitute for
this explicitly numerical field path.

Initial parameter simplices come from the SDK chart triangulation. Pyramid
types 14/27/37 use the full cube chart with its collapsed top; type 33 uses a
cube decomposition because the SDK local triangulation includes virtual IDs.
Every cell is uniformly subdivided at least to the configured minimum depth.
Edge midpoint and simplex centroid samples check both geometry and field
interpolation residuals, including regions with equal corner signs. Tetrahedra
choose the shortest internal octahedron diagonal during subdivision.

An edge crossing is bisected using original FE evaluation. Surface cuts produce
lines; volume cuts produce section triangles and clipped tetrahedra. FE weights
and original element IDs accompany their nodes and elements. Exact duplicate
section primitives within an original element are removed. Different original
elements keep their separate provenance and physical traces.

`MeshCutIsoOptions` / DRAW `isooptions absolute relative minimumDepth limit primitives`
control field refinement. The field tolerance is
`max(absolute, relative * max(abs(original scalar coefficients)))` **in field
units**, independently of `MeshCutGeometryOptions` tolerances **in model units**.
Midpoint/centroid interpolation errors must be at most one quarter of these
tolerances; emitted volume-section facet centroid residuals must also pass.
The reported field residual is the largest checked interpolation/facet residual,
not just the residual of an edge root. Result diagnostics include field tolerance,
field residual, geometry residual, subdivision depth and source simplex count.
Input and output coordinate storage remains float.
`IsoRange` reports original coefficient bounds for the threshold control; it is
not a certified bound on higher-order FE overshoot between coefficients.

Subdivision, sampled points and output primitives have explicit limits.
Preparation publishes its mesh/maps only after all cells pass. Failed display
synchronization retains the existing presentations and synchronized revisions.
Threshold sliders update before pointer release; Escape restores the definitions
captured at pointer press. Version 4 definition JSON stores the iso options;
versions 1–3 migrate using their defaults. Invalid options/bindings/restores are
rejected without changing valid definitions.

## Supported scope and remaining limits

The native reference suite checks SDK chart types
3/5/8/9/10/11/12/13/14/21/22/23/24/25/26/27/28/29/30/31/32/33/34/35/37/
68/69/70/71/72/73/75/76/77/78/79/80. This is a bounded numerical capability,
not certified arbitrary-degree root isolation. Finite samples can miss narrower
or more oscillatory crossings; increasing minimum depth is a user-controlled
resolution choice, and subdivision/residual/budget failures are explicit errors.
Rational/polynomial maps still have to be numerically evaluable by the SDK.
No claim is made for self-intersecting or folded input elements.

Generic world-coordinate MVC polygons/polyhedra, convex point sets, polylines,
triangle strips and point-only cells are rejected by the new chart path. Their
geometric plane/sphere/cylinder support is unchanged. Mixed high-order meshes
with native polygon/polyhedron/point cells are likewise rejected. Native
first-order behavior is retained. Constant-level volume plateaus have no unique
surface and are explicitly rejected. VTK-disabled builds keep the first-order
cutter and reject generic/second-order bindings.

Uniform refinement is conforming within a chart. Separate original elements
retain independent refinement and provenance; arbitrary mismatching interfaces
are not welded. Physical interface trace policy and presentation OR union/cap
ownership belong to N03. Threshold edits currently rebuild this numerical field
representation while retaining the manager's source epoch; large-model caching
and full-frame budgets remain N08 work.

## Independent references

`cut_iso` checks affine reproduction on the 37 chart types; nonmonotone
quadratic fields with two crossings despite identical corner signs; independent
tetra/hex area and volume partitions; polynomial enclosed circle/sphere results
and refinement convergence; rational curve/quad/hex roots from an independently
derived quadratic equation; provenance, all masks, CellData, fixed step bindings,
curved/nonuniformly deformed geometry, invalidation, options persistence and
failed preparation without replacing the last valid display.

`draw_cut_view_high_order_iso` computes its own root and area formulas from a
generated Lagrange triangle fixture. Its visual variant uses actual Tk press,
held-motion and Escape events, checking pre-release FE values, geometry,
presentation revisions, cloud pixels and rollback. Generated fixtures and images
are build outputs, not repository input data.
