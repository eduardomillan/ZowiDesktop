@echo off
REM ======================================================
REM  Build Zowi Desktop Windows installer (.exe)
REM  Prerequisites: Inno Setup 6+ installed, Qt on PATH
REM
REM  Usage (from CMD or PowerShell):
REM    build-installer.bat              (uses version from CMakeLists.txt)
REM    build-installer.bat /DVERSION=1.2.3
REM ======================================================

setlocal enabledelayedexpansion

set APP_NAME=ZowiDesktop

REM --- Resolve absolute paths (avoids issues when launched from PowerShell) ---
set SCRIPT_DIR=%~dp0
for %%i in ("%SCRIPT_DIR%..\..\..") do set PROJECT_ROOT=%%~fi
set BUILD_DIR=%PROJECT_ROOT%\build-win-installer
set DIST_DIR=%PROJECT_ROOT%\dist
set DIST_INST=%PROJECT_ROOT%\dist-installer

REM --- Locate tools ---
if not defined QT_PATH     set QT_PATH=C:\Qt\6.11.1\msvc2022_64
if not defined CMAKE_PATH  set CMAKE_PATH=C:\Qt\Tools\CMake_64\bin
if not defined ISCC_PATH   set ISCC_PATH=C:\Program Files (x86)\Inno Setup 6

set "PATH=%CMAKE_PATH%;%QT_PATH%\bin%;%ISCC_PATH%;C:\Windows\System32\WindowsPowerShell\v1.0;%PATH%"

REM --- Extract version from CMakeLists.txt (match the project() line, not cmake_minimum_required) ---
set VERSION=
for /f %%v in ('powershell -NoProfile -Command "$c = Get-Content '%PROJECT_ROOT%\CMakeLists.txt' -Raw; if ($c -match 'project\(.*VERSION\s+([\d\.]+)') { $Matches[1] }"') do set VERSION=%%v
echo Detected version: %VERSION%

REM --- Packaged builds ship with dev mode OFF. Re-enabled at runtime via ZOWI_DEV ---
powershell -NoProfile -Command "$c = [IO.File]::ReadAllText('%PROJECT_ROOT%\src\config.json'); $c = $c -replace '(\"dev_mode\"\s*:\s*\")[^\"]*(\")', '${1}false${2}'; [IO.File]::WriteAllText('%PROJECT_ROOT%\src\config.json', $c)"

REM --- Step 1: Configure ---
echo.
echo === Step 1: Configure with CMake ===
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"
cmake "%PROJECT_ROOT%" -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QT_PATH%" -DZOWI_BUILD_CLI=ON
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

REM --- Step 2: Build ---
echo.
echo === Step 2: Build ===
cmake --build . --config Release
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

REM --- Step 3: Collect DLLs ---
echo.
echo === Step 3: Collect DLLs with windeployqt ===
if not exist "%DIST_DIR%" mkdir "%DIST_DIR%"
"%QT_PATH%\bin\windeployqt.exe" --qmldir "%PROJECT_ROOT%\src\views" --dir "%DIST_DIR%" "%BUILD_DIR%\src\gui\Release\%APP_NAME%.exe"
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

REM --- Step 4: Copy executables ---
echo.
echo === Step 4: Copy executables ===
copy "%BUILD_DIR%\src\gui\Release\%APP_NAME%.exe" "%DIST_DIR%\" >nul
copy "%BUILD_DIR%\src\cli\Release\zowi_cli.exe" "%DIST_DIR%\" >nul 2>nul

REM --- Step 5: Build installer ---
echo.
echo === Step 5: Build installer with Inno Setup ===
if not exist "%DIST_INST%" mkdir "%DIST_INST%"
"%ISCC_PATH%\iscc.exe" /DVERSION=%VERSION% "%SCRIPT_DIR%zowi-desktop.iss"
if %ERRORLEVEL% neq 0 (
    git checkout -- "%PROJECT_ROOT%\src\config.json" 2>nul
    echo ERROR: Inno Setup failed
    exit /b %ERRORLEVEL%
)

REM --- Restore dev mode for the development tree ---
git checkout -- "%PROJECT_ROOT%\src\config.json" 2>nul

echo.
echo === Done ===
dir /b "%DIST_INST%\*.exe"
pause
