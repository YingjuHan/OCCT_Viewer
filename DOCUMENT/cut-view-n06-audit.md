# N06 native time and slicing closure

`NativeCutTimeline` prepares geometry, contour and vector fields, deformation,
datums, definitions, CAD sections and clipping before publishing a new step.
Missing fields, changed schemas, invalid anchors and unmapped remesh regions
preserve the live display, active drag and generation. Successful publication
releases old handles and annotations and retains AIS selection modes.

Material frames use three explicitly mapped nodes. Reference configuration uses
the first snapshot and key -1; updated configuration uses per-step anchors and
the actual deformed coordinates. Following cuts retain their local definition.
Animation supports uniform periods and strictly increasing source timestamps,
pause, restart, loop, end and failed-frame pause on the Viewer thread.

`MeshCutSlicing` supplies regular spacing, bounded ranges, rotational spacing and
piecewise-linear paths, with explicit midpoint interval bounds and a 10000-slice
limit. No inferred material orientation or topology correspondence is used.

The actual Tk time panel has 20 controls. The Viewer Tools panel has 37 controls
including its time-panel entry. Step changes refresh field and vector inventories
and preserve the selected scalar component. Closing a manager cancels its timer.

The time acceptance exposed and repaired fixed-range texture normalization for
constant fields. Primitive UVs retain the source domain; the color mapper converts
that domain to the common contour range without changing FE values. Legends use
the requested intervals while contour textures retain continuous resolution.
Reviewed step images show different colors for values 7 and 17 in range 0..20.

Validation evidence is in `build/mingw/cut-n06-final-gate.log`,
`cut-n06-final-focused.log` and the OFF `cut-n06-final-gate.log`. Initial failures
and their repair logs are retained. Geometry references account for float source
coordinates (2e-7 area tolerance); frame-only checks retain 1e-9 tolerance.

Final evidence: ON **64/64** once (346.63 s), affected native/time/tensor cases
**7/7** each three times (16.72 s, including fixture setup), OFF **26/26** each
three times (71.30 s). Time/tools widget inventories **20/37** are complete.
