# T10 evidence

The two Windows Release suites use the same 3,375,000-cell VTU and native binaries. Run them sequentially with `test/run-cut-view-gpu-tests.ps1 -Mode All -Frames 60 -Repetitions 3`; use a fresh output directory for each GPU.

`DOCUMENT/tools/summarize_cut_performance.py` verifies and copies the evidence here. Each GPU directory contains its manifest, SHA-256 hashes, hardware/renderer probes, 85-case CTest XML and logs, eight large-model gates, and nine 60-frame CSVs. `summary.json` pools the original 180 frames per GPU/scenario; it does not average round FPS or percentiles.

The manifests identify the base Git revision and pending test instrumentation. Binary hashes identify the exact measured build. `instrumentation.patch` records the benchmark instrumentation relative to the base revision. The product changes T01–T09 were already committed before these suites started.

`image-comparison.json` records the first/last frame comparison for all three rounds and scenarios, including both PNG hashes. PNGs and the large VTU remain in the corresponding `build/perf-t10-*` directories and are not committed.

Per-frame process memory samples are outside `total_ms`. Private/working-set samples do not capture intra-frame transient peaks; peak working set is the whole process lifetime high-water mark, including prior scenes. These counters are neither the logical cache budget nor GPU memory.

Numerical or suite PASS does not imply that the FPS/p95 performance target passed. See [the report](../../../cut-view-manager-performance-report-2026-10-24.md) for results and limitations.
