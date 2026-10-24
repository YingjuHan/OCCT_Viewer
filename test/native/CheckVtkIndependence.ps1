param([string]$BuildDirectory = 'build/mingw')
$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$taskBuild = (Resolve-Path -LiteralPath (Join-Path $taskRoot $BuildDirectory)).Path
$objdump = (Get-Command objdump.exe -ErrorAction Stop).Source
$artifacts = @()
foreach ($directory in @('bind','bin')) {
    $artifactDirectory = Join-Path $taskBuild $directory
    if (Test-Path -LiteralPath $artifactDirectory) {
        $artifacts += @(Get-ChildItem -LiteralPath $artifactDirectory -File | Where-Object Extension -in @('.dll','.exe'))
    }
}
if (!$artifacts.Count) { throw 'No native artifacts to audit' }
$results = foreach ($artifact in $artifacts) {
    $headers = & $objdump -p $artifact.FullName
    if ($LASTEXITCODE -ne 0) { throw "Cannot inspect $($artifact.Name)" }
    $imports = @($headers | Select-String 'DLL Name:\s*(\S+)' | ForEach-Object { $_.Matches[0].Groups[1].Value })
    $vtkImports = @($imports | Where-Object { $_ -match '^(lib)?vtk.*\.dll$' })
    if ($vtkImports.Count) { throw "External VTK dependency in $($artifact.Name): $($vtkImports -join ', ')" }
    [pscustomobject]@{ artifact=$artifact.Name; imports=$imports }
}
$results | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $taskBuild 'vtk-dependency-audit.json') -Encoding utf8
Write-Output "VTK_INDEPENDENCE_PASS $($artifacts.Count) artifacts"
