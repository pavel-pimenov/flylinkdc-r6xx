del *.exe
del *.asm
del *.obj

call "%VS140COMNTOOLS%\..\..\VC\bin\vcvars32.bat"

cl /D _CONSOLE /W4 /O2 /FAs test_memset.cpp

