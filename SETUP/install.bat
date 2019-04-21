@echo off

vcls /B 0x01 /F 0x0f /G
vcursor HIDE

REM copy the files
:copy_files
vecho BirdOS Setup
vecho ============
vecho
vframe /W30 /H4 /B0x00 /F0x07 shadow /C
vecho Copying files...
REM copy boot.zip C:\boot.zip
mkdir C:\BIRDOS 
copy D:\VESA.SYS C:\BIRDOS\VESA.SYS
copy D:\TASK1.SYS C:\BIRDOS\TASK1.SYS
copy D:\TASK2.SYS C:\BIRDOS\TASK2.SYS
vdelay 2000
