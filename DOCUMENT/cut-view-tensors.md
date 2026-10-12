# N03.2 Tensor Display Contract

`MeshCutTensorOptions` explicitly selects a symmetric tensor measure and component
layout. Native display applies it only to fields marked `MeshNVecPropertyType::Tensor`;
independent scalar sets and vectors keep their original component binding. The
standalone `DeriveCutTensorField()` API also accepts explicitly supplied scalar sets.
The original field and fixed isosurface cutting component are never rewritten.

## Layout and Shear

| Layout | Component order |
| --- | --- |
| `xx-yy-zz-xy-xz-yz` | Six independent symmetric entries in the stated order |
| `xx-yy-zz-xy-yz-xz` | Six entries with the last two shear components exchanged |
| `row-major` | Nine entries: xx, xy, xz, yx, yy, yz, zx, zy, zz |
| `column-major` | Nine entries: xx, yx, zx, xy, yy, zy, xz, yz, zz |

`tensor` shear uses the supplied off-diagonal tensor entries. `engineering` shear
halves off-diagonal entries before deriving a measure. This affects display only;
physical integration still uses the supplied original field and explicit component
map. It does not infer strain/stress meaning, local coordinate systems or unit
conversion. Components must share display units, storage, mapping and tuple counts.
Element-node fields must have matching trace keys in every component.

Nine-component input must be symmetric within 64 double epsilons after scaling by
its largest absolute entry. Roundoff within that tolerance is symmetrized; other
matrices are rejected. Arbitrary nonsymmetric tensors, complex tensors and mixed
coordinate frames are outside this contract. Standard VTK `Tensors` attributes
require nine components; native six-component tensors are explicitly constructed
through the C++ API. No reader constraint has been relaxed to accept invalid VTK.

## Measures and Numerical Range

- Mises: square root of half the sum of squared normal differences plus three
  times the sum of squared tensor shear entries.
- Pressure: negative one-third of the trace (positive for hydrostatic compression).
- Maximum/middle/minimum principal: eigenvalues of the symmetric matrix, descending.
- Tresca: maximum principal minus minimum principal (twice maximum shear).

Calculations normalize the matrix before differences, norms and Jacobi rotations,
then restore its scale. This avoids intermediate overflow for representable finite
measures, including large hydrostatic and very small tensors. An unrepresentable
final result throws; it is never clamped to a plausible finite value. Jacobi
convergence is relative to the largest matrix entry: very small principal values
in a severely ill-conditioned tensor do not have a relative-accuracy guarantee.
This is double-precision display evaluation, not arbitrary-precision spectral
analysis. Any NaN
component yields a NaN derived tuple. Infinity is rejected even if another
component is NaN. Recovery retains missing source traces rather than filling them
from a finite neighbour. Finite component recovery remains component-wise.

## Recovery and Interpolation Order

The before-order flag has these explicit meanings for projected elemental tensors:

1. Before: derive the selected measure on original elemental/element-node tuples,
   then apply region/threshold recovery to that scalar.
2. After: recover the original tensor components using the same policies, then
   derive the selected measure on the recovered source tuples.

Both paths then map the resulting scalar onto cut samples with original FE weights.
The after path does **not** recompute a nonlinear measure from tensor components
interpolated at each new cut point. Thus it is a display reconstruction, not an
exact nonlinear FE invariant isosurface or a replacement physical field. Threshold
tests act on the scalar in the before path and separately on components in the
after path; region boundaries and rejected averaging keep separate element traces.
Nodal and constant elemental display derive directly on source tuples without
recovery. Lines, surfaces and volume sections use the same scalar binding.

`MeshCutResult::TensorDerived` tells renderers/DRAW to use derived component zero;
the original selected raw index is retained for returning to component display.
`PhysicalFields` remain original components. `EvaluatePhysical()` bypasses display
recovery and measures entirely; DRAW freebody/heatflow use it so an unrelated or
invalid display convention cannot block original physical integration.

## Native Controls and Definitions

The Tk manager has measure, layout and shear menus, with the existing invariant
before-averaging checkbox and region/threshold controls. `components` restores
raw component display. These options apply to tensor fields only. A layout mismatch
is an error, not an automatic reinterpretation. DRAW preflights active tensor input
before changing settings; failed presentation synchronization preserves existing
presentations.

```tcl
scvcutview model tensor mises row-major tensor
scvcutview model recovery 100 1 none
scvcutview model tensor maxprincipal row-major engineering
scvcutview model tensor components row-major tensor
```

Definition JSON v6 persists the tensor measure/layout/shear within recovery options.
v1-v5 restore with default tensor component display; v5 retains its recovery values.
Invalid enums/types are rejected in staging. Definitions still do not contain the
complete display-field/session binding; complete native sessions remain N09.

## Acceptance

`cut_tensor` checks analytical uniaxial, hydrostatic, pure-shear and repeated roots;
30 rotations of independently known eigenvalues across all four layouts; signed
principal/pressure values, extreme scales, missing/infinite/unrepresentable data;
region/order references, all masks, elemental-node and surface-line traces;
original force integration, independent physical evaluation, metadata, v5/v6
migration and presentation failure atomicity. DRAW imports a standard nine-entry
tensor fixture, verifies each measure/convention and checks original integration.
The real manager inventory is **39/39**, with both interactive `-i` and batch
`-b` manager startup covered. Its added layout uses `pack`, avoiding the Tk/DRAW
`grid` alias difference between those startup modes. The visual test compares two scalar
gradients without positioning aids. The control test captures both manager layouts.

Stage evidence: full VTK ON **41/41** and OFF **10/10**, every case three times;
`cut-n03-tensor-tests.log` in each build. Reviewed images are
`draw-tensor/tensor_cloud.png`, `tensor_before.png`,
`draw-controls/tensor_manager.png` and `manager_controls.png`. Physical interface trace choices and
presentation OR union/cap ownership are still N03.3/N03.4 work.
