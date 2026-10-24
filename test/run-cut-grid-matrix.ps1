# Million-cell numerical and display-mode validation. Cases run sequentially.
[CmdletBinding()]
param(
    [string]$BuildDirectory = 'D:/workspace/OpenCASCADE/OCCT_Viewer/build/mingw-deformation',
    [string]$DrawExe = 'D:/workspace/OpenCASCADE/OCCT_private/install/mingw/win64/gcc/bin/DRAWEXE.exe',
    [string]$OcctResources = 'D:/workspace/OpenCASCADE/OCCT_private/install/mingw/src',
    [string]$ModelDirectory = 'D:/workspace/OpenCASCADE/OCCT_Viewer/build/cut-nonregular',
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [ValidateSet('MX450','IrisXe')][string[]]$Gpus = @('MX450','IrisXe'),
    [ValidateRange(2,1000)][int]$Frames = 12
)
$ErrorActionPreference = 'Stop'
$cutModels = @('tetra_60.vtu','tetra_60.vtk','mixed_70.vtu','hex_100_warped.vtu')
foreach($model in $cutModels) { $null = Resolve-Path -LiteralPath (Join-Path $ModelDirectory $model) }
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $OutputDirectory) { throw "Output already exists: $OutputDirectory" }
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
$records = @()
foreach($gpu in $Gpus) {
    foreach($model in $cutModels) {
        foreach($displayMode in @('cloud','wire','cloud_wire')) {
            $case = Join-Path $OutputDirectory "$gpu/$model/$displayMode"
            Write-Output "CUT_GRID_START $gpu $model $displayMode"
            & (Join-Path $PSScriptRoot 'run-cut-view-gpu-tests.ps1') -Gpu $gpu -BuildDirectory $BuildDirectory `
                -DrawExe $DrawExe -OcctResources $OcctResources -Model (Join-Path $ModelDirectory $model) `
                -OutputDirectory $case -RuntimeDirectories @('C:/mingw64/bin','D:/workspace/OpenCASCADE/3rdparty/mingw64/zlib-1.3.1-mingw-64/bin') `
                -Mode Performance -Frames $Frames -Repetitions 1 -DisplayMode $displayMode -FullValidation
            $lines = @(Select-String -LiteralPath (Join-Path $case 'performance/round-1/run.log') -Pattern 'CUT_BENCHMARK_RESULT')
            if($lines.Count -ne 3) { throw "Missing scene results: $case" }
            $results = @()
            foreach($line in $lines) {
                $checks = [regex]::Match($line.Line,'full_checks ([0-9.eE+-]+)')
                if(!$checks.Success -or [double]::Parse($checks.Groups[1].Value,[cultureinfo]::InvariantCulture) -ne 2) { throw "Full numerical validation missing: $case" }
                $results += $line.Line
            }
            $records += [ordered]@{gpu=$gpu; model=$model; displayMode=$displayMode; directory=$case; frames=$Frames; results=$results}
            $records | ConvertTo-Json -Depth 5 | Set-Content -Encoding utf8 (Join-Path $OutputDirectory 'matrix.json')
            Write-Output "CUT_GRID_PASS $gpu $model $displayMode"
        }
    }
}
Write-Output "CUT_GRID_MATRIX_PASS cases $($records.Count)"
