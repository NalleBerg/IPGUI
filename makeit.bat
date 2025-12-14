@echo off
cls
REM Remove old build directory
rmdir /s /q build

REM Configure the project for Visual Studio 2022 x64, with Qt6 CMake path
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64\lib\cmake"

REM Build in Release mode
cmake --build build --config Release --verbose

REM Deploy Qt dependencies
C:\Qt\6.9.1\msvc2022_64\bin\windeployqt.exe .\build\Release\IPGUI.exe

REM Copying Standard Working gif to the build directory
copy .\StdWorking.gif .\build\Release\StdWorking.gif

REM Renaming the build/Release directory to IPGui
ren .\build\Release IPGui

REM Run the app
.\build\IPGui\IPGUI.exe