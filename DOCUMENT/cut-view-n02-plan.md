# N02 completion queue

N02 remains **partial**. Finish the following gates before starting new N03 features.
The native track excludes Web/Vue/TypeScript/WASM. Existing N03 recovery work is
retained and remains in the regression suite.

| Order | Task | Acceptance | Current status |
| --- | --- | --- | --- |
| N02.1 | Remaining curved-family independent references | Curves, triangular/quadrilateral surfaces, Lagrange/Bezier tetrahedra and mixed-order hex: independent measures, original FE values, provenance, moving-cut source reuse; native and DRAW checks | Verified for the listed references; each case passed three times |
| N02.2 | Analytic support and precision boundary | Degree-six and anisotropic charts, positive rational weight scaling, unsupported degree/precision fallback, required-mode atomic rejection; document sampled geometry and float storage separately | Verified for the listed bounds and repair; each case passed three times |
| N02.3 | Higher-order/generic field isosurfaces | Original cell FE evaluation; below/on/above and all masks; enclosed/nonmonotone field crossings; numerical convergence and explicit limits; DRAW field binding and held-threshold synchronization | Queued, next implementation |
| N02.4 | N02 closure audit | Recheck remaining family/interface/precision gaps, deformed geometry and FE mapping, both VTK configurations, repeated native/DRAW/visual suite; identify remaining G02/G03 limits individually | Queued |
| N03 | Recovery, physical traces and presentation OR union | Resume the N03 queue only after the N02 gates pass; N03's existing numerical/metadata foundation is partial | Deferred |

## N02.1 reference definition

`cut_reference` uses independent polynomial/Bernstein moment formulas, not VTK
evaluation or cut outputs as the expected result. Geometry coefficients and result
coefficients are treated separately: sampling a quadratic at Bernstein control
locations does not make its coefficients interpolate those samples. The raw
`u_squared` data is therefore checked against its binomial moments for Bezier
families and against `u^2` for interpolatory families.

Curves: 21/35/68/75. Surfaces: 22/23/28/30/34/69/70/76/77. Additional
tetrahedra: 71/78. Four rational curve/surface/tetra cases complement the earlier
rational hex/wedge references. `cut_analytic` additionally tests type 33. These
extend the earlier bowed-tetra, hex, wedge, pyramid and quadratic-polygon gates;
they do not prove every admissible high-order geometry is valid.

Generated VTU fixtures are build outputs. DRAW consumes the same source fixtures
but computes its own reference measures and field values, with geometric plane
constraints, CellData/line provenance, pre-release presentation revisions, pixel
changes and abort rollback.

## N02.2 support and precision

Polynomial/rational triangle and quad charts and curves through degree six can
be converted to OCCT Bezier boundaries. Conversion is additionally guarded by
positive control weights and off-grid map residuals. Degree alone is not a
guarantee of conversion. The general numerical geometry is sampled and bounded,
not a certified arbitrary-order intersection.

The analytic converter normalizes rational weights by their maximum. Every input
weight must be finite and positive. A normalized weight at or below `1e-12`
does not meet the analytic control-net precision prerequisite and is rejected
before endpoint sampling. Preferred mode can then use its explicit numerical
fallback; required mode rejects the representation without publishing a source.
Numerical fallback still has to satisfy tessellation and primitive limits.

An intentionally coarse extreme-weight fixture verifies fallback publication,
not geometric accuracy. Uniform weight rescaling by `1e-200` and `1e200` must
preserve the independent geometry and original FE reference values.

Output positions remain floats. The earlier `storageResolution` test and geometry
tolerance statistics continue to expose that limit; refining the source cannot
recover detail already lost in input/output float coordinates.

## N02.3 implementation requirements

- Keep plane/sphere/cylinder sections as geometric intersections. Field
  isosurfaces are a separate shape and do not replace those paths.
- Retain the existing first-order path. Extend supported generic/high-order
  cells with original FE evaluation and a controlled numerical representation.
- Give field residual/refinement limits their own meaning; a geometric tolerance
  is not a tolerance in arbitrary field units. Check nonmonotone and enclosed
  crossings even when all corner samples have the same sign.
- Preserve original-element provenance and FE weights, field/component/step
  bindings, deformation, constant/projected CellData and all eight masks.
- Threshold/field edits must invalidate the relevant results; abort and failed
  preparation must preserve valid displayed state. Reject unsupported input
  explicitly, including the VTK-disabled configuration.
- Require independent polynomial/rational field references and convergence,
  native and DRAW tests, real held-threshold pixel/revision checks and definition
  migration before marking this task complete.

## Verification

Build logs and test results for this increment are under `build/mingw/cut-n02-*`.
VTK-enabled `build/mingw`: **35/35**, each repeated three times (105 passes),
635.41 seconds. VTK-disabled `build/mingw-no-vtk`: **8/8**, each repeated three
times (24 passes), 7.75 seconds. The VTK build remains configured ON/visual ON.

The final visual script displays negative+section portions so the actual clipped
cloud boundary changes during motion. After that test-only adjustment,
`draw_cut_view_references_visual` passed three more times (21.55 seconds).
Both the initial and held images were inspected. The source fixture setup was
excluded for this additional filtered run because its unchanged generated sources
had already passed the full repeated suite.

Logs: `build/mingw/cut-n02-build.log`, `cut-n02-vtk-tests.log`,
`cut-n02-reference-visual-tests.log`; and `build/mingw-no-vtk/cut-n02-build.log`,
`cut-n02-tests.log`. Missing historical external fixtures are not substituted.
N02.3 and N02.4 are still open; this is not completion of N02/G03.
