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
set "DEV_ARCH=x86"
echo.%PRESET%| findstr /I "x64" >nul
if not errorlevel 1 set "DEV_ARCH=x64"

echo Generating project files with configure preset "%PRESET%"...
call "%ROOT%\setup_ev.bat" %DEV_ARCH% >nul
if errorlevel 1 exit /b %errorlevel%

cmake --preset "%PRESET%"
exit /b %errorlevel%

:usage
echo Usage: generate_project.bat [configure-preset]
echo.
echo Common configure presets:
echo   vs2026-win32
echo   vs2026-x64
echo   vs2026-x64-dedicated
echo   vs2026-x64-editor
echo   ninja-debug
echo.
echo Use "generate_project.bat list" to show every configure preset.
exit /b 0
