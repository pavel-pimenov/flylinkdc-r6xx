#requires -Version 5.1
<#
  build-asan.ps1 - собрать FlylinkDC++ с AddressSanitizer (Release|x64).

  Все проекты решения (включая вендоров) инструментируются /fsanitize=address
  через корневой Directory.Build.targets (активируется /p:FlyASan=true).
  Результат: compiled\flylinkdc_x64_asan.exe (+ .pdb) и скопированная рядом
  clang_rt.asan_dynamic-x86_64.dll. Промежуточные файлы — в подкаталогах asan\.

  Запуск:  powershell -NoProfile -ExecutionPolicy Bypass -File build-win\build-asan.ps1
           powershell -NoProfile -ExecutionPolicy Bypass -File build-win\build-asan.ps1 -Target Rebuild
#>
param(
  [ValidateSet('Build','Rebuild','Clean')][string]$Target = 'Build'
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$msb  = 'C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe'

$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$log   = Join-Path $repo ("build-win\build_asan_{0}_{1}.log" -f $Target.ToLower(), $stamp)

$msbArgs = @(
  'FlyLinkDC_2022.sln',
  "/t:$Target",
  '/p:Configuration=Release',
  '/p:Platform=x64',
  '/p:FlyASan=true',
  '/p:RunCodeAnalysis=false',
  '/m',
  '/v:m',
  '/clp:PerformanceSummary'
)

Push-Location $repo
$sw = [Diagnostics.Stopwatch]::StartNew()
& $msb @msbArgs *>&1 | Out-File $log -Encoding utf8
$code = $LASTEXITCODE
$sw.Stop()
Pop-Location

$text = Get-Content $log
$warn = ($text | Select-String -Pattern ': warning ').Count
$err  = ($text | Select-String -Pattern ': error ').Count

$exePath = Join-Path $repo 'compiled\flylinkdc_x64_asan.exe'
$size = if (Test-Path $exePath) { (Get-Item $exePath).Length } else { 0 }

$summary = '{0} | ASan {1} | {2:N2} min | W={3} E={4} | EXE={5} | exit={6}' -f `
  (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'), $Target, $sw.Elapsed.TotalMinutes, $warn, $err, $size, $code
Write-Host $summary
Write-Host ("log: {0}" -f $log)
