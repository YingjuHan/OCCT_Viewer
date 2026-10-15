# Capture only the DRAW/Tk manager window for UI regression review.
param([Parameter(Mandatory = $true)][string]$WindowHandle,
      [Parameter(Mandatory = $true)][string]$OutputPath)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class CutViewWindowCapture {
    [StructLayout(LayoutKind.Sequential)]
    public struct Rect { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")] public static extern IntPtr GetAncestor(IntPtr window, uint flags);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr window, int command);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr window, IntPtr insertAfter, int x, int y, int width, int height, uint flags);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr window, IntPtr dc, uint flags);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr window, out Rect rectangle);
}
'@
$cutWindowId = if ($WindowHandle.StartsWith('0x')) {
    [Convert]::ToInt64($WindowHandle.Substring(2), 16)
} else { [long]::Parse($WindowHandle) }
$cutWindow = [IntPtr]::new($cutWindowId)
$cutRoot = [CutViewWindowCapture]::GetAncestor($cutWindow, 2)
# DRAW started by CTest may have a hidden top-level wrapper. Restore this test
# window so Tk paints its client widgets before requesting the full content.
[void][CutViewWindowCapture]::ShowWindow($cutRoot, 9)
[void][CutViewWindowCapture]::SetWindowPos($cutRoot, [IntPtr]::new(-1), 0, 0, 0, 0, 3)
Start-Sleep -Milliseconds 150
$cutRectangle = New-Object CutViewWindowCapture+Rect
if (-not [CutViewWindowCapture]::GetWindowRect($cutRoot, [ref]$cutRectangle)) { throw 'Invalid Tk manager window' }
$cutWidth = $cutRectangle.Right - $cutRectangle.Left
$cutHeight = $cutRectangle.Bottom - $cutRectangle.Top
if ($cutWidth -lt 1 -or $cutHeight -lt 1 -or $cutWidth -gt 4096 -or $cutHeight -gt 4096) { throw 'Invalid Tk client size' }
$cutBitmap = New-Object System.Drawing.Bitmap($cutWidth, $cutHeight)
$cutGraphics = [System.Drawing.Graphics]::FromImage($cutBitmap)
try {
    # Tk paints through the root wrapper with PW_RENDERFULLCONTENT (2),
    # not through its inner client HWND. Tk must keep dispatching events.
    $cutDc = $cutGraphics.GetHdc()
    try {
        if (-not [CutViewWindowCapture]::PrintWindow($cutRoot, $cutDc, 2)) { throw 'Tk manager capture failed' }
    } finally { $cutGraphics.ReleaseHdc($cutDc) }
    $cutColors = [Collections.Generic.HashSet[int]]::new()
    for ($cutY = 50; $cutY -lt $cutHeight - 40; $cutY += 16) {
        for ($cutX = 16; $cutX -lt $cutWidth - 16; $cutX += 16) { [void]$cutColors.Add($cutBitmap.GetPixel($cutX, $cutY).ToArgb()) }
    }
    if ($cutColors.Count -lt 8) { throw 'Tk capture contains no controls' }
    $cutBitmap.Save([IO.Path]::GetFullPath($OutputPath), [System.Drawing.Imaging.ImageFormat]::Png)
    Write-Output 'CUT_VIEW_CAPTURE_PASS'
} finally {
    $cutGraphics.Dispose(); $cutBitmap.Dispose()
    [void][CutViewWindowCapture]::SetWindowPos($cutRoot, [IntPtr]::new(-2), 0, 0, 0, 0, 3)
}
