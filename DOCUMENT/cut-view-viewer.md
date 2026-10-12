# Ordinary Native Viewer VTK Cells

N04.2 adds `MeshPartGeneralCells` to the ordinary scalar presentation used by
`Viewer::InteractiveCaeMeshManager` and `scvreadmesh`. Original external VTK cells
produce points, segments and surface/volume boundary triangles. This uses the
prepared original-cell evaluator and trusted internal-face filtering; it does not
create a Cut View object or require a cut definition.

Every primitive carries its original indexed element and an original-FE weight
set for each vertex. CellData uses the original element tuple; nodal and
element-node values use those weights. No centroid-to-node field substitution is
performed. Field changes invalidate the new presentation part, and element-type
and presentation-mode changes rebuild its arrays. The existing 0D/1D/face/volume
flags control visibility. Point markers use the ordinary point color and size;
pure point datasets therefore remain visible on a white background.

The optional native VTK backend supplies the general cells. With VTK disabled,
the existing portable native types retain their display path and unsupported
general-cell imports fail explicitly. The concrete-cell and geometric/FE support
boundaries remain those in [the cell matrix](vtk-cell-support.md). Scalar colors
are sampled at tessellated primitive vertices. Geometry approximation is governed
by the prepared source's existing tolerance and primitive ceiling; this is not
an exact arbitrary-order surface renderer or a million-cell frame-budget result.

Ordinary point/line/surface/volume display and scalar mapping are the N04 gate.
Generic result picking, annotations, dedicated vector plots, full per-cell style
options and coordinated remeshing animation belong to N05/N06. Native mesh paths
and the independent physical Cut View evaluations retain their existing behavior.

`vtk_import` checks Legacy/XML polyline connectivity, mixed-dimensional original
CellData and affine nodal weights, a quadratic field on a bowed quadratic edge,
pure polyvertices and general tetra/voxel boundaries. `draw_vtk_import_visual`
uses ordinary `scvreadmesh`, requires visible pixels for every fixture, and
requires pixel changes on nodal/CellData switching. Reviewed images are under
`build/mingw/test/native/draw-import/ordinary-*`. The initial full run's pure-point
white-on-white failure is preserved in `cut-n04-viewer-first-full.log`; the repaired
run and focused repeated checks are recorded in the N04 plan and handoff.

N04.8 final native closure: Final VTK ON **58/58**, OFF **22/22**, every case three times (174 / 66 executions; 547.47 / 41.50 seconds). DRAW/Viewer-disabled import/reference tests **2/2** each pass three times (0.52 seconds).
See [the audit](cut-view-n04-audit.md) for current support boundaries and reproduction.
