del *.exe
del *.obj

call "C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Auxiliary\Build\vcvars64.bat"

cl /D _CONSOLE /W4 /O2 shell.c sqlite3.c -Fesqlite3-x64.exe
