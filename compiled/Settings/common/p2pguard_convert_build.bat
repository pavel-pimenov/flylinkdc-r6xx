del p2pguard_convert.exe

call "C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Auxiliary\Build\vcvars64.bat"

cl p2pguard_convert.cpp /EHsc /O2 -I ..\..\..\boost
