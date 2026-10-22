param(
  [Parameter(Mandatory=$true)][string]$Image,
  [string]$Points = ""
)
Add-Type -AssemblyName System.Drawing
$bmp = [System.Drawing.Bitmap]::FromFile($Image)
Write-Host ("image: {0} {1}x{2}" -f $Image, $bmp.Width, $bmp.Height)
foreach ($p in ($Points -split ';' | Where-Object { $_ -ne "" })) {
  $xy = $p -split ','
  $x = [int]$xy[0]; $y = [int]$xy[1]
  if ($x -lt 0 -or $y -lt 0 -or $x -ge $bmp.Width -or $y -ge $bmp.Height) { Write-Host ("({0},{1}) out of range" -f $x, $y); continue }
  $c = $bmp.GetPixel($x, $y)
  Write-Host ("({0},{1}) = #{2:X2}{3:X2}{4:X2}" -f $x, $y, $c.R, $c.G, $c.B)
}
$bmp.Dispose()
