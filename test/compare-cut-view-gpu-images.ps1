# Windows PowerShell: powershell.exe -NoProfile -File <this file> -First <png> -Second <png>
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$First,
    [Parameter(Mandatory=$true)][string]$Second,
    [ValidateRange(0,255)][int]$Tolerance = 2
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
if (!('CutGpuImageStats' -as [type])) {
    $cutReferences = @([System.Drawing.Bitmap].Assembly.Location,[System.Drawing.Color].Assembly.Location)
    foreach ($cutAssembly in @('System.Private.Windows.Core.dll','System.Private.Windows.GdiPlus.dll')) {
        $cutAssemblyPath = Join-Path $PSHOME $cutAssembly
        if (Test-Path -LiteralPath $cutAssemblyPath) { $cutReferences += $cutAssemblyPath }
    }
    Add-Type -ReferencedAssemblies ($cutReferences | Select-Object -Unique) -TypeDefinition @'
using System;
using System.Drawing;
public static class CutGpuImageStats {
    public static long[] Compare(string first, string second, int tolerance) {
        using (var a = new Bitmap(first)) using (var b = new Bitmap(second)) {
            if (a.Width != b.Width || a.Height != b.Height) throw new ArgumentException("Image dimensions differ");
            long changed = 0, maskXor = 0, above = 0;
            int maximum = 0;
            for (int y = 0; y < a.Height; ++y) for (int x = 0; x < a.Width; ++x) {
                Color p = a.GetPixel(x,y), q = b.GetPixel(x,y);
                bool pa = Math.Min(p.R,Math.Min(p.G,p.B)) < 250;
                bool pb = Math.Min(q.R,Math.Min(q.G,q.B)) < 250;
                int error = Math.Max(Math.Abs((int)p.R-q.R),Math.Max(Math.Abs((int)p.G-q.G),Math.Abs((int)p.B-q.B)));
                if (p.ToArgb() != q.ToArgb()) ++changed;
                if (pa != pb) ++maskXor;
                if (error > tolerance || p.A != q.A) ++above;
                maximum = Math.Max(maximum,error);
            }
            return new long[] {a.Width,a.Height,changed,maskXor,maximum,above};
        }
    }
}
'@
}
$First = (Resolve-Path -LiteralPath $First).Path
$Second = (Resolve-Path -LiteralPath $Second).Path
$cutValues = [CutGpuImageStats]::Compare($First,$Second,$Tolerance)
[pscustomobject]@{
    first=$First; second=$Second
    firstSha256=(Get-FileHash -LiteralPath $First -Algorithm SHA256).Hash
    secondSha256=(Get-FileHash -LiteralPath $Second -Algorithm SHA256).Hash
    width=$cutValues[0]; height=$cutValues[1]; changedPixels=$cutValues[2]
    foregroundMaskXorPixels=$cutValues[3]; foregroundThreshold=250
    maximumChannelError=$cutValues[4]; pixelsAboveTolerance=$cutValues[5]; tolerance=$Tolerance
}
if ($cutValues[5] -ne 0) { throw 'Cross-GPU image exceeds channel tolerance' }
