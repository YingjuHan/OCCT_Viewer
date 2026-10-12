# Native presentation OR union

`MeshCutViewManager::EvaluatePresentation(field)` builds a separately owned
display artifact. `Evaluate(id)`, `EvaluatePhysical(id)` and `EvaluateVisible()`
remain independent numerical cuts. Union branches carry `PresentationOnly` and
physical integration rejects them. No presentation area/volume is substituted
for an independent cut's physical result.

The retained negative/positive regions are Boolean-fused within each original
source cell. The exposed boundary is split by OCCT operations; coincident
triangles alone are not the union algorithm. A reciprocal, trusted original
topology interface permits subtraction of the neighboring retained region.
Disconnected cells keep separate boundaries even at equal coordinates.

Ownership follows Boolean Modified/Generated history, preferring visible On
caps, then lexical cut ID. Each displayed triangle, segment or point has cut,
portion, original source element and cap provenance. Generated nodal values
map through the original FE evaluator; display trace selection remains separate
from physical trace selection. Isosurfaces retain their original scalar binding
and numerical refinement contract.

Single cuts use the same retained-region contract as multiple cuts. In mask 7,
the two sides fill the source and the internal On sheet disappears from display.
Its independent numerical section remains available. An On-only sheet remains
visible, including intersecting sheets, but covered internal sheets are removed.

`InteractiveCaeCutView::Synchronize()` retains the artifact lifetime and swaps
completed presentations atomically. The artifact detaches only its own geometry
roots to break parent/child Handle cycles.

## Acceptance queue

| Item | Acceptance | State |
| --- | --- | --- |
| N03.4a | Actual retained-region OR, identical/crossing cuts, 64 mask pairs, stable cut ownership, trusted/disconnected source boundaries, renderer/integration separation | Verified: ON focused 7/7 and OFF full 13/13, each three times (29.25 / 26.67 s) |
| N03.4b | FE interpolation, material jumps, cap ownership, touching cuts, deformation, mixed dimensions and geometry/iso cases | Verified: ON focused 7/7 and OFF full 13/13, each three times (286.04 / 28.31 s) |
| N03.4c | DRAW presentation diagnostics and transparent cloud comparisons | Active |
| N03.5 | Full repeated ON/OFF native/DRAW/visual suites and scope audit | Queued |

Evidence: `build/mingw/cut-n03-union-foundation-tests.log` and its OFF counterpart.
The 64-pair oracle uses rectangular occupancy, volume, exposed perimeter and
uncovered sheet area, independently of OCCT and the numerical cut backend.
This foundation gate does not close N03.4 or establish the N08 performance target.

N03.4b adds oblique/touching/separated planes, overlapping/touching spheres,
cylinder ownership, surface caps, deformation, exact material region selection,
disconnected element-node jumps, original quadratic FE interpolation, mixed
high-order iso/plane regions, line endpoints, polyvertices and empty cells.
Independent section area, force, moment and heat flow are unchanged by display OR.
The iso replacement partition first cancels equal generated topology faces within
one original cell and sews a closed boundary; cross-cut overlaps then undergo
actual Boolean OR. A covered trusted interface sheet is removed from display.
Native neighbor traces discard tiny off-interface inverse weights before mapping
the neighboring element-node tuple. Logs: `cut-n03-union-fields-tests.log` in both
build configurations. These are bounded correctness checks, not full-frame timings.
