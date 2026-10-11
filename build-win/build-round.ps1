#requires -Version 5.1
<#
  build-round.ps1 - run one MSBuild round and append metrics to build-win\build-stats.csv

  Usage:
    powershell -NoProfile -ExecutionPolicy Bypass -File build-win\build-round.ps1 -Config Release -Target Rebuild -Notes "full LTCG"
    powershell -NoProfile -ExecutionPolicy Bypass -File build-win\build-round.ps1 -Config Debug -Target Build -Analysis false -Notes "fast iterate"

  Columns: timestamp,config,target,scope,minutes,warnings,errors,exe_bytes,exit,notes
#>
param(
  [ValidateSet('Debug','Release')][string]$Config = 'Release',
  [ValidateSet('Build','Rebuild','Clean')][string]$Target = 'Build',
  [ValidateSet('true','false','')][string]$Analysis = '',
  [string]$Project = '',
  [string]$Notes = ''
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$msb  = 'C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe'

$scope = if ($Project) { $Project } else { 'FlyLinkDC_2022.sln' }
$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$log   = Join-Path $repo ("build-win\build_{0}_{1}_{2}.log" -f $Config, $Target.ToLower(), $stamp)

$msbArgs = @()
if ($Project) {
  $msbArgs += (Join-Path $repo $Project)
} else {
  $msbArgs += 'FlyLinkDC_2022.sln'
}
$msbArgs += "/t:$Target"
$msbArgs += "/p:Configuration=$Config"
$msbArgs += '/p:Platform=x64'
$msbArgs += '/m'
$msbArgs += '/v:m'
$msbArgs += '/clp:PerformanceSummary'
if ($Analysis -eq 'true')  { $msbArgs += '/p:RunCodeAnalysis=true' }
if ($Analysis -eq 'false') { $msbArgs += '/p:RunCodeAnalysis=false' }

Push-Location $repo
$sw = [Diagnostics.Stopwatch]::StartNew()
& $msb @msbArgs *>&1 | Out-File $log -Encoding utf8
$code = $LASTEXITCODE
$sw.Stop()
Pop-Location

$text = Get-Content $log
$warn = ($text | Select-String -Pattern ': warning ').Count
$err  = ($text | Select-String -Pattern ': error ').Count

$exeName = if ($Config -eq 'Release') { 'FlyLinkDC_x64.exe' } else { 'FlyLinkDC_Debug_x64.exe' }
$exePath = Join-Path $repo ("compiled\{0}" -f $exeName)
$size = if (Test-Path $exePath) { (Get-Item $exePath).Length } else { 0 }

$ts  = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'
$csv = Join-Path $repo 'build-win\build-stats.csv'
$row = '{0},{1},{2},{3},{4:N2},{5},{6},{7},{8},{9}' -f $ts, $Config, $Target, $scope, $sw.Elapsed.TotalMinutes, $warn, $err, $size, $code, $Notes
Add-Content -Path $csv -Value $row -Encoding utf8

$summary = '{0} | {1} {2} ({3}) | {4:N2} min | W={5} E={6} | EXE={7} | exit={8}' -f $ts, $Config, $Target, $scope, $sw.Elapsed.TotalMinutes, $warn, $err, $size, $code
Write-Host $summary
