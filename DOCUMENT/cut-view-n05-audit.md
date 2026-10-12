# N05 native Viewer closure

N05 covers datums/local edits/reference axes, common and per-cut/portion styles,
contour ranges and HUD legends, original-FE probes and real AIS node/element
picking, vector/orientation plots, annotations, independent CAD sections/global
clipping, and the native lifecycle/event/error facade. Web/WASM remains excluded.

`NativeCutView` owns its cut display, guides/manipulators, annotations, CAD section
and clipping plane. Ordinary caller CAD objects and clipping planes retain their
owners. Removed viewport refresh detaches owned objects; closed models reject
operations. An independent clipping edit cancels CAD clip synchronization.
Sphere/cylinder CAD tools are infinite analytic surfaces, independent of FE bounds.

Probe values, source element and original node weights use the selected physical
FE trace. Display smoothing never becomes a physical probe. Selected owners expire
after synchronization. Vector orientation normalizes a display copy and preserves
the original field. Explicit field name/mapping/component/step binding validates
tuple counts and reports missing/ambiguous bindings through `LastError()`.

The existing Tcl manager opens Viewer Tools; its real widget tests cover 48 main
controls and 36 tools controls. Rendered evidence includes the 2D HUD legend,
original-FE cloud, normalized arrows, annotation and CAD wire. Real mouse picking
checks FE values and provenance after native projection.

Validation logs live under the existing build configurations. The earlier
`cut-n05-closure-tests.log` is retained: ON 60/61 passed; tools failed when a new
Tcl command was added during an old-binary run. Final evidence must come from the
recompiled binaries and `cut-n05-final-gate.log`, not that initial run.

Final gate: VTK ON **61/61** (1196.43 s), OFF **24/24** (59.78 s), headless import/reference **2/2** (0.37 s), each case three consecutive runs. Actual Tk inventory **48+36**; final screenshots reviewed. See [the continuous queue](cut-view-n05-n09.md).
