param(
    [string]$BuildDirectory = 'build/mingw',
    [ValidateRange(6,150)][int]$Grid = 6,
    [string]$OcctDirectory = 'D:/workspace/OpenCASCADE/OCCT_private/install/mingw',
    [string]$OcctSource = 'D:/workspace/OpenCASCADE/OCCT_private/src'
)
$ErrorActionPreference = 'Stop'
$repositoryDirectory = Split-Path -Parent $PSScriptRoot
$resolvedBuild = (Resolve-Path -LiteralPath (Join-Path $repositoryDirectory $BuildDirectory)).Path
$runtimeDirectory = Join-Path $resolvedBuild 'bind'
if (!(Test-Path -LiteralPath $runtimeDirectory)) { $runtimeDirectory = Join-Path $resolvedBuild 'bin' }
$fixtureDirectory = Join-Path $resolvedBuild 'test/native/upload-fixture'
& python (Join-Path $PSScriptRoot 'generate-large-vtk.py') --grids $Grid --deformation --formats vtu --output $fixtureDirectory
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Copy-Item -LiteralPath "$OcctDirectory/win64/gcc/bin/DRAWEXE.exe" -Destination (Join-Path $runtimeDirectory 'DRAWEXE.exe') -Force
$env:PATH = "$runtimeDirectory;$OcctDirectory/win64/gcc/bin;D:/workspace/OpenCASCADE/3rdparty/mingw64/zlib-1.3.1-mingw-64/bin;C:/mingw64/bin;" + $env:PATH
$env:CSF_OCCTResourcePath = "$OcctDirectory/src"
$env:CSF_ShadersDirectory = "$OcctDirectory/src/Shaders"
$env:CSF_DrawPluginDefaults = "$OcctDirectory/src/DrawResources"
$env:DRAWHOME = "$OcctDirectory/src/DrawResources"
$env:DRAWDEFAULT = "$OcctDirectory/src/DrawResources/DrawDefault"
$env:OCCTV_OCCT_SOURCE = $OcctSource.Replace('\','/')
$env:CUTVIEW_UPLOAD_FIXTURE = Join-Path $fixtureDirectory "hex_$Grid.vtu"
$captureLabel = if ($Grid -eq 6) { 'cut-upload-capture' } else { "cut-upload-capture-$Grid" }
$env:CUTVIEW_UPLOAD_REPORT = Join-Path $resolvedBuild "$captureLabel.json"
$captureScript = (Join-Path $PSScriptRoot 'capture-cut-upload.py').Replace('\','/')
$captureLog = Join-Path $resolvedBuild "$captureLabel.log"
& gdb -batch -ex "source $captureScript" -ex run --args (Join-Path $runtimeDirectory 'DRAWEXE.exe') -b -f (Join-Path $PSScriptRoot 'cae/cut_view_upload_capture.tcl') *> $captureLog
$captureExit = $LASTEXITCODE
Get-Content -LiteralPath $captureLog -Tail 30
if ($captureExit -ne 0 -or !(Select-String -LiteralPath $captureLog -SimpleMatch 'UPLOAD_CAPTURE_PASS' -Quiet)) { exit 1 }
exit 0
