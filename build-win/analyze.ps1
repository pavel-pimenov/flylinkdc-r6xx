#requires -Version 5.1
<#
  analyze.ps1 - run MSVC /analyze (code analysis) over the project and dump
  a compact, sortable list of findings to build-win\analysis\<name>.txt

  Usage:
    powershell -NoProfile -ExecutionPolicy Bypass -File build-win\analyze.ps1 -Name dbg -Project windows\flylinkdc_2022.vcxproj
    powershell -NoProfile -ExecutionPolicy Bypass -File build-win\analyze.ps1 -Name full

  Output format: <file>|<line>|<rule>|<message>
#>
param(
  [string]$Name = 'scan',
  [string]$Project = '',
  [ValidateSet('Debug','Release')][string]$Config = 'Debug'
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$msb  = 'C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe'
$outDir = Join-Path $repo 'build-win\analysis'
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Path $outDir | Out-Null }

$log = Join-Path $outDir ("analyze_{0}_{1}.log" -f $Name, $Config)
$txt = Join-Path $outDir ("analyze_{0}_{1}.txt" -f $Name, $Config)

$msbArgs = @()
if ($Project) { $msbArgs += (Join-Path $repo $Project) } else { $msbArgs += 'FlyLinkDC_2022.sln' }
$msbArgs += "/t:Rebuild"
$msbArgs += "/p:Configuration=$Config"
$msbArgs += '/p:Platform=x64'
$msbArgs += '/p:RunCodeAnalysis=true'
$msbArgs += '/p:EnablePREfast=true'
$msbArgs += '/m'
$msbArgs += '/v:m'

Push-Location $repo
$sw = [Diagnostics.Stopwatch]::StartNew()
& $msb @msbArgs *>&1 | Out-File $log -Encoding utf8
$code = $LASTEXITCODE
$sw.Stop()
Pop-Location

$found = New-Object System.Collections.Generic.List[string]
$rx = [regex]'(?<file>[A-Za-z]:\\[^\(]+?)\((?<line>\d+)\)\s*:\s*(?:warning|error)\s+(?<rule>C\d+)\s*:\s*(?<msg>.*?)(?:\s*\[.*\])?\s*$'
foreach ($l in (Get-Content -LiteralPath $log)) {
  $m = $rx.Match($l)
  if ($m.Success) {
    $found.Add(('{0}|{1}|{2}|{3}' -f $m.Groups['file'].Value, $m.Groups['line'].Value, $m.Groups['rule'].Value, $m.Groups['msg'].Value.Trim()))
  }
}
$found = $found | Sort-Object -Unique
$found | Set-Content -LiteralPath $txt -Encoding utf8

Write-Host ('{0} | analysis {1} {2} | {3:N2} min | findings={4} | exit={5}' -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'), $Name, $Config, $sw.Elapsed.TotalMinutes, $found.Count, $code)
$found | ForEach-Object { ($_ -split '\|')[2] } | Group-Object | Sort-Object Count -Descending | ForEach-Object { Write-Host ('  {0} {1}' -f $_.Count, $_.Name) }
