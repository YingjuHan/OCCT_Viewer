# N03 bounded closure audit

N03.1-N03.5 are verified at this bounded support scope. Final VTK ON 48/48 and
OFF 14/14 each pass three consecutive runs (144 / 42 successful executions,
538.76 / 47.00 seconds). The stage covers native recovery controls,
symmetric tensor measures, independent interface traces and actual presentation
OR union. Complete native sessions and N04-N09 remain separate work.

## Evidence and contracts

| Area | Evidence | Supported boundary |
| --- | --- | --- |
| Recovery | `cut_recovery`, native/DRAW recovery cases and real Tk edits | Explicit element regions, threshold/order, all components, metadata, finite means and missing traces; original physical values remain independent |
| Tensor | `cut_tensor`, tensor DRAW/visual and controls | Real symmetric six/nine layouts, tensor/engineering shear, Mises/pressure/principal/Tresca and before/after recovery; nonlinear measure precedes scalar FE mapping |
| Interface traces | `cut_trace_binding`, `cut_interface`, DRAW/visual trace cases | Reciprocal trusted topology, both original sources/FE weights, independent lowest/negative/positive/region selection, deformation and all masks |
| Definition persistence | Native JSON rejection/migration and Tk Save/Load/error/cancel cases | JSON v7, exact signed int64 region IDs, legacy defaults and atomic staging; definitions are not full sessions |
| Presentation OR | `cut_union`, DRAW union and transparent visual comparisons | Actual OCCT retained-region OR per original cell, 64 mask pairs, trusted neighbor boundaries, stable exposed-cap/source provenance, touching/contained/overlapping regions and mixed dimensions |
| Field/provenance | Independent FE and material references in `cut_union` | Original nodal/element-node mappings, deformation, representative quadratic type 25 and high-order iso/plane combinations; no coordinate welding across disconnected sources |
| Numerical isolation | `EvaluatePhysical`, integration rejection and unchanged per-cut area/force/moment/heat flow assertions | Presentation branches cannot supply physical integrals; numerical `Evaluate(id)` remains independent |
| Native UI | Full controls gate and manager images | 47/47 inventoried Tk controls, invalid edits and independent display/physical policies |

## Explicit limits retained

- VTK ON uses the existing geometric 48-concrete-cell matrix and the bounded
  high-order FE isosurface contract. Representative quadratic union checks do
  not establish exact arbitrary-order intersections or certified global errors.
- VTK OFF retains native first-order point/triangle/quad/tetra/hexa/prism/pyramid
  FE mapping for supported planar cuts; native curved-tool cuts are limited to
  triangle/tetra sources. Generic/high-order chart geometry still requires VTK; unsupported
  polygon/polyhedron/high-order union FE inverse requests fail explicitly.
  Native nonplanar quad/source BRep construction also retains its geometric limits.
- Trusted reciprocal topology controls interface subtraction and selection.
  Coincident disconnected or mismatching cells keep separate boundaries; ambiguous
  explicit trace choices reject rather than infer a material from coordinates.
- Physical integration remains supplied volume-field planar-section integration.
  Shell/beam/composite section mappings and nonplanar local-normal mapping remain
  N07; unsupported requests are rejected.
- Transparent variable-color image comparisons use unlit weighted OIT, fixing
  lighting and primitive-order effects. Constant CellData also verifies ordinary
  alpha blending. OIT-off variable-color blending remains order dependent.
- Definition JSON does not persist complete source/field/style/deformation/session
  state. Per-cut style overrides/legends/probes remain N05; full sessions remain N09.
- CPU/BRep union fixture timings do not establish the million-cell full-frame
  15 FPS/p95 <=67 ms target. The old geometry/field-only benchmark excludes the
  current union/render pipeline; frame/memory/cache acceptance remains N08.

## Verification procedure

Both configured MinGW builds finish before their tests run. The final gate is:

```powershell
ctest --test-dir build/mingw --output-on-failure --repeat until-fail:3 -j 4
ctest --test-dir build/mingw-no-vtk --output-on-failure --repeat until-fail:3 -j 4
```

Logs: `build/mingw/cut-n03-closure-tests.log` and
`build/mingw-no-vtk/cut-n03-closure-tests.log`. The final OFF gate has passed
14/14 cases, 42 executions in 47.00 seconds; ON has passed 48/48 cases,
144 executions in 538.76 seconds. Every case has exactly three successful runs.
Union-focused gates and images are recorded in [the union contract](cut-view-union.md).
Final image review covers the complete trace manager (including Close), tensor
gradients, opposite trace colors, union/crossing/full-mask opacity, geometric
sphere/cylinder On-only caps and changed hexahedron held-drag clouds.

The first full ON gate found missing triangulations in the VTK-cell and quadric
visual cases. A native warm-source/all-mask sphere oracle reproduced mask 2:
converting float pole triangles back into BRep produced a degenerate face.
Curved On-only cap seeds now keep the original OCCT cutting surface. The same
oracle caught the full-mask compsolid's shared internal spherical face; boundary
assembly now excludes shared volume faces by topology within one original cell.
Plane boundary-coplanar sheets retain their numerical section path, preserving
both disconnected material sources. Warm masks, duplicate/intersecting On-only
spheres, cap ownership and OFF native simplex support remain explicit regressions.
The original failed gate is retained as `cut-n03-closure-initial-tests.log`.

Targeted structural verification uses codebase-memory search/trace/snippets and
path coverage, followed by direct source reads for partial Handle/template ranges.
The inspected paths are manager evaluation/definition staging, trace mapping,
integration rejection, union ownership/FE output and native AIS synchronization.
Graph zero callers or missing edges are not treated as exhaustive evidence.
N03.4c `eba2a37` and source repair `3a1fd44` are committed separately. After the
source repair commit the index was refreshed to 34,418 nodes / 127,831 edges;
518 partial and 9 unusable parse records still require source fallback.

Web/Vue/TypeScript/WASM work is excluded. The installed VTK 9.5.2 MinGW SDK is
reused; this stage requires no VTK checkout/build change.
