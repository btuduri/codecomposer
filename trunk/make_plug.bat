	@echo off
call setenv_devkitPror20.bat

:loop
cls
goto skipclean
rd /q /s arm9\build\

:skipclean

del arm9\arm9.bin
del arm9\arm9.so
del arm9\arm9.map
del arm9\specs\ds_arm9_mshlplg_crt0.o

del arm9\arm9_sort.map
del arm9\arm9_objdump_*.txt

make
if exist mspv21_midrcp.arm9 goto run
pause
goto loop

:run

copy mspv21_midrcp.arm9 midrcp.msp
del mspv21_midrcp.*

rem copy midrcp.msp D:\MyDocuments\NDS\MoonShell\files_EXFS\shell\plugin\midrcp.msp
copy midrcp.msp P:\moonshl\plugin\midrcp.msp

echo $ > D:\MyDocuments\NDS\MoonShell\safeeject_req.$$$

pause
goto loop

