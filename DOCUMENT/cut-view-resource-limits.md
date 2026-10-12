# Native VTK Import Resource Limits

`RWVtk::VtkResourceLimits` configures standalone readers with `SetResourceLimits()`
and MeshLoader with `SetVtkResourceLimits()`. Composite/series children share one
budget; separate imports reset it. A limit failure rejects publication and keeps
the live Viewer model. Zero is not an unlimited setting.

| Limit | Default | Checked before |
| --- | --- | --- |
| MaxFileBytes | 512 MiB | Each stream chunk and generated Legacy XML growth |
| MaxInputBytes | 2 GiB | Aggregate stream growth across referenced files/steps |
| MaxArrayBytes | 256 MiB | ASCII typed storage, binary payload, packed/decompressed blocks, converted and dense merged field storage |
| MaxDecodedBytes | 1 GiB | Cumulative decoded/converted/merged numeric storage |
| MaxArrays | 4096 | Array header/component allocation |
| MaxComponents | 1024 | Component-name vector allocation |
| MaxNodes / MaxCells | 10,000,000 each | Mesh/stored-element allocation; PolyData expansions also consume cells |
| MaxFiles / MaxDepth | 4096 / 64 | Reader/stager file traversal and reference/block depth; cyclic VTM references reject |

Portable and native imports validate the declared dimensions; the native SDK runs
only after original numeric arrays, topology and budgets pass preflight. Legacy
normalization has its own bounded output stream. Sparse composite fields are
budgeted at their full merged sizes before `CopyRaw` allocates them. Mapping
fan-out and replicated PolyData CellData consume converted-field budgets.

Appended raw data uses spans into the input buffer. Base64 uses an incremental
quartet cursor, including separately padded header/payload chunks; offsets remain
encoded-character offsets for base64. Only the current header and payload are
decoded. Zlib validates the complete declared output size and packed block sizes
before allocating its result. Inline arrays reject trailing bytes. Integer overflow
checks remain mandatory even when users raise limits.

The XML envelope and appended source are still read into bounded file buffers;
this is not on-demand file streaming. The limits are logical import budgets,
not an exact OS working-set cap: DOM/SDK overhead and published-series copies
also occupy memory. Full session cache/eviction and rendering budgets remain N08.
Explicit local staging checks copied source bytes/files/depth before dataset
decoding, using a separate budget with the same configured limits. Generated
manifest output is bounded by MaxFileBytes. Successful attempt directories remain
caller-owned; staging failure removes only that attempt's directory.

`vtk_limits` checks configurable limits, an oversized Legacy declaration, a
decompression-size bomb, dense merged fields and aggregate series node limits.
`vtk_safety` retains endian/type/compression/truncation checks. Final focused ON
9/9 and OFF 7/7 each pass three times (52.19 / 1.87 seconds).

Run the Windows peak-memory gate after building both configurations:

```powershell
./test/native/MeasureVtkMemory.ps1
```

It runs separate processes with 16/64/128 raw appended arrays, each 512 KiB,
validates every original offset/value and reads the kernel PeakWorkingSetSize.
Measured peak bytes on this MinGW Debug environment:

| Payload | VTK ON | VTK OFF |
| --- | ---: | ---: |
| 8 MiB | 37,638,144 | 32,915,456 |
| 32 MiB | 87,990,272 | 82,681,856 |
| 64 MiB | 154,669,056 | 150,351,872 |

All six processes stay below 256 MiB. This measures decoder temporary storage
with discarded array outputs; it is not a million-cell Viewer memory/performance
claim. Logs are `cut-n04-limits-focused.log`, `cut-n04-memory-*.log` in each build
and `build/cut-n04-memory.json`.

N04.8 repeats all six peak-memory processes after the final repair; each remains
below 256 MiB. Original N04.6 values are preserved in the milestone queue.
Final full native/DRAW acceptance is in [the closure audit](cut-view-n04-audit.md).
