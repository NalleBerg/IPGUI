@echo off
cls
REM Remove old build directory
rmdir /s /q build

REM Configure the project for MinGW64, with Qt6 CMake path
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\mingw_64\lib\cmake" -DCMAKE_BUILD_TYPE=Release

REM Build in Release mode
cmake --build build --verbose

REM Deploy Qt dependencies
C:\Qt\6.9.1\mingw_64\bin\windeployqt.exe .\build\IPGUI.exe

REM Copying Standard Working gif and logo PNG to the build directory
copy .\StdWorking.gif .\build\StdWorking.gif
copy .\ipgui_logo.png .\build\ipgui_logo.png

REM Create clean IPGui distribution folder
if exist IPGui rmdir /s /q IPGui
mkdir IPGui
xcopy /E /I /Y .\build\* .\IPGui\

REM Run the app
.\IPGui\IPGUI.exe