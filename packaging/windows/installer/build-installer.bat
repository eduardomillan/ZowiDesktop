@echo off
REM ======================================================
REM  Build Zowi Desktop Windows installer (.exe)
REM  Prerequisites: Inno Setup 6+ installed, Qt on PATH
REM
REM  Usage:
REM    build-installer.bat              (uses version from CMakeLists.txt)
REM    build-installer.bat /DVERSION=1.2.3
REM ======================================================

setlocal enabledelayedexpansion

set APP_NAME=ZowiDesktop
set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..\..
set BUILD_DIR=%PROJECT_ROOT%\build-win-installer
set DIST_DIR=%PROJECT_ROOT%\dist
set DIST_INST=%PROJECT_ROOT%\dist-installer

REM --- Locate tools ---
if not defined QT_PATH     set QT_PATH=C:\Qt\6.11.1\msvc2022_64
if not defined CMAKE_PATH  set CMAKE_PATH=C:\Qt\Tools\CMake_64\bin
if not defined ISCC_PATH   set ISCC_PATH=C:\Program Files (x86)\Inno Setup 6

set "PATH=%CMAKE_PATH%;%QT_PATH%\bin%;%ISCC_PATH%;%PATH%"

REM --- Extract version from CMakeLists.txt ---
for /f "tokens=2 delims= " %%v in ('findstr /R "^project.*VERSION" %PROJECT_ROOT%\CMakeLists.txt') do set CMAKE_VER=%%v
for /f "tokens=2 delims=)" %%v in ("%CMAKE_VER%") do set VERSION=%%v
echo Detected version: %VERSION%

REM --- Step 1: Build ---
echo.
echo === Step 1: Configure with CMake ===
mkdir "%BUILD_DIR%" 2>nul
cd /d "%BUILD_DIR%"
cmake "%PROJECT_ROOT%" -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DZOWI_BUILD_CLI=ON
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

echo.
echo === Step 2: Build ===
cmake --build . --config Release
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

REM --- Step 2: Collect DLLs ---
echo.
echo === Step 3: Collect DLLs with windeployqt ===
mkdir "%DIST_DIR%" 2>nul
windeployqt --qmldir "%PROJECT_ROOT%\src\views" --dir "%DIST_DIR%" "%BUILD_DIR%\src\gui\Release\%APP_NAME%.exe"
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

echo.
echo === Step 4: Copy executables ===
copy "%BUILD_DIR%\src\gui\Release\%APP_NAME%.exe" "%DIST_DIR%\" >nul
copy "%BUILD_DIR%\src\cli\Release\zowi_cli.exe" "%DIST_DIR%\" >nul 2>nul

REM --- Step 3: Build installer ---
echo.
echo === Step 5: Build installer with Inno Setup ===
mkdir "%DIST_INST%" 2>nul
iscc.exe /DVERSION=%VERSION% "%SCRIPT_DIR%zowi-desktop.iss"
if %ERRORLEVEL% neq 0 (
    echo ERROR: Inno Setup failed
    exit /b %ERRORLEVEL%
)

echo.
echo === Done ===
dir /b "%DIST_INST%\*.exe"
pause
