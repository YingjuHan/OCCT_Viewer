param([string[]]$Configurations = @('mingw','mingw-no-vtk'))
$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$results = @()
function CacheValue($cache, $name) {
  $match = $cache | Select-String "^${name}:.*?=(.+)$" | Select-Object -First 1
  if ($match) { return $match.Matches[0].Groups[1].Value }
  return $null
}
foreach ($config in $Configurations) {
  $cache = Get-Content -LiteralPath (Join-Path $taskRoot "build/$config/CMakeCache.txt")
  $occtCmake = CacheValue $cache 'OpenCASCADE_DIR'
  $compiler = CacheValue $cache 'CMAKE_CXX_COMPILER'
  $paths = @((Join-Path (Split-Path $occtCmake -Parent) 'win64/gcc/bin'), (Split-Path $compiler -Parent))
  $zlib = CacheValue $cache 'ZLIB_LIBRARY_RELEASE'
  if ($zlib) { $paths += Join-Path (Split-Path (Split-Path $zlib -Parent) -Parent) 'bin' }
  $env:PATH = ($paths -join ';') + ';' + $env:PATH
  foreach ($arrays in @(16,64,128)) {
    $out = Join-Path $taskRoot "build/$config/cut-n04-memory-$arrays.log"
    $err = Join-Path $taskRoot "build/$config/cut-n04-memory-$arrays.err"
    $exe = Join-Path $taskRoot "build/$config/bind/vtk_limits_tests.exe"
    $proc = Start-Process -FilePath $exe -ArgumentList @('--memory', "$arrays") -WindowStyle Hidden -PassThru -Wait -RedirectStandardOutput $out -RedirectStandardError $err
    if ($proc.ExitCode -ne 0 -or -not (Select-String -LiteralPath $out -SimpleMatch 'MEMORY_PASS')) { throw "Memory test failed: $config/$arrays" }
    $peakLine = (Select-String -LiteralPath $out -Pattern '^MEMORY_PEAK_BYTES ([0-9]+)$').Matches.Groups[1].Value
    if (-not $peakLine) { throw 'Missing kernel process peak-memory measurement' }
    $peak = [long]$peakLine
    if ($peak -gt 256MB) { throw "Peak memory exceeded acceptance: $config/$arrays/$peak" }
    $results += [pscustomobject]@{ config=$config; arrays=$arrays; payload_bytes=$arrays*512KB; peak_working_set_bytes=$peak }
  }
}
$results | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskRoot 'build/cut-n04-memory.json') -Encoding utf8
$results | Format-Table
