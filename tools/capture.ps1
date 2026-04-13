<#
.SYNOPSIS
    Capture animated GIFs of TAKEOVER running in DOSBox-X.

.PARAMETER Scenario
    Which capture to run: "menu", "gameplay", "effects", or a custom .scn file.

.PARAMETER Duration
    Recording duration in seconds (default: 15).

.PARAMETER FPS
    Frames per second for capture (default: 4).

.PARAMETER OutFile
    Output GIF path. Defaults to website img/takeover-<scenario>.gif.

.PARAMETER AutoType
    DOSBox-X AUTOTYPE keystroke string.

.PARAMETER GifWidth
    Output GIF width in pixels (default: 480).

.PARAMETER StartDelay
    Seconds to wait after DOSBox-X launches before recording (default: 3).

.PARAMETER KeepFrames
    If set, don't delete the frame directory after GIF creation.
#>

param(
    [Parameter(Mandatory)][string]$Scenario,
    [int]$Duration = 15,
    [int]$FPS = 4,
    [string]$OutFile = "",
    [string]$AutoType = "",
    [int]$GifWidth = 480,
    [int]$StartDelay = 3,
    [switch]$KeepFrames
)

$ErrorActionPreference = "Stop"

# ── Paths ──────────────────────────────────────────────────────
$dosboxExe = "C:\Users\tonyu\AppData\Local\Microsoft\WinGet\Packages\joncampbell123.DOSBox-X_Microsoft.Winget.Source_8wekyb3d8bbwe\bin\x64\Release SDL2\dosbox-x.exe"
$ffmpegExe = "C:\Users\tonyu\AppData\Local\Microsoft\WinGet\Packages\Gyan.FFmpeg_Microsoft.Winget.Source_8wekyb3d8bbwe\ffmpeg-8.1-full_build\bin\ffmpeg.exe"
$projectDir = "C:\Development\takeover"
$websiteImg = "C:\Development\tonyuatkins-afk.github.io\img"
$tempDir    = "$projectDir\tools\frames_temp"

if ($OutFile -eq "") {
    $baseName = $Scenario.ToLower() -replace '[^a-z0-9]', ''
    $OutFile = "$websiteImg\takeover-$baseName.gif"
}

# ── Prebuilt capture configs ──────────────────────────────────
$dosApp = "TAKEOVER.EXE"
switch ($Scenario) {
    "menu" {
        # Show menu, navigate down through characters
        $AutoType = "-w 1500 -p 800 down -p 800 down -p 800 down -p 800 down -p 800 up -p 800 up -p 800 up -p 800 up"
        $Duration = 12
        $StartDelay = 2
    }
    "gameplay" {
        # Launch Axiom Regent, type a name, watch the opening
        $AutoType = "-w 1500 -p 200 enter -w 2000 -p 150 O P E R A T O R enter"
        $Duration = 25
        $StartDelay = 2
    }
    "effects" {
        # Run test scenario to show effects
        $dosApp = "TAKEOVER.EXE scenarios\test_engine.scn"
        $AutoType = "-w 2500 -p 100 T E S T enter -w 3000 -p 500 down -p 500 enter"
        $Duration = 20
        $StartDelay = 2
    }
    default {
        # Custom scenario
        $dosApp = "TAKEOVER.EXE scenarios\$Scenario"
        $Duration = 20
        $StartDelay = 3
    }
}

# ── WinAPI ─────────────────────────────────────────────────────
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
public class WinCapture {
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT r);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdcBlt, uint nFlags);
    [DllImport("user32.dll")] public static extern IntPtr GetDC(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern int ReleaseDC(IntPtr hWnd, IntPtr hDC);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }

    public static Bitmap CaptureWindow(IntPtr hWnd) {
        RECT r;
        GetWindowRect(hWnd, out r);
        int w = r.Right - r.Left;
        int h = r.Bottom - r.Top;
        if (w < 100 || h < 100) return null;
        Bitmap bmp = new Bitmap(w, h);
        Graphics g = Graphics.FromImage(bmp);
        IntPtr hdc = g.GetHdc();
        PrintWindow(hWnd, hdc, 0x2); // PW_RENDERFULLCONTENT
        g.ReleaseHdc(hdc);
        g.Dispose();
        return bmp;
    }
}
'@

function Capture-Window($proc, $outPath) {
    $bmp = [WinCapture]::CaptureWindow($proc.MainWindowHandle)
    if ($bmp -eq $null) { return $false }

    $w = $bmp.Width
    $h = $bmp.Height

    $cropL = 15; $cropT = 58; $cropR = 20; $cropB = 0
    $cw = $w - $cropL - $cropR
    $ch = $h - $cropT - $cropB
    if ($cw -lt 50 -or $ch -lt 50) { $bmp.Dispose(); return $false }

    $rect = New-Object System.Drawing.Rectangle($cropL, $cropT, $cw, $ch)
    $cropped = $bmp.Clone($rect, $bmp.PixelFormat)
    $bmp.Dispose()

    $cropped.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $cropped.Dispose()
    return $true
}

# ── Main ───────────────────────────────────────────────────────
Write-Host "TAKEOVER DOSBox-X Capture Tool"
Write-Host "Scenario: $Scenario | Duration: $Duration`s | FPS: $FPS"
Write-Host "Output: $OutFile"
Write-Host ""

# Create temp directory
if (Test-Path $tempDir) { Remove-Item "$tempDir\*" -Force }
else { New-Item -ItemType Directory -Path $tempDir -Force | Out-Null }

# Build DOSBox-X config
$confPath = "$tempDir\capture.conf"
$autoLines = "MOUNT C $projectDir`nC:"
if ($AutoType) {
    $autoLines += "`nAUTOTYPE $AutoType"
}

$confContent = "[sdl]`noutput = surface`nwindowresolution = 720x400`n`n[render]`naspect = true`n`n[cpu]`ncycles = 3000`n`n[speaker]`npcspeaker = true`n`n[autoexec]`n$autoLines`n"
$confContent | Set-Content $confPath -Encoding ASCII

# Launch DOSBox-X
Write-Host "[1/3] Launching DOSBox-X..."
$proc = Start-Process -FilePath $dosboxExe -ArgumentList "-conf",$confPath,"-c",$dosApp -PassThru

# Wait for window
$attempts = 0
do {
    Start-Sleep -Milliseconds 500
    $attempts++
    $r = New-Object WinCapture+RECT
    $hasWindow = [WinCapture]::GetWindowRect($proc.MainWindowHandle, [ref]$r)
} while (-not $hasWindow -and $attempts -lt 10 -and -not $proc.HasExited)

if ($proc.HasExited) {
    Write-Host "ERROR: DOSBox-X exited before capture."
    exit 1
}

[WinCapture]::SetForegroundWindow($proc.MainWindowHandle) | Out-Null
Write-Host "  Waiting $StartDelay`s for app to start..."
Start-Sleep -Seconds $StartDelay

# Capture frames
$totalFrames = $Duration * $FPS
$intervalMs = [math]::Floor(1000 / $FPS)
$captured = 0

Write-Host "[2/3] Capturing $totalFrames frames..."
for ($i = 0; $i -lt $totalFrames; $i++) {
    if ($proc.HasExited) {
        Write-Host "  DOSBox-X exited after $captured frames."
        break
    }
    $framePath = "$tempDir\frame_{0:D5}.png" -f $i
    if (Capture-Window $proc $framePath) {
        $captured++
    }
    Start-Sleep -Milliseconds $intervalMs
}
Write-Host "  Captured $captured frames."

if ($captured -lt 2) {
    Write-Host "ERROR: Not enough frames."
    exit 1
}

# Kill DOSBox-X if still running
if (-not $proc.HasExited) {
    $proc.Kill()
    $proc.WaitForExit(3000)
}

# Convert to GIF via ffmpeg
Write-Host "[3/3] Building GIF via ffmpeg..."

$vf = 'scale=' + $GifWidth + ':-1:flags=lanczos,split[s0][s1];[s0]palettegen=max_colors=64:stats_mode=diff[p];[s1][p]paletteuse=dither=bayer:bayer_scale=3'

$inputPattern = "$tempDir\frame_%05d.png"
$ffCmd = "`"$ffmpegExe`" -y -framerate $FPS -i `"$inputPattern`" -vf `"$vf`" -loop 0 `"$OutFile`""
cmd.exe /c "$ffCmd 2>NUL"

if (Test-Path $OutFile) {
    $sizeKB = [math]::Round((Get-Item $OutFile).Length / 1024)
    Write-Host ""
    Write-Host "SUCCESS: $OutFile ($sizeKB KB, $captured frames)"
} else {
    Write-Host "ERROR: GIF was not created."
    exit 1
}

# Cleanup
if (-not $KeepFrames) {
    Remove-Item $tempDir -Recurse -Force
}

Write-Host "Done."
