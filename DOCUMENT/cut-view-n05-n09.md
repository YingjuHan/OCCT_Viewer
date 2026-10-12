# Native N05–N09 execution

N01–N04 are the verified baseline at `4f135da`. Work continues in dependency order;
each completed increment gets its own validation evidence, commit and explicit
codebase-memory refresh. The web/WASM track remains excluded.

| Stage | Work and acceptance | Status |
| --- | --- | --- |
| N05 | Datums/editing, styles/legend, original-FE picking/probes, vector/annotation display, CAD/clipping, native lifecycle API and real Tk controls | Verified: ON 61/61, OFF 24/24, headless 2/2 x3; 48+36 controls |
| N06 | Material frame, atomic geometry/field/deformation steps, animation and slicing with remesh/failure/abort tests | In progress |
| N07 | Explicit groups/units/frames and shell/beam/composite adapters, reports/overlays, mapped automatic cuts and independent physics references | Queued |
| N08 | Hard cache budgets, cancellation, full-frame instrumentation, million-volume-cell crossing/rotation/deformation >=15 FPS and p95 <=67 ms, long-session resource tests | Queued |
| N09 | Versioned full session, source digests/staging, migration and atomic restoration including camera | Queued |

Passing a partial increment does not close its stage. N08 must include real
contour primitives and redraw; warm same-topology cache timings do not close it.
