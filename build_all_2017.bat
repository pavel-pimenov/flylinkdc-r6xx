echo on
call clean_all_2017.bat
call update_geo_ip.bat
call build_flylinkdc_x64_2017.bat

if not exist .\compiled\FlylinkDC*.exe goto :builderror

copy FlylinkDC-*-x64-*build-*-*.7z "O:\flylinkdc\r6xx\x64"
move *-debug-info-*.7z "O:\flylinkdc-src"
move *-src-*.7z "O:\flylinkdc-src"
move *.7z "O:\flylinkdc-src"

cd setup 
rem call build_setup_base.bat
cd ..
cd compiled
call SymRegisterBinaries.bat
goto:end

:builderror
echo Compilation error. Building terminated.
pause

:end
