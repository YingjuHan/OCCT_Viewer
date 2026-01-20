# Web Benchmark Notes

This document describes the minimal, repeatable benchmark output exposed in `samples/html` (Benchmark panel).

## Metrics
- `loadMs`: time from load start to model load completion.
- `firstFrameMs`: time from load start to first rendered frame after load.
- `memBefore` / `memAfter`: JS heap usage snapshot from `performance.memory` when available (otherwise `null`).

## How To Run
1. Open `samples/html/index.html` with a local server.
2. Open the "Benchmark" panel.
3. Select a model and click "Run Benchmark".
4. Copy the JSON output from the panel.

## Output JSON
Example keys:
- `model`: file name
- `type`: `shape` or `mesh`
- `loadMs`, `firstFrameMs`
- `memBefore`, `memAfter`

## Repeatability Tips
- Use the same browser/device and close other heavy tabs.
- Reload the page between runs or clear the scene before each run.
- Prefer a local server to avoid network variance.

## Limitations
- `performance.memory` is only available in Chromium-based browsers.
- Timings include decoding and GPU upload in the current implementation.
