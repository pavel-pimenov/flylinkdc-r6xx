del *.exe
del *.obj

rem call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"

cl /D _CONSOLE /W4 /O2 /IC:\vc17\r6xx\boost speed-compare.cpp -Fespeed-compare.exe
rem cl speed-compare.cpp /EHsc /link  /out:speed-compare.exe 
rem  -Fespeed-compare.exe

