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

REM Copy executable
copy .\build\IPGui.exe .\IPGui\

REM Copy Qt and MinGW runtime DLLs
copy .\build\Qt6*.dll .\IPGui\
copy .\build\lib*.dll .\IPGui\

REM Copy DirectX/Vulkan DLLs
copy .\build\D3Dcompiler_47.dll .\IPGui\
copy .\build\dxcompiler.dll .\IPGui\
copy .\build\opengl32sw.dll .\IPGui\

REM Copy application resources
copy .\build\StdWorking.gif .\IPGui\
copy .\build\ipgui_logo.png .\IPGui\

REM Copy Qt plugin folders
xcopy /E /I /Y .\build\generic .\IPGui\generic
xcopy /E /I /Y .\build\iconengines .\IPGui\iconengines
xcopy /E /I /Y .\build\imageformats .\IPGui\imageformats
xcopy /E /I /Y .\build\networkinformation .\IPGui\networkinformation
xcopy /E /I /Y .\build\platforms .\IPGui\platforms
xcopy /E /I /Y .\build\styles .\IPGui\styles
xcopy /E /I /Y .\build\tls .\IPGui\tls

REM Copy translations
xcopy /E /I /Y .\build\translations .\IPGui\translations

REM Run the app
.\IPGui\IPGUI.exe