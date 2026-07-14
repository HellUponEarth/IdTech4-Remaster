@echo off
setlocal EnableExtensions

if /I "%~1"=="--help" goto usage
if /I "%~1"=="-h" goto usage
if /I "%~1"=="help" goto usage
if /I "%~1"=="list" (
    cmake --list-presets=build
    exit /b %errorlevel%
)

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

if "%~1"=="" goto build_original_layout
if /I "%~1"=="original" goto build_original_layout
if /I "%~1"=="all" goto build_original_layout

set "BUILD_PRESET=%~1"
echo Building preset "%BUILD_PRESET%"...
cmake --build --preset "%BUILD_PRESET%"
exit /b %errorlevel%

:build_original_layout
echo Generating original Doom 3 Win32 project layout...
call "%ROOT%\generate_project.bat" vs2026-win32
if errorlevel 1 exit /b %errorlevel%

echo Building Doom3.exe and game DLLs...
cmake --build --preset vs2026-win32-release
if errorlevel 1 exit /b %errorlevel%

echo Building DedServer.exe...
cmake --build --preset vs2026-win32-dedicated-release
if errorlevel 1 exit /b %errorlevel%

echo.
echo Expected runtime outputs:
echo   %ROOT%\Doom3.exe
echo   %ROOT%\DedServer.exe
echo   %ROOT%\base\gamex86.dll
echo.
exit /b 0

:usage
echo Usage: build.bat [build-preset^|original^|all]
echo.
echo With no arguments, builds the original Doom 3 layout:
echo   Doom3.exe      - repo root
echo   DedServer.exe  - repo root
echo   gamex86.dll    - base\
echo.
echo Common build presets:
echo   vs2026-win32-release
echo   vs2026-win32-dedicated-release
echo   vs2026-x64-release
echo   vs2026-x64-dedicated-release
echo   ninja-release
echo.
echo Use "build.bat list" to show every build preset.
exit /b 0
