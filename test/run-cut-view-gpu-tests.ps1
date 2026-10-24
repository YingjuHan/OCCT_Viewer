# Windows Release validation. Use one runner at a time; GPU preferences are per EXE.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][ValidateSet('MX450','IrisXe')][string]$Gpu,
    [Parameter(Mandatory=$true)][string]$BuildDirectory,
    [Parameter(Mandatory=$true)][string]$DrawExe,
    [Parameter(Mandatory=$true)][string]$OcctResources,
    [Parameter(Mandatory=$true)][string]$Model,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [string[]]$RuntimeDirectories = @(),
    [ValidateSet('Probe','Performance','Functional','Large','Camera','All')][string]$Mode = 'All',
    [ValidateRange(2,1000)][int]$Frames = 60,
    [ValidateRange(1,20)][int]$Repetitions = 3,
    [ValidateSet('cloud','wire','cloud_wire')][string]$DisplayMode = 'cloud',
    [switch]$FullValidation
)
$ErrorActionPreference = 'Stop'
$cutRepo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$BuildDirectory = (Resolve-Path -LiteralPath $BuildDirectory).Path
$DrawExe = (Resolve-Path -LiteralPath $DrawExe).Path
$OcctResources = (Resolve-Path -LiteralPath $OcctResources).Path
$Model = (Resolve-Path -LiteralPath $Model).Path
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $OutputDirectory) { throw "Output already exists: $OutputDirectory" }
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$cutManifest = [ordered]@{
    gpu=$Gpu; mode=$Mode; started=(Get-Date -Format o); frames=$Frames; repetitions=$Repetitions; displayMode=$DisplayMode
    cellTopologyReuse=($env:OCCTV_CUT_CELL_TOPOLOGY_CACHE -eq '1')
    fullValidation=[bool]$FullValidation
    repository=$cutRepo; build=$BuildDirectory; executable=$DrawExe; model=$Model
    revision=(& git -C $cutRepo rev-parse HEAD); succeeded=$false
}
$cutEnvironment = @{}
foreach ($entry in Get-ChildItem Env:) { $cutEnvironment[$entry.Name] = $entry.Value }
$cutRegistryPath = 'Software\Microsoft\DirectX\UserGpuPreferences'
$cutRegistry = [Microsoft.Win32.Registry]::CurrentUser.CreateSubKey($cutRegistryPath)
$cutHadPreference = @($cutRegistry.GetValueNames()) -contains $DrawExe
$cutOldPreference = $cutRegistry.GetValue($DrawExe)
$cutOldKind = if ($cutHadPreference) { $cutRegistry.GetValueKind($DrawExe) } else { $null }
$cutManifest.preferenceBefore = @{exists=$cutHadPreference; value=$cutOldPreference; kind=[string]$cutOldKind}
$cutExpected = if ($Gpu -eq 'MX450') { 'NVIDIA GeForce MX450' } else { 'Intel(R) Iris(R) Xe Graphics' }
$cutPreference = if ($Gpu -eq 'MX450') { 'GpuPreference=2;' } else { 'GpuPreference=1;' }
function Invoke-CutDraw([string]$ScriptName, [string]$LogPath, [string]$PassPattern) {
    & $DrawExe -i -f (Join-Path $cutRepo "test/cae/$ScriptName") *> $LogPath
    $cutExit = $LASTEXITCODE
    Get-Content -LiteralPath $LogPath -Tail 8
    if ($cutExit -ne 0 -or !(Select-String -LiteralPath $LogPath -Pattern $PassPattern -Quiet)) {
        throw "DRAW validation failed: $LogPath (exit $cutExit)"
    }
}
try {
    # Store only this executable's setting; restore its exact original value/kind.
    $cutRegistry.SetValue($DrawExe,$cutPreference,[Microsoft.Win32.RegistryValueKind]::String)
    $cutManifest.preferenceDuring = $cutPreference
    $cutManifest | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $OutputDirectory 'manifest.json')
    $env:PATH = (@((Join-Path $BuildDirectory 'bin'),(Split-Path $DrawExe -Parent)) + $RuntimeDirectories + @($env:PATH)) -join ';'
    $env:CSF_OCCTResourcePath = $OcctResources
    $env:CSF_ShadersDirectory = Join-Path $OcctResources 'Shaders'
    $env:DRAWHOME = Join-Path $OcctResources 'DrawResources'
    $env:DRAWDEFAULT = Join-Path $env:DRAWHOME 'DrawDefault'
    $env:CSF_DrawPluginDefaults = $env:DRAWHOME
    $env:CSF_VIEWDRAWDefaults = Join-Path $cutRepo 'source-draw/ViewDraw'
    $env:OCCTV_DRAW_RUNTIME = Join-Path $BuildDirectory 'bin/viewdraw-runtime.tcl'
    # Do not let an inherited file-mode benchmark turn CTest smoke into a big run.
    foreach ($entry in @(Get-ChildItem Env:CUTVIEW_*)) { Remove-Item -LiteralPath $entry.PSPath }
    $env:CUTVIEW_EXPECTED_GPU = $cutExpected
    Get-FileHash -LiteralPath $Model,$DrawExe,(Join-Path $BuildDirectory 'bin/libViewCaeTest.dll'),(Join-Path $BuildDirectory 'bin/libViewer.dll') -Algorithm SHA256 |
        Select-Object Path,Hash | ConvertTo-Json | Set-Content (Join-Path $OutputDirectory 'hashes.json')
    Get-CimInstance Win32_Processor | Select-Object Name,NumberOfCores,NumberOfLogicalProcessors | ConvertTo-Json | Set-Content (Join-Path $OutputDirectory 'cpu.json')
    Get-CimInstance Win32_VideoController | Select-Object Name,DriverVersion | ConvertTo-Json | Set-Content (Join-Path $OutputDirectory 'adapters.json')
    & powercfg /getactivescheme > (Join-Path $OutputDirectory 'power-plan.txt')
    Invoke-CutDraw 'cut_view_gpu_probe.tcl' (Join-Path $OutputDirectory 'renderer-before.log') '^CUT_GPU_PROBE_PASS'
    if ($Mode -in @('Functional','All')) {
        Write-Output 'CUT_GPU_FUNCTIONAL_START'
        & ctest --test-dir $BuildDirectory --parallel 1 --output-on-failure --output-junit (Join-Path $OutputDirectory 'functional.xml') *> (Join-Path $OutputDirectory 'functional.log')
        $cutExit = $LASTEXITCODE
        Copy-Item -LiteralPath (Join-Path $BuildDirectory 'Testing/Temporary/LastTest.log') -Destination (Join-Path $OutputDirectory 'functional-details.log')
        Get-Content (Join-Path $OutputDirectory 'functional.log') -Tail 15
        if ($cutExit -ne 0) { throw 'CTest functional regression failed' }
        # Every visual benchmark scene also records its own actual renderer.
        foreach ($scene in @('crossing','rotating','deformed')) {
            $cutRenderer = Join-Path $BuildDirectory "test/native/draw-benchmark-smoke/$scene-renderer.txt"
            if (!(Get-Content -LiteralPath $cutRenderer -Raw).Contains($cutExpected)) { throw "CTest used unexpected renderer: $cutRenderer" }
            Copy-Item -LiteralPath $cutRenderer -Destination (Join-Path $OutputDirectory "functional-$scene-renderer.txt")
        }
        $cutManifest.functionalPassed = $true
    }
    if ($Mode -in @('Large','All')) {
        $env:CUTVIEW_BENCHMARK_FILE = $Model
        foreach ($cutCase in @('placement','material','free_body','iso_binding','slices','plot_options','automatic','scope')) {
            $env:CUTVIEW_OUTPUT_DIR = Join-Path $OutputDirectory "large/$cutCase"
            New-Item -ItemType Directory -Force -Path $env:CUTVIEW_OUTPUT_DIR | Out-Null
            Write-Output "CUT_GPU_LARGE_START $cutCase"
            Invoke-CutDraw "cut_view_${cutCase}_large.tcl" (Join-Path $env:CUTVIEW_OUTPUT_DIR 'run.log') '^CUT_.*_LARGE_PASS'
        }
        $cutManifest.largePassed = $true
    }
    if ($Mode -in @('Performance','Camera','All')) {
        $env:CUTVIEW_BENCHMARK_FILE = $Model
        $env:CUTVIEW_BENCHMARK_FRAMES = [string]$Frames
        $env:CUTVIEW_BENCHMARK_MASK = '3'
        $env:CUTVIEW_BENCHMARK_BACKEND = 'auto'
        $env:CUTVIEW_BENCHMARK_DISPLAY_MODE = $DisplayMode
        $env:CUTVIEW_BENCHMARK_FULL_VALIDATION = if($FullValidation){'1'}else{'0'}
        $env:CUTVIEW_BENCHMARK_DEFORMATION_SCALE = '0.1'
        $env:CUTVIEW_BENCHMARK_SCENARIOS = 'crossing rotating deformed'
        $cutCategory = if ($Mode -eq 'Camera') { 'camera' } else { 'performance' }
        $cutScript = if ($Mode -eq 'Camera') { 'cut_view_gpu_camera.tcl' } else { 'cut_view_benchmark.tcl' }
        $cutPass = if ($Mode -eq 'Camera') { '^CUT_GPU_CAMERA_PASS' } else { '^CUT_BENCHMARK_PASS' }
        for ($cutRound = 1; $cutRound -le $Repetitions; ++$cutRound) {
            $env:CUTVIEW_OUTPUT_DIR = Join-Path $OutputDirectory "$cutCategory/round-$cutRound"
            New-Item -ItemType Directory -Force -Path $env:CUTVIEW_OUTPUT_DIR | Out-Null
            Write-Output "CUT_GPU_PERFORMANCE_START $cutRound"
            Invoke-CutDraw $cutScript (Join-Path $env:CUTVIEW_OUTPUT_DIR 'run.log') $cutPass
            foreach ($scene in @('crossing','rotating','deformed')) {
                $cutRenderer = Join-Path $env:CUTVIEW_OUTPUT_DIR "$scene-renderer.txt"
                if (!(Get-Content -LiteralPath $cutRenderer -Raw).Contains($cutExpected)) { throw "Unexpected performance renderer: $cutRenderer" }
            }
        }
        # target=0 is a performance miss, not a numerical failure; report both.
        if ($Mode -eq 'Camera') { $cutManifest.cameraCompleted = $true }
        else { $cutManifest.performanceCompleted = $true }
    }
    Invoke-CutDraw 'cut_view_gpu_probe.tcl' (Join-Path $OutputDirectory 'renderer-after.log') '^CUT_GPU_PROBE_PASS'
    $cutManifest.succeeded = $true
    Write-Output "CUT_GPU_TESTS_PASS $Gpu $Mode"
} finally {
    if ($cutHadPreference) { $cutRegistry.SetValue($DrawExe,$cutOldPreference,$cutOldKind) }
    else { $cutRegistry.DeleteValue($DrawExe,$false) }
    $cutManifest.preferenceRestored = ($cutRegistry.GetValue($DrawExe) -eq $cutOldPreference)
    $cutRegistry.Dispose()
    $cutManifest.finished = Get-Date -Format o
    $cutManifest | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $OutputDirectory 'manifest.json')
    foreach ($entry in @(Get-ChildItem Env:)) {
        if (!$cutEnvironment.ContainsKey($entry.Name)) { Remove-Item -LiteralPath $entry.PSPath }
    }
    foreach ($cutName in $cutEnvironment.Keys) { Set-Item -LiteralPath "Env:$cutName" -Value $cutEnvironment[$cutName] }
}
