param(
    [string]$BuildDirectory = 'build/mingw-release',
    [int]$Grid = 100,
    [int]$Frames = 60,
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
$env:CUTVIEW_OUTPUT_DIR = Join-Path $resolvedBuild "test/native/draw-benchmark-$Grid"
$benchmarkLog = Join-Path $resolvedBuild "cut-full-frame-$Grid.log"
& (Join-Path $runtimeDirectory 'DRAWEXE.exe') -b -f (Join-Path $PSScriptRoot 'cae/cut_view_benchmark.tcl') *> $benchmarkLog
$benchmarkExit = $LASTEXITCODE
Get-Content -LiteralPath $benchmarkLog
if ($benchmarkExit -ne 0 -or !(Select-String -LiteralPath $benchmarkLog -SimpleMatch 'CUT_BENCHMARK_PASS' -Quiet)) { exit 1 }
exit 0
