@echo off
rmdir /s /q build
cmake -S . -B build
cmake --build build --verbose --config Debug
C:\Qt\6.9.1\msvc2022_64\bin\windeployqt.exe --qmldir .\build\Debug .\build\Debug\IPGUI.exe
.\build\Debug\IPGUI.exe