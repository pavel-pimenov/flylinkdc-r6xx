@echo off
rem ---------------------------------------------------------------------------
rem  run-asan.cmd - запуск ASan-сборки FlylinkDC++ (compiled\flylinkdc_x64_asan.exe)
rem
rem  - continue_on_error: приложение продолжает работу после ошибки памяти, а
rem    уникальные отчёты пишутся в лог (удобно для нагрузочного теста);
rem  - windows_fast_fail_on_error=false: не завершать процесс через __fastfail;
rem  - COE_LOG_FILE: файл, куда складываются найденные ошибки.
rem ---------------------------------------------------------------------------
setlocal
set "ROOT=%~dp0.."
set "LOGDIR=%ROOT%\build-win\asan-logs"
if not exist "%LOGDIR%" mkdir "%LOGDIR%"

set "ASAN_OPTIONS=continue_on_error=1:windows_fast_fail_on_error=false:alloc_dealloc_mismatch=1"
set "COE_LOG_FILE=%LOGDIR%\asan_errors.log"

echo ASAN_OPTIONS=%ASAN_OPTIONS%
echo COE_LOG_FILE=%COE_LOG_FILE%
echo launching %ROOT%\compiled\flylinkdc_x64_asan.exe
echo.

"%ROOT%\compiled\flylinkdc_x64_asan.exe" %*
set "RC=%errorlevel%"
echo.
echo exit code: %RC%
endlocal
