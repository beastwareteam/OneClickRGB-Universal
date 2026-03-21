@echo off
REM OneClickRGB-Universal Build Script
REM Requires Visual Studio 2019+ Build Tools

echo ============================================
echo OneClickRGB-Universal Build
echo ============================================

REM Find Visual Studio
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" 2>nul
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" 2>nul
)

REM Generate hardware config
echo.
echo [1/3] Generating hardware config...
cd ..\tools
python generate_config.py
if errorlevel 1 (
    echo ERROR: Config generation failed
    exit /b 1
)
cd ..\build

REM Compile
echo.
echo [2/3] Compiling...

set SOURCES=^
    ..\src\bridges\HIDBridge.cpp ^
    ..\src\bridges\SMBusBridge.cpp ^
    ..\src\devices\HIDDevice.cpp ^
    ..\src\devices\SMBusDevice.cpp ^
    ..\src\scanner\HardwareScanner.cpp ^
    ..\src\core\DeviceRegistry.cpp ^
    ..\src\plugins\asus\AsusAuraController.cpp ^
    ..\src\plugins\steelseries\SteelSeriesRival.cpp ^
    ..\src\plugins\evision\EVisionKeyboard.cpp ^
    ..\src\plugins\gskill\GSkillTridentZ5.cpp

set INCLUDES=/I..\src /I..\dependencies\hidapi
set LIBS=hidapi.lib shell32.lib user32.lib advapi32.lib

cl /nologo /EHsc /MD /O2 /W3 /std:c++17 /DUNICODE /D_UNICODE ^
    %INCLUDES% %SOURCES% /FeOneClickRGB-Universal.exe ^
    /link /LIBPATH:..\dependencies\hidapi %LIBS%

if errorlevel 1 (
    echo ERROR: Compilation failed
    exit /b 1
)

echo.
echo [3/3] Build complete!
echo Output: OneClickRGB-Universal.exe

exit /b 0
