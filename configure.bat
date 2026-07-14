@echo off
setlocal EnableExtensions

if /I "%~1"=="--help" goto usage
if /I "%~1"=="-h" goto usage
if /I "%~1"=="help" goto usage
if /I "%~1"=="list" (
    cmake --list-presets
    exit /b %errorlevel%
)

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=vs2026-win32"

echo Selected configure preset: %PRESET%
call "%ROOT%\generate_project.bat" "%PRESET%"
exit /b %errorlevel%

:usage
echo Usage: configure.bat [configure-preset]
echo.
echo Selects a CMake configure preset and generates that build directory.
echo The default preset is vs2026-win32, matching the original Doom 3 game layout.
echo.
echo Common configure presets:
echo   vs2026-win32
echo   vs2026-x64
echo   vs2026-x64-dedicated
echo   vs2026-x64-editor
echo   ninja-debug
echo.
echo Use "configure.bat list" to show every configure preset.
exit /b 0
