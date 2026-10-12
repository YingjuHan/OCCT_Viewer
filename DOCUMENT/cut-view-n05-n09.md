# Native N05–N09 execution

N01–N04 are the verified baseline at `4f135da`. Native stages follow dependency order;
each completed increment gets its own validation evidence, commit and explicit
codebase-memory refresh. The web/WASM track remains excluded.

| Stage | Work and acceptance | Status |
| --- | --- | --- |
| N05 | Datums/editing, styles/legend, original-FE picking/probes, vector/annotation display, CAD/clipping, native lifecycle API and real Tk controls | Verified: ON 61/61, OFF 24/24, headless 2/2 x3; 48+36 controls |
| N06 | Material frame, atomic geometry/field/deformation steps, animation and slicing with remesh/failure/abort tests | Verified: ON 64/64, focused 7/7 x3, OFF 26/26 x3; 20 time controls |
| N07 | Explicit groups/units/frames and shell/beam/composite adapters, reports/overlays, mapped automatic cuts and independent physics references | Verified: ON 67/67, OFF 28/28 x3; 31 physics controls |
| N08 | Logical cache reservation budgets, cancellation, full-frame instrumentation, million-volume-cell crossing/rotation/deformation >=15 FPS and p95 <=67 ms, long-session resource tests | Verified: three measured million-cell runs; 4.41 MiB private-byte growth; logical cache/lease/cancellation tests |
| N09 | Versioned full session, source digests/staging, migration and atomic restoration including camera | Active: final session/budget validation and documentation |

Passing a partial increment does not close its stage. N08 measurements include real contour primitives and redraw. See
[the audit](cut-view-n08-audit.md) for full-frame evidence and the bounded workload.
