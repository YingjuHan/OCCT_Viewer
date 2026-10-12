# N07 explicit physical analysis and automatic cuts

Native analysis uses independently selected original physical FE fields and
provenance. Explicit group IDs, SI factors, input/output frames, moment origin
and signed/absolute-contribution summation are validated. Reports contain
distributed contributions and totals and export CSV in N, Nm, m2 and W.

Volume stress and heat use local analytic plane/sphere/cylinder normals.
Isosurface analysis requires explicitly supplied element normals or a core
callback. Native requests accept serializable element normals and reject
callbacks. Planar shell adapters require a matching support normal and positive
thickness. Composite layers carry separate fields, component layouts, thickness,
offset and frame; offsets contribute to moments. Linear VTK_LINE beams require
an aligned section frame and an explicit area or six integrated resultants.
Unsupported nonplanar shells, composite heat and beam heat fail explicitly.

`NativeCutAnalysis`, owned by `NativeCutView`, computes reports and actual AIS
force/moment/heat arrows before replacing old overlays. Invalid requests preserve
reports and overlays. Edits, abort, delete, detach and close update owned objects.
Coordinated time steps rebind physical/layer fields; remeshed groups, sections and
normal maps require explicit per-step assignments.

Automatic crack-distance and optimization-density cuts use explicit field,
component, threshold, sign, range and visibility mapping. No name inference is
used. Finite/range/tuple validation precedes creating the original-FE isosurface;
negative crack sign exchanges the positive and negative portions. Empty/full
states, density range rejection and failure preservation have independent checks.

Independent references cover tetra force/moment/area, mm/MPa conversion, rotated
field and output frames, signed/absolute sums, original interface stress jumps,
shell thickness, composite offsets, beam stress/resultants, midpoint slice
intervals, plane/sphere/cylinder heat and stress. Native tests cover request JSON,
overlay preservation, drag abort and coordinated physical step bindings.

Final Debug gates: VTK ON **67/67** once (404.56 s), OFF **28/28** each three
times (72.38 s). Affected native/physics/tools checks and repaired physics UI
passed three times. Actual physics inventory **31/31**; tools **38/38**.
Reviewed actual contour/force-arrow pixels and exported CSV. Initial UI test
failure (assuming map order) is retained in `cut-n07-ui-gate.log`; final UI evidence
is `cut-n07-ui-final.log`. N08 performance is separate and remains under development.
