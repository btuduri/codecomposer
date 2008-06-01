call set_bccenv.bat
@echo off
cls
del *.obj
bcc32.exe -c -O2 -5 ttacenc_static.c
copy ttacenc_static.obj ..
pause
make.bat
