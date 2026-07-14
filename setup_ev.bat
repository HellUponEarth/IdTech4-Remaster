@echo off
rem Prepares a Visual Studio developer command environment for this checkout.
rem Usage: setup_ev.bat [x86^|x64^|Win32]

set "IDTECH4_ROOT=%~dp0"
if "%IDTECH4_ROOT:~-1%"=="\" set "IDTECH4_ROOT=%IDTECH4_ROOT:~0,-1%"

set "IDTECH4_ARCH=%~1"
if "%IDTECH4_ARCH%"=="" set "IDTECH4_ARCH=x86"
if /I "%IDTECH4_ARCH%"=="Win32" set "IDTECH4_ARCH=x86"

if /I not "%IDTECH4_ARCH%"=="x86" if /I not "%IDTECH4_ARCH%"=="x64" (
    echo Unsupported architecture "%IDTECH4_ARCH%". Use x86, x64, or Win32.
    exit /b 2
)

set "IDTECH4_HOST_ARCH=x64"
set "IDTECH4_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "IDTECH4_VSINSTALL="

if exist "%ProgramFiles%\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" set "IDTECH4_VSINSTALL=%ProgramFiles%\Microsoft Visual Studio\18\Community"
if not defined IDTECH4_VSINSTALL if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" set "IDTECH4_VSINSTALL=%ProgramFiles(x86)%\Microsoft Visual Studio\18\Community"

if defined IDTECH4_VSINSTALL goto found_vs
if not exist "%IDTECH4_VSWHERE%" goto found_vs
for /f "usebackq delims=" %%I in (`"%IDTECH4_VSWHERE%" -latest -products * -version "[18.0,19.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "IDTECH4_VSINSTALL=%%I"
:found_vs

if not defined IDTECH4_VSINSTALL (
    echo Visual Studio 2026 with C++ tools was not found.
    echo Install VS2026 Community C++ desktop tools, or run from an existing developer prompt.
    exit /b 1
)

call "%IDTECH4_VSINSTALL%\Common7\Tools\VsDevCmd.bat" -arch=%IDTECH4_ARCH% -host_arch=%IDTECH4_HOST_ARCH%
if errorlevel 1 exit /b %errorlevel%

set "IDTECH4_DEFAULT_CONFIGURE_PRESET=vs2026-win32"
set "IDTECH4_DEFAULT_BUILD_PRESET=vs2026-win32-release"
set "IDTECH4_DEFAULT_DEDICATED_BUILD_PRESET=vs2026-win32-dedicated-release"

echo.
echo IdTech4 developer environment ready.
echo   Root: %IDTECH4_ROOT%
echo   VS:   %IDTECH4_VSINSTALL%
echo   Arch: %IDTECH4_ARCH%
echo.
exit /b 0
