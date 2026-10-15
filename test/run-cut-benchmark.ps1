param(
    [string]$BuildDirectory = 'build/mingw-release',
    [int]$Grid = 100,
    [int]$Frames = 60,
    [ValidateRange(0,7)][int]$Mask = 2,
    [ValidateSet('auto','cpu')][string]$Backend = 'auto',
    [string]$RunLabel = 'run',
    [string]$InputFile = '',
    [switch]$MouseDrag,
    [ValidateSet('none','fps','extended')][string]$PerfCounters = 'none',
    [ValidateSet('crossing','rotating','deformed')][string[]]$Scenarios = @('crossing','rotating','deformed'),
    [string]$OcctDirectory = 'D:/workspace/OpenCASCADE/OCCT_private/install/mingw'
)
$ErrorActionPreference = 'Stop'
$repositoryDirectory = Split-Path -Parent $PSScriptRoot
$resolvedBuild = (Resolve-Path -LiteralPath (Join-Path $repositoryDirectory $BuildDirectory)).Path
$runtimeDirectory = Join-Path $resolvedBuild 'bin'
if (!(Test-Path -LiteralPath $runtimeDirectory)) { $runtimeDirectory = Join-Path $resolvedBuild 'bind' }
Copy-Item -LiteralPath "$OcctDirectory/win64/gcc/bin/DRAWEXE.exe" -Destination (Join-Path $runtimeDirectory 'DRAWEXE.exe') -Force
$env:PATH = "$runtimeDirectory;$OcctDirectory/win64/gcc/bin;D:/workspace/OpenCASCADE/3rdparty/mingw64/zlib-1.3.1-mingw-64/bin;C:/mingw64/bin;" + $env:PATH
$env:CSF_OCCTResourcePath = "$OcctDirectory/src"
$env:CSF_ShadersDirectory = "$OcctDirectory/src/Shaders"
$env:CSF_DrawPluginDefaults = "$OcctDirectory/src/DrawResources"
$env:DRAWHOME = "$OcctDirectory/src/DrawResources"
$env:DRAWDEFAULT = "$OcctDirectory/src/DrawResources/DrawDefault"
$env:CUTVIEW_BENCHMARK_GRID = $Grid.ToString()
$env:CUTVIEW_BENCHMARK_FRAMES = $Frames.ToString()
$env:CUTVIEW_BENCHMARK_MASK = $Mask.ToString()
$env:CUTVIEW_BENCHMARK_PERFCOUNTERS = $PerfCounters
$env:CUTVIEW_BENCHMARK_BACKEND = $Backend
$env:CUTVIEW_BENCHMARK_SCENARIOS = $Scenarios -join ' '
$caseLabel = $Grid.ToString()
if ($InputFile) {
    $env:CUTVIEW_BENCHMARK_FILE = (Resolve-Path -LiteralPath $InputFile).Path
    $caseLabel = [System.IO.Path]::GetFileName($InputFile)
} else { Remove-Item Env:CUTVIEW_BENCHMARK_FILE -ErrorAction SilentlyContinue }
$env:CUTVIEW_OUTPUT_DIR = Join-Path $resolvedBuild "test/native/draw-benchmark-$caseLabel-$Mask-$Backend-$RunLabel"
$benchmarkLog = Join-Path $resolvedBuild "cut-full-frame-$caseLabel-$Mask-$Backend-$RunLabel.log"
if ($MouseDrag) {
    if (!$InputFile -or $Backend -ne 'auto' -or $Mask -notin @(2,3,7)) { throw 'MouseDrag requires InputFile, auto backend and mask 2, 3 or 7' }
    & (Join-Path $runtimeDirectory 'DRAWEXE.exe') -i -f (Join-Path $PSScriptRoot 'cae/cut_view_drag_benchmark.tcl') *> $benchmarkLog
    $passMarker = 'CUT_DRAG_BENCHMARK_PASS'
} else {
    & (Join-Path $runtimeDirectory 'DRAWEXE.exe') -b -f (Join-Path $PSScriptRoot 'cae/cut_view_benchmark.tcl') *> $benchmarkLog
    $passMarker = 'CUT_BENCHMARK_PASS'
}
$benchmarkExit = $LASTEXITCODE
Get-Content -LiteralPath $benchmarkLog
if ($benchmarkExit -ne 0 -or !(Select-String -LiteralPath $benchmarkLog -SimpleMatch $passMarker -Quiet)) { exit 1 }
exit 0
