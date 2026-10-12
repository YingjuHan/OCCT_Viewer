# N02 bounded closure audit

The audit covers the planned N02 family references, analytic conversion/precision
boundaries, original-FE numerical isosurfaces and their native/DRAW presentation
contracts. It does not close N03 recovery/physical traces/OR union or N04–N09.
N02.4 is verified at this bounded scope. VTK ON: 38/38, each three times
(114 passes), 610.23 seconds. VTK OFF: 8/8, each three times (24 passes),
6.24 seconds. Logs: `build/mingw/cut-n02-closure-tests.log` and
`build/mingw-no-vtk/cut-n02-closure-tests.log`. Current control inventory remains
32/32; initial/held high-order iso images were inspected.

| Area | Evidence and supported boundary | Disposition |
| --- | --- | --- |
| Geometry families | Existing 48-concrete-type geometric matrix; independent curved curve/surface/tetra/hex/wedge/pyramid references and nonlinear FE checks | Covered at the documented fixture scope |
| Analytic conversion | Polynomial/rational charts through degree six, anisotropic orders, positive controls and off-grid residuals; unsupported degree/normalized-weight precision uses preferred fallback or required rejection | Explicit bounded support |
| Numerical precision | Independent convergence, absolute/relative model tolerance, primitive/subdivision limits, original and output float resolution | Sampled numerical accuracy; no arbitrary-order certificate |
| Scalar FE cuts | 37 chart types, six native second-order families; independent nonmonotone roots, enclosed circle/sphere, polynomial/rational reference equations, masks and field/element provenance | Covered by `cut_iso` and DRAW numerical checks |
| Scalar limits | Separate field units; midpoint/centroid/facet checks; explicit budget/residual errors and constant-level volume plateau rejection; unsupported MVC/strip/point chart inputs | Documented in `cut-view-isosurfaces.md` |
| Shared interfaces | Geometric source grid/adjacency cases remain in existing regressions; isosurface charts conform within an element and retain separate original-element sources | Mismatching interfaces remain separate; trace policy/union are N03 |
| Deformation | Original node modifiers applied before FE mapping; curved and nonuniformly deformed scalar references, reset and existing geometric/guide cases | Covered at the tested maps, not folded-element certification |
| Binding/lifecycle | Fixed field/component/step; displayed field independent; equal-weight cutting-scalar projection independent of display recovery; explicit mutation invalidation, held updates, Escape rollback, failed display publication and version 4 migration | Native and real Tk/visual acceptance |
| VTK disabled | Native first-order behavior retained; high-order/generic bindings rejected; all affected native/DRAW targets rebuild | 8/8, each three times, 6.24 seconds |
| Performance | Source epochs, BVH/lazy preparation remain; numerical field cuts rebuild their representation at threshold edits | Large-model caching/full-frame/memory budgets remain N08 |

Structural inspection used codebase-memory search/trace/snippets and path
coverage checks. The index reports partial parsing and metadata changes on
Handle/template-heavy files, and some call edges are misresolved. Material
paths were verified directly in source; graph zero results were not used as
absence or complete-impact evidence. Scope includes `MeshCutViewManager`,
`MeshVtkCell`, `MeshVtkAnalytic`, `MeshVtkCutSource`, geometric and scalar VTK
backends, `InteractiveCaeCutView`, DRAW commands and the affected native/Tcl tests.

The user-modified legacy VTK 7.1.1 checkout is untouched. The installed 9.5.2
SDK/MinGW patch remains the configured dependency. Six official external fixtures
are available with prior SHA512 evidence; the two missing historical fixtures
are not counted as reproduced tests. No Web/Vue/TypeScript/WASM work is included.

Existing N03 recovery foundation remains a regression requirement. N03.1 starts
with native recovery widgets and exact option persistence; tensor conventions,
physical traces and presentation OR/cap ownership follow the separate N03 queue.
