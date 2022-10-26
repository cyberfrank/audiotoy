@echo off

if "%~1"=="debug" goto debug
if "%~1"=="clean" goto clean

:release
set target=release
goto build

:debug
set target=debug
goto build

:clean
echo Cleaning object files
rmdir bin\\obj /s /q
goto exit

:build
echo Starting %target% mode build...
premake5 vs2019 --verbose
msbuild bin/workspace.sln -p:Configuration=%target% -p:Platform=Win64 -p:GenerateFullPaths=true -t:build -v:normal -maxcpucount:8 -nologo 

:exit
echo Done!
