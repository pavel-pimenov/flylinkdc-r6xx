del /S /F /Q *.obj
del /S /F /Q *.pdb
del /S /F /Q *.lib
del /S /F /Q tmp32dll\*
del /S /F /Q out32dll\*

rem call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"
call "C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Auxiliary\Build\vcvars64.bat"

set PATH=%PATH%;"C:\Program Files (x86)\Windows Kits\8.1\bin\x86"
rem https://github.com/Microsoft/vcpkg/issues/1689

perl Configure VC-WIN64A no-asm no-shared --prefix=c:\openssl-1-1-1
"C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.31.30818\bin\Hostx64\x64\nmake.exe" -f makefile
