@echo off

vcls /B 0x01 /F 0x0f /G
vcursor HIDE

REM The title screen.
:title_screen
vecho BirdOS Setup
vecho ============
vecho
vecho Thank you for choosing BirdOS! It is currently in PRE release and we do not
vecho recommend you to install this software on your main computer.
vecho
vecho Would you like to proceed?
vframe /X0x00 /Y0x01 /H0x05 /F0x0f optionbox hidden /C 
vecho Continue with installation
vecho End setup and shutdown
vchoice /B 0x00 /D 2

if errorlevel 1 goto t%errorlevel%
if errorlevel 2 

:t2
vcls /B0x00 /F0x07 /G
vecho Shutting down...
shutdown

:t1
vcls /B0x01 /F0x0f /G

vecho BirdOS Setup
vecho ============
vecho
vecho Before BirdOS can be installed you must first have 
vecho installed FreeDOS on your computer.
vecho
vecho Recommaneded version: FreeDOS 1.2

goto d2
REM vecho Before BirdOS can be installed the harddrive should be formatted.
REM vframe /H0x05  optionbox hidden /C
REM vecho Format drive D:
REM vecho End setup and return to DOS
REM vchoice /B0x00 /D2

REM goto d%errorlevel%

:d1
vframe /W30 /H4 /B0x00 /F0x07 shadow /C
vecho Formatting D:...
REM format D /V:BIRDOS
vdelay 5000

:d2
call D:\install.bat %0


:hi
vecho hidden

:d2
vcls /B 0x00 /F 0x07
exit