param(
  [string]$Exe = "F:\src-git\flylinkdc-r6xx\compiled\flylinkdc_x64.exe",
  [string]$OutDir = "F:\src-git\flylinkdc-r6xx\build-win\shots",
  [int]$WaitSeconds = 15
)

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public class Win2 {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowTextW(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassNameW(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr parent, EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  public static List<IntPtr> Top(uint pid) {
    List<IntPtr> res = new List<IntPtr>();
    EnumWindows(delegate(IntPtr h, IntPtr p) {
      uint p2; GetWindowThreadProcessId(h, out p2);
      if (p2 == pid) res.Add(h);
      return true;
    }, IntPtr.Zero);
    return res;
  }
  public static List<IntPtr> Children(IntPtr parent) {
    List<IntPtr> res = new List<IntPtr>();
    EnumChildWindows(parent, delegate(IntPtr h, IntPtr p) { res.Add(h); return true; }, IntPtr.Zero);
    return res;
  }
  public static string Title(IntPtr h) { StringBuilder s = new StringBuilder(512); GetWindowTextW(h, s, 512); return s.ToString(); }
  public static string Cls(IntPtr h) { StringBuilder s = new StringBuilder(128); GetClassNameW(h, s, 128); return s.ToString(); }
}
"@ -ErrorAction SilentlyContinue

if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Path $OutDir | Out-Null }

function Save-Screen([string]$Name)
{
    $b = [System.Windows.Forms.SystemInformation]::VirtualScreen
    $bmp = New-Object System.Drawing.Bitmap($b.Width, $b.Height)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($b.Left, $b.Top, 0, 0, $bmp.Size)
    $path = Join-Path $OutDir $Name
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bmp.Dispose()
    Write-Host "saved $path"
}

$proc = Start-Process -FilePath $Exe -PassThru
Write-Host "started pid=$($proc.Id)"
Start-Sleep -Seconds $WaitSeconds

# close the "automatic update" dialog: click the "later" button
foreach ($h in [Win2]::Top([uint32]$proc.Id)) {
    $t = [Win2]::Title($h)
    if ($t -like "*обновление*") {
        Write-Host "update dialog: $t"
        foreach ($c in [Win2]::Children($h)) {
            $ct = [Win2]::Title($c)
            if ($ct -ne "") { Write-Host ("   child: '{0}' class={1}" -f $ct, [Win2]::Cls($c)) }
        }
        foreach ($c in [Win2]::Children($h)) {
            if ([Win2]::Title($c) -eq "Позже") {
                Write-Host "clicking 'later'"
                [Win2]::PostMessage($c, 0x00F5, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
            }
        }
    }
}
Start-Sleep -Seconds 4

$main = [IntPtr]::Zero
foreach ($h in [Win2]::Top([uint32]$proc.Id)) {
    $t = [Win2]::Title($h)
    $c = [Win2]::Cls($h)
    Write-Host ("window: {0} class={1}" -f $t, $c)
    if ($t -like "*FlylinkDC++*" -and $c -notlike "*#32770*") { $main = $h }
}
Write-Host "main window: $main"

if ($main -ne [IntPtr]::Zero) {
    [Win2]::ShowWindow($main, 3) | Out-Null
    [Win2]::SetForegroundWindow($main) | Out-Null
    Start-Sleep -Seconds 2
    Save-Screen "01_main.png"

    # open the settings dialog
    [Win2]::PostMessage($main, 0x0111, [IntPtr]32778, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Seconds 5
    Save-Screen "02_settings.png"

    foreach ($h in [Win2]::Top([uint32]$proc.Id)) {
        Write-Host ("after: {0} class={1}" -f [Win2]::Title($h), [Win2]::Cls($h))
    }
}
else {
    Save-Screen "00_nomain.png"
}

Write-Host "done"
