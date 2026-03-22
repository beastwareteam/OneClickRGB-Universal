@echo off
REM =============================================================================
REM OneClickRGB-Universal Build Script
REM =============================================================================
REM Prerequisites:
REM   - Visual Studio 2019+ Build Tools (cl.exe in PATH)
REM   - Or run from "Developer Command Prompt for VS"
REM =============================================================================

setlocal enabledelayedexpansion

cd /d "%~dp0"
cd ..

echo ========================================
echo OneClickRGB-Universal Build
echo ========================================
echo.

REM Check for compiler
where cl >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo ERROR: cl.exe not found. Run from Visual Studio Developer Command Prompt.
    exit /b 1
)

REM Create output directory
if not exist "build\bin" mkdir "build\bin"

echo [1/3] Generating hardware config...
cd tools
python generate_config.py
if %ERRORLEVEL% neq 0 (
    echo WARNING: Config generation failed, using existing config.
)
cd ..

echo.
echo [2/3] Building main application...

set SOURCES=^
    src\main.cpp ^
    src\OneClickRGB.cpp ^
    src\core\DeviceRegistry.cpp ^
    src\scanner\HardwareScanner.cpp ^
    src\plugins\PluginFactory.cpp ^
    src\plugins\asus\AsusAuraController.cpp ^
    src\plugins\steelseries\SteelSeriesRival.cpp ^
    src\plugins\evision\EVisionKeyboard.cpp ^
    src\plugins\gskill\GSkillTridentZ5.cpp ^
    src\devices\HIDDevice.cpp ^
    src\devices\SMBusDevice.cpp ^
    src\bridges\HIDBridge.cpp ^
    src\bridges\SMBusBridge.cpp ^
    src\app\config\DeviceConfiguration.cpp ^
    src\app\config\ConfigBundleParser.cpp ^
    src\app\pipeline\DevicePipeline.cpp ^
    src\app\effects\EffectFactory.cpp ^
    src\app\services\DeviceService.cpp ^
    src\app\services\ProvisioningService.cpp ^
    src\app\services\ProfileResolver.cpp ^
    src\app\fingerprint\MachineFingerprint.cpp

set INCLUDES=/I"src" /I"build\generated" /I"dependencies\hidapi"
set LIBS=setupapi.lib hid.lib
set OUTPUT=/Fe"build\bin\oneclickrgb.exe"
set FLAGS=/EHsc /O2 /MT /W3

cl %FLAGS% %INCLUDES% %SOURCES% %LIBS% %OUTPUT% /link /SUBSYSTEM:CONSOLE

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Build failed!
    exit /b 1
)

echo.
echo [3/3] Building tests...

set TEST_SOURCES=tests\test_main.cpp ^
    src\core\DeviceRegistry.cpp ^
    src\app\config\DeviceConfiguration.cpp ^
    src\app\config\ConfigBundleParser.cpp ^
    src\app\effects\EffectFactory.cpp ^
    src\app\pipeline\DevicePipeline.cpp

set TEST_OUTPUT=/Fe"build\bin\test_runner.exe"

cl %FLAGS% /I"src" %TEST_SOURCES% %TEST_OUTPUT% /link /SUBSYSTEM:CONSOLE

if %ERRORLEVEL% neq 0 (
    echo WARNING: Test build failed, continuing...
)

echo.
echo ========================================
echo Build complete!
echo ========================================
echo.
echo Output:
echo   build\bin\oneclickrgb.exe
echo   build\bin\test_runner.exe
echo.
echo Run tests:
echo   build\bin\test_runner.exe
echo.

exit /b 0
