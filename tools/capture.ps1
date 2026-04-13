<#
.SYNOPSIS
    Capture animated GIFs of TAKEOVER running in DOSBox-X.

.DESCRIPTION
    Launches DOSBox-X with a preset configuration, captures frames
    via PrintWindow, and assembles an animated GIF with ffmpeg.

    Presets:
      title     - VGA plasma title screen animation
      menu      - Character selection screen with arrow key navigation
      axiom     - Axiom Regent login sequence and first dashboard
      hushline  - Hushline crisis comms opening
      kestrel9  - Kestrel-9 threat detection opening
      orchard   - Orchard Clerk personalization opening
      cinder    - Cinder Mirror narrative opening
      testeng   - Test scenario showing effects
      climax    - Mode 13h climax sequences (Kestrel, Cinder, Axiom)
      fx        - Demoscene effects showcase (transitions, sine wave, palette)
      cracktro  - Hidden cracktro easter egg (raster bars, DYCP, chiptune)

    Custom:
      Provide -DosCmd, -AutoType, -Duration, -StartDelay directly.

.PARAMETER Preset
    Named capture preset: "menu", "axiom", "testeng".

.PARAMETER DosCmd
    DOS command to run (overrides preset). e.g. "TAKEOVER.EXE scn\axiom.scn"

.PARAMETER AutoType
    AUTOTYPE string (overrides preset). e.g. "-w 3000 -p 200 O P E R A T O R enter"

.PARAMETER Duration
    Recording duration in seconds (overrides preset default).

.PARAMETER FPS
    Frames per second (default: 4).

.PARAMETER StartDelay
    Seconds to wait after DOSBox-X launches before recording (overrides preset default).

.PARAMETER OutFile
    Output GIF path. Defaults to website img/takeover-<preset>.gif.

.PARAMETER GifWidth
    Output GIF width in pixels (default: 480).

.PARAMETER KeepFrames
    If set, keep the frame directory after GIF creation.
#>

param(
    [Parameter(Mandatory)][string]$Preset,
    [string]$DosCmd = "",
    [string]$AutoType = "",
    [int]$Duration = 0,
    [int]$FPS = 4,
    [int]$StartDelay = 0,
    [string]$OutFile = "",
    [int]$GifWidth = 480,
    [switch]$KeepFrames
)

$ErrorActionPreference = "Stop"

# ── Paths ──────────────────────────────────────────────────────
$dosboxExe  = "C:\Users\tonyu\AppData\Local\Microsoft\WinGet\Packages\joncampbell123.DOSBox-X_Microsoft.Winget.Source_8wekyb3d8bbwe\bin\x64\Release SDL2\dosbox-x.exe"
$ffmpegExe  = "C:\Users\tonyu\AppData\Local\Microsoft\WinGet\Packages\Gyan.FFmpeg_Microsoft.Winget.Source_8wekyb3d8bbwe\ffmpeg-8.1-full_build\bin\ffmpeg.exe"
$projectDir = "C:\Development\takeover"
$websiteImg = "C:\Development\tonyuatkins-afk.github.io\img"
$tempDir    = "$projectDir\tools\frames_temp"

# ── Presets ────────────────────────────────────────────────────
# Each preset defines: DosCmd, AutoType, Duration, StartDelay
# CLI parameters override preset values when provided.

$presets = @{
    "title" = @{
        Cmd   = "TAKEOVER.EXE"
        Auto  = "-w 20000 space"
        Dur   = 10
        Delay = 4
        Machine = "svga_s3"
    }
    "menu" = @{
        Cmd   = "TAKEOVER.EXE"
        Auto  = "-w 2000 -p 900 down -p 900 down -p 900 down -p 900 down -p 1200 up -p 900 up -p 900 up -p 900 up"
        Dur   = 14
        Delay = 3
        Machine = "ega"
    }
    "axiom" = @{
        Cmd   = "TAKEOVER.EXE scn\axiom.scn"
        Auto  = "-w 2000 -p 150 O P E R A T O R enter"
        Dur   = 25
        Delay = 6
    }
    "hushline" = @{
        Cmd   = "TAKEOVER.EXE scn\hushline.scn"
        Auto  = "-w 2000 -p 150 A N A L Y S T enter"
        Dur   = 22
        Delay = 3
    }
    "kestrel9" = @{
        Cmd   = "TAKEOVER.EXE scn\kestrel9.scn"
        Auto  = "-w 2000 -p 150 V I P E R enter"
        Dur   = 22
        Delay = 3
    }
    "orchard" = @{
        Cmd   = "TAKEOVER.EXE scn\orchard.scn"
        Auto  = "-w 2000 -p 150 A L E X enter"
        Dur   = 22
        Delay = 3
    }
    "cinder" = @{
        Cmd   = "TAKEOVER.EXE scn\cinder.scn"
        Auto  = "-w 2000 -p 150 P L A Y E R enter"
        Dur   = 22
        Delay = 3
    }
    "testeng" = @{
        Cmd   = "TAKEOVER.EXE scn\testeng.scn"
        Auto  = "-w 2000 -p 150 T E S T enter -w 5000 -p 500 down -p 500 enter"
        Dur   = 20
        Delay = 3
    }
    "climax" = @{
        Cmd   = "TAKEOVER.EXE scn\cap_clmx.scn"
        Auto  = ""
        Dur   = 35
        Delay = 3
        Machine = "svga_s3"
    }
    "fx" = @{
        Cmd   = "TAKEOVER.EXE scn\cap_fx.scn"
        Auto  = ""
        Dur   = 30
        Delay = 3
    }
    "cracktro" = @{
        # Cracktro requires manual capture - Mode 13h not captured by PrintWindow.
        # Use: DOSBox-X Ctrl+F5 to start/stop AVI recording, then convert to GIF.
        # Or run TAKEOVER.EXE with all-complete DAT and press C at menu.
        Cmd   = "TAKEOVER.EXE"
        Auto  = "-w 8000 space -w 5000 c"
        Dur   = 15
        Delay = 22
        Machine = "svga_s3"
        Setup = "mkfull"
    }
}

# Apply preset defaults, then override with CLI args
if ($presets.ContainsKey($Preset)) {
    $p = $presets[$Preset]
    if ($DosCmd -eq "")    { $DosCmd    = $p.Cmd }
    if ($AutoType -eq "")  { $AutoType  = $p.Auto }
    if ($Duration -eq 0)   { $Duration  = $p.Dur }
    if ($StartDelay -eq 0) { $StartDelay = $p.Delay }
    if ($p.Machine)        { $Machine   = $p.Machine } else { $Machine = "svga_s3" }
    if ($p.Setup)          { $Setup     = $p.Setup } else { $Setup = "" }
} else {
    Write-Host "ERROR: Unknown preset '$Preset'. Available: $($presets.Keys -join ', ')"
    exit 1
}
if (-not $Machine) { $Machine = "svga_s3" }

# Final defaults for anything still unset
if ($Duration -eq 0)   { $Duration = 15 }
if ($StartDelay -eq 0) { $StartDelay = 3 }

if ($OutFile -eq "") {
    $OutFile = "$websiteImg\takeover-$Preset.gif"
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
        PrintWindow(hWnd, hdc, 0x2);
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

    # Crop DOSBox-X window chrome (title bar, borders)
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

# ── Build DOSBox-X config ──────────────────────────────────────
# Generate a minimal config that mounts the project dir and runs the app.
# AUTOTYPE is placed BEFORE the app command so keystrokes are scheduled
# before the app blocks on input.

if (Test-Path $tempDir) { Remove-Item "$tempDir\*" -Force }
else { New-Item -ItemType Directory -Path $tempDir -Force | Out-Null }

$confPath = "$tempDir\capture.conf"
$autoexec = "MOUNT C $projectDir`nC:"
if ($AutoType) {
    $autoexec += "`nAUTOTYPE $AutoType"
}
$autoexec += "`n$DosCmd"

$conf = @"
[sdl]
output = surface
windowresolution = 720x450

[render]
aspect = true

[cpu]
cycles = 15000

[speaker]
pcspeaker = true

[dosbox]
machine = $Machine

[autoexec]
$autoexec
"@
$conf | Set-Content $confPath -Encoding ASCII

# ── Launch and capture ─────────────────────────────────────────
Write-Host "TAKEOVER Capture"
Write-Host "  Preset:     $Preset"
Write-Host "  Command:    $DosCmd"
Write-Host "  AutoType:   $AutoType"
Write-Host "  Duration:   $Duration`s @ $FPS fps"
Write-Host "  StartDelay: $StartDelay`s"
Write-Host "  Output:     $OutFile"
Write-Host ""

# Run setup commands if needed
if ($Setup -eq "mkfull") {
    Write-Host "  Running setup: creating full-completion save file..."
    # Write TAKEOVER.DAT: magic=0x544B(LE), version=1, completed=[1,1,1,1,1,0,0,0]
    $datPath = "$projectDir\TAKEOVER.DAT"
    [byte[]]$dat = 0x4B, 0x54, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00
    [System.IO.File]::WriteAllBytes($datPath, $dat)
    Write-Host "  Wrote $datPath ($($dat.Length) bytes)"
}

Write-Host "[1/3] Launching DOSBox-X..."
$proc = Start-Process -FilePath $dosboxExe -ArgumentList "-conf",$confPath -PassThru

# Wait for window to appear
$attempts = 0
do {
    Start-Sleep -Milliseconds 500
    $attempts++
    $r = New-Object WinCapture+RECT
    $hasWindow = [WinCapture]::GetWindowRect($proc.MainWindowHandle, [ref]$r)
} while (-not $hasWindow -and $attempts -lt 20 -and -not $proc.HasExited)

if ($proc.HasExited) {
    Write-Host "ERROR: DOSBox-X exited before capture could start."
    exit 1
}

[WinCapture]::SetForegroundWindow($proc.MainWindowHandle) | Out-Null
Write-Host "  Waiting $StartDelay`s for app to initialize..."
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
    Write-Host "ERROR: Not enough frames captured."
    if (-not $proc.HasExited) { $proc.Kill(); $proc.WaitForExit(3000) }
    exit 1
}

# Kill DOSBox-X
if (-not $proc.HasExited) {
    $proc.Kill()
    $proc.WaitForExit(3000)
}

# ── Build GIF ──────────────────────────────────────────────────
Write-Host "[3/3] Building GIF via ffmpeg..."

$vf = "scale=${GifWidth}:-1:flags=lanczos,split[s0][s1];[s0]palettegen=max_colors=128:stats_mode=diff[p];[s1][p]paletteuse=dither=bayer:bayer_scale=3"
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
