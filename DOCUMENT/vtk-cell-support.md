# VTK Cell Support and Acceptance

## Scope

Enable `OCCTV_USE_VTK=ON` with VTK 9.5 or newer for original VTK topology,
shape-function interpolation, and geometric plane/sphere/infinite-cylinder cuts.
The current native SDK is VTK 9.5.2. Its 48 instantiable cell types are listed below.
Reserved IDs and abstract or unimplemented higher-order placeholder types are
rejected explicitly; they are not counted as concrete cells.

| Family | Concrete VTK IDs | Acceptance |
| --- | --- | --- |
| Empty, vertex, polyvertex | 0, 1, 2 | Empty handling; point classification and contour markers |
| Line, polyline | 3, 4 | Section points and clipped/source contour lines |
| Triangle, strip, polygon, pixel, quad | 5, 6, 7, 8, 9 | Geometric surface sections and clipped surfaces |
| Tetra, voxel, hexahedron, wedge, pyramid | 10, 11, 12, 13, 14 | Three portions and original-cell interpolation |
| Pentagonal and hexagonal prism | 15, 16 | Three portions; bounded affine correction for ID 15 |
| Quadratic edge/triangle/quad/tetra/hex/wedge/pyramid | 21, 22, 23, 24, 25, 26, 27 | Original shape functions, not corner-only interpolation |
| Biquadratic/triquadratic/mixed-order cells | 28, 29, 30, 31, 32, 33, 34, 37 | Three portions and source provenance |
| Cubic line, quadratic polygon | 35, 36 | Curve sampling; quadratic-polygon world-coordinate MVC |
| Convex point set, polyhedron | 41, 42 | Tetrahedralization boundary; closed oriented triangular MVC cage |
| Lagrange curve/triangle/quad/tetra/hex/wedge | 68, 69, 70, 71, 72, 73 | Original connectivity and shape functions |
| Bezier curve/triangle/quad/tetra/hex/wedge | 75, 76, 77, 78, 79, 80 | Original connectivity and rational weights |

The matrix tests canonical reference geometry, affine nodal data, constant
CellData, provenance, geometric cut constraints, and all eight portion masks.
Native extensions test anisotropic quad/wedge degrees, 343-node hexahedra,
non-unit rational weights for all six Bezier families, quadratic fields,
generic-volume integration, and invalid degree/weight metadata.

DRAW images cover points, curves, surfaces, and volumes. Held-pointer tests on
IDs 12, 24, 41, 42, 72 and 79 require completed display synchronization,
nonempty recomputed sections, interpolated values and changing pixels before
release. These checks use geometric plane cuts, not field isosurfaces.

## Real Data

`OCCTV_VTK_TEST_DATA` optionally points to a VTK `ExternalData/Testing/Data`
directory. External fixtures remain untouched. Registered cases are:

- `quadraticTetra01.vtu`, including curved midside geometry.
- `onePolyhedron.vtu` and `concavePolyhedron.vtu`.
- `nonplanar_quad.vtk` and `Disc_BiQuadraticQuads_0_0.vtu`.
- `cube.vtu`.
- `BoxIncorrectConnectivity.vtp`: import must fail without retaining a mesh.
- `nonWatertightPolyhedron.vtu`: cutting must report invalid topology.

Valid fixture probes must produce nonempty plane, sphere and cylinder sections.
FE-map residuals and field interpolation are checked separately from faceting.

## Geometry and Remaining Limits

Source preparation uses `MeshCutGeometryOptions`: the larger absolute/relative
tolerance, using the deformed nodal diagonal. Defaults are `1e-6` / `1e-3`, face/line
subdivision ceilings `7/12`, and `2000000` source primitives. DRAW queries/edits these
with `scvcutview model geometry [absolute relative faceLimit lineLimit primitives]`.
`scvcutview model geometrymode numerical/preferred/required` selects bounded
numerical geometry, analytic-preferred geometry (the default), or an explicit
analytic requirement. Mode changes invalidate the source epoch; tolerance edits
preserve the chosen mode.

Supported polynomial/rational curves and triangle/quad charts through degree six
are converted from the original VTK map to OCCT Bezier control nets. Triangles
use a degenerate rectangular Duffy chart, including positive rational weights.
Off-grid map checks guard the conversion, and positive-weight control hulls
supplement tessellated BVH bounds. Exact source shapes are cached lazily; displayed
output is still tessellated and stored as floats. Planar quadratic polygons use
their exact quadratic boundary curves; boundary values use the original edge
functions, while interior values retain VTK's world-coordinate MVC convention.
Unsupported representations use the identified numerical fallback in preferred
mode and fail explicitly in required mode. Nonplanar quadratic polygons and
orders beyond the conversion limit are not labelled analytic.
Rational chart conversion preflights finite positive input weights. Normalized
weights at or below `1e-12` do not meet the analytic precision prerequisite and
return unsupported before endpoint sampling; preferred mode can then attempt
bounded numerical geometry. Uniform rescaling by `1e-200`/`1e200` is independently
tested and is distinct from the ratio limit. Numerical fallback must still meet
tessellation/primitive budgets; the extreme-weight fixture is a publication test
with deliberately coarse geometry, not an accuracy reference.
Flat straight-sided high-order faces avoid unnecessary parametric-spacing
subdivision. OCCT cuts this geometric boundary; output coordinates use the
repository's float mesh storage. Surface FE points may differ from displayed
facets within the geometric tolerance. Canonical dyadic curved-face grids avoid
adaptive T-junctions; curved volumes use a common epoch refinement depth.
Refinement/budget failures reject the result instead of returning partial geometry.
This is a configurable sampled numerical policy, not exact/certified arbitrary-order
intersection. Statistics expose float `storageResolution` separately from requested
`geometryTolerance`; tighter tessellation does not change float output storage.
Analytic sphere/cylinder tool faceting is additionally limited to one percent of
the tool radius to preserve small-tool curve/contour quality.

Side output is a boundary mesh, not remeshed volume cells. Measures describe the
selected analytic or tessellated source solid and analytic cutting tool, using
adaptive OCCT area/volume integration. `eval` identifies the source representation;
`stats` exposes `analyticCells`, `numericalCells` and `analyticResidual`.
Conforming planar,
straight-sided internal faces are removed using original face node IDs, edge
connectivity and compatible types, with exactly two opposite-side owners. Joined
hex/voxel/quadratic-hex/Lagrange-hex/polyhedron tests cover this rule. Disconnected
coincident nodes and ambiguous topology are not welded. N02 additionally matches
canonical tessellations and opposite interior points for conforming curved interfaces;
joined quadratic/triquadratic/Lagrange/rational-Bezier hex cases test this foundation.
Mismatching/nonconforming interfaces remain visible and multi-cut union/deduplication
remains open. Invalid sewn solids fail
explicitly instead of silently returning empty contours. A coincident verified
interface section uses the lowest original element index once; configurable
physical/material trace policies remain pending.

The general backend shares prepared original-cell evaluators, geometric bounds
and an OCCT BVH across cuts, portions and drag revisions. Candidate OCCT shapes
are built lazily and reused. `scvcutview model stats` reports cumulative work in
the current source epoch without triggering a cut. Geometry/deformation edits
invalidate it; cut motion and field-only edits retain it. Evaluators remain
single-threaded and retained for the epoch, without hard memory budgets or
eviction. There is no full-frame performance guarantee.

A direct generic-backend run on `hexa.vtk`
(10,648 cells) exceeded a 90-second diagnostic budget before broad-phase work;
after cell-box rejection it completes nonempty plane/sphere/cylinder numerical
probes within that budget. Million-cell/full-frame performance is not accepted.

Native import preserves cell types, arbitrary-length connectivity, polyhedron
faces, degrees, rational weights and numeric PointData/CellData. Composite/time
series native fallback, requested mapping files, portable/WASM equivalent
coverage, ordinary non-cut viewer presentation, and general topology export
remain incomplete. MDB export rejects generic topology before opening output
files to prevent silent data loss. Generic field isosurfaces remain unsupported.
The portable reader currently prints unsupported-cell diagnostics before a successful
native fallback; final diagnostic routing is tracked under B01.

## Recorded Result

N01's recorded 25-case native/DRAW suite passed three consecutive runs per case,
including the 48-cell matrix and external acceptance above. The VTK-disabled
Release build's three native cases also pass three times. The new source-cache
tests include held-pointer reuse and changing cloud pixels before release,
cached-versus-fresh geometry, adjacency and geometry/topology/deformation
invalidation. These results do not
assert that every external VTK dataset or the remaining manager features are complete.
N02's expanded 28-case suite also passes three consecutive runs per case after
the simplex tool-tolerance repairs. The VTK-disabled native/DRAW build passes all
six cases three times. The analytic source increment adds bowed tetrahedra,
warped quadratic/triquadratic/Lagrange/rational-Bezier hex references, exact curves
and planar quadratic polygons. Its expanded 31-case suite passes three consecutive
runs per case; the VTK-disabled build passes all six cases three times. Remaining
curved-family reference evidence and manager tasks are not closed by this result.

The current Windows N02 increment adds independent curve/surface/Lagrange-tetra/
Bezier-tetra references, mixed-order hex 33, original nonlinear FE values and
numerical convergence, plus degree-six/anisotropic and unsupported-precision gates.
VTK-enabled acceptance is 35/35 cases, each passing three times; VTK-disabled is
8/8, each passing three times. The final negative+section held-pointer image case
also passes three additional runs. N02's higher-order field isosurfaces and closure
audit remain open; see [the N02 queue](cut-view-n02-plan.md).

## N02 closure record

N02 closes at the documented bounded scope, including the separate
[original-FE field chart path](cut-view-isosurfaces.md). Full VTK ON 38/38 and
OFF 8/8 passed every case three times; [audit and remaining stage boundaries](cut-view-n02-audit.md).
Earlier partial/milestone records above describe their historical validation.
