# N09 native session closure

`Viewer::NativeCutSession` is the ordinary native Viewer facade for complete cut
sessions. It owns references to an existing `NativeCutView` and its
`NativeCutTimeline`. Web/Vue/TypeScript/WASM are excluded from this acceptance.
The document format is `occt-cut-native`, version 3. Definition-only JSON v7
remains a separate API and is not a full session.

## Persisted state and source identity

Sessions preserve cut definitions and independent display/physical traces,
active geometry/field/deformation step, scale, geometry/refinement and cache
policies, common/per-cut/portion styles, contour ranges/legend, vectors and
orientation, datums/axes, material anchors/following cuts, per-step recovery and
trace mappings, playback, physical requests/per-step assignments/automatic cuts,
annotations, CAD section/global clipping and camera.

Save freezes the supplied local VTK root and all referenced files into a unique
bundle under `<session-name>.assets`. Each declared file carries its byte size
and streaming SHA-256 digest. The active mesh fingerprint covers original node
coordinates, topology/connectivity, general-cell faces/degrees/rational weights,
raw field values/mappings and component/time metadata. All source-series
snapshots and timestamps are compared to the live timeline before publication.
The initial stable-series aggregate field view is checked against its imported
root while the saved active fingerprint identifies the step snapshot.

CAD requires an explicit BREP source file that agrees with the independent live
CAD snapshot. Its canonical digest includes geometry/topology, excluding
transient Free/Modified/Checked flags and triangulations. Section operations do
not change the preserved snapshot. No shapes, pointers, AIS handles or cache
contents are embedded in JSON.

The JSON and its assets folder must move together. Relative paths are resolved
and canonical-contained; every composite/series reference must identify a
declared, digested file. File, count, decoded-input and import limits reuse the
N04 resource contract. The session JSON limit is 32 MiB; nesting is limited to
64 and duplicate keys are rejected. Unknown versions, duplicate IDs/mappings,
missing sources/fields, digest/tuple/component mismatches, invalid geometry,
camera, anchors, physical groups/units/sections and inconsistent automatic cuts
reject before live publication.

## Publication and failure behavior

Save creates and reloads a temporary complete session for validation, then
replaces the target JSON atomically with Windows `MoveFileExW` (native rename on
other platforms). Validation, staging or CAD mismatch failures preserve the
previous JSON bytes. Failed attempts remove only their own newly staged bundle.
Successful repeated saves retain older unique bundles; existing snapshots can
continue to reference them. Asset garbage collection is not performed.

Load resolves source identities, validates every binding and prepares AIS state
on a staged owner before adoption. Current and candidate geometry reservations
share the N08 ledger, including definition validators, save-preview validation
and per-step physical validation. Budget refusal preserves live state and
releases temporary leases. The limits are logical geometry/cache envelopes;
they are not exact process/GPU memory ceilings.

Failed loads preserve the old display, definitions, generation, drag and policy.
Successful loads replace owned display/analysis/CAD/clip objects, preserve AIS
selection modes, restore the camera, cancel the previous drag and send Abort
then Update after timeline/source bindings have been committed. Numeric edit
rollback restores the already validated typed definition without another JSON
validator when a new source epoch cannot fit alongside the published geometry.

Versions 1 and 2 of this full-session envelope migrate absent additions to native
defaults. Nested definition versions 1, 2 and 3 migrate independently. An old
definition-only document does not gain missing full-session source identity by
changing its version number.

## Native and DRAW usage

```cpp
Viewer::NativeCutView view(mesh);
Viewer::NativeCutTimeline timeline(view);
Viewer::NativeCutSession session(view, timeline);
// The source files must describe the unchanged raw mesh/fields in the timeline.
session.Save("D:/models/example.json", "D:/models/source.vtm.series");
session.Load("D:/models/example.json");
// Methods return bool; failures are reported by view.LastError().
```

```tcl
scvcutview model sessionsave D:/models/example.json
# Supply the CAD BREP source when the model owns a CAD section:
scvcutview model sessionsave D:/models/example.json D:/models/source.brep
scvcutview model sessionload D:/models/example.json
scvcutview model sessioninfo
scvcutsession model
```

DRAW keeps the existing ViewerTest display map, model field inventory and source
bindings coordinated after restoration. The session panel has six actual
controls; tools has 40, main manager 48, time 20, physics 31 and cache seven.
Model `clear` closes the panel and cancels its polling timer.

## Acceptance evidence

`NativeSessionTests.cpp` covers standard SHA-256 vectors (empty, abc, multiblock,
million-a and chunked input), repeated save/load, field/style/vector/axes/probe
annotation/heat/CAD/disabled-clip round trips, old handles, drag cancellation,
14 malformed metadata classes and duplicate keys, tight-budget numeric rollback
and save refusal, automatic-cut failure preservation, migration, relocation and
file limits. Generated stable and
remeshed VTM series cover deformation, following frames, per-step regions/traces,
physical groups, automatic density cuts, playback and subsequent step switching.
Nine invalid series mappings and an invalid low-level mapping cannot publish.

`draw_cut_view_session` operates all six real Tk controls and file dialogs,
including CAD browse, save/load, cancellations and invalid-load errors. The
before/restored image checksums match; invalid and cancelled operations preserve
pixels, definitions and native event state. Windows automation uses the real Tk
dialog backend; Windows shell-native file-dialog automation is not covered.
The tools/manager integration and model-clear panel lifecycle are also exercised.

Final restored/rejected/cancelled session images were reviewed. Earlier failed
fixture assumptions and interrupted pre-rebuild runs remain in their build logs;
only completed final gates establish closure.

## Final rebuilt validation

| Configuration | Result | Elapsed | Log |
| --- | --- | --- | --- |
| VTK ON | 71/71, each three consecutive runs | 1421.50 s | `build/mingw/cut-n05-n09-final-gate.log` |
| VTK OFF | 30/30, each three consecutive runs | 129.16 s | `build/mingw-no-vtk/cut-n05-n09-final-save-gate.log` |
| headless | 17/17, each three consecutive runs | 91.21 s | `build/mingw-import-headless/cut-n08-n09-budget-gate.log` |
| Release affected | 6/6, each three consecutive runs | 92.66 s | `build/mingw-release/cut-n05-n09-final-gate.log` |

The full suites above use binaries rebuilt after the shared-budget definition-validator,
per-step physical-validator and save-preview fixes. Two later Viewer-only fixes
share drag-baseline accounting/options and directly remove an automatic cut after
failed preparation. Those changes and their refusal/preservation regressions were
rebuilt and verified separately on the final source:

| Configuration | Result | Elapsed | Log |
| --- | --- | --- | --- |
| VTK ON affected | 11/11, each three consecutive runs | 48.58 s | `build/mingw/cut-n08-n09-rollback-final.log` |
| VTK OFF affected | 6/6, each three consecutive runs | 7.00 s | `build/mingw-no-vtk/cut-n08-n09-rollback-final.log` |
| Release Viewer affected | 4/4, each three consecutive runs | 6.80 s | `build/mingw-release/cut-n08-n09-rollback-final.log` |

The first numeric-edit fixture
was rejected as evidence because it reused an existing reservation and did not
exercise budget refusal; its corrected source-epoch fixture passes. Initial failed
and interrupted logs remain separate. These regressions do not replace the N08
million-cell performance evidence or broaden its workload.
