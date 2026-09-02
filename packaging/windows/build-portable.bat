@echo off
REM ======================================================
REM  Build Zowi Desktop portable .zip for Windows
REM  Run from a Developer Command Prompt (MSVC) with Qt
REM  and CMake on PATH, or set QT_PATH / CMAKE_PATH below.
REM ======================================================

setlocal

set APP_NAME=ZowiDesktop
set BUILD_DIR=build-win-portable
set DIST_DIR=dist
set WIN_DIST_DIR=build-windows\dist

REM --- Locate tools (adjust if paths differ) ---
if not defined QT_PATH     set QT_PATH=C:\Qt\6.11.1\msvc2022_64
if not defined CMAKE_PATH  set CMAKE_PATH=C:\Qt\Tools\CMake_64\bin

set "PATH=%CMAKE_PATH%;%QT_PATH%\bin;%PATH%"

REM --- Back up src/config.json so uncommitted changes survive packaging ---
copy src\config.json "%TEMP%\zowi_config_backup.json" >nul

REM --- Packaged builds ship with dev mode OFF. Re-enabled at runtime via DEV_MODE ---
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0set_dev_mode.ps1" false "src\config.json"
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

REM --- Extract version from CMakeLists.txt ---
set VERSION=
for /f %%v in ('powershell -NoProfile -Command "$c = Get-Content 'CMakeLists.txt' -Raw; if ($c -match 'project\(.*VERSION\s+([\d\.]+)') { $Matches[1] }"') do set VERSION=%%v
echo Detected version: %VERSION%

echo === Step 1: Configure with CMake ===
mkdir %BUILD_DIR% 2>nul
cd %BUILD_DIR%
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DZOWI_BUILD_CLI=ON
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

echo.
echo === Step 2: Build ===
cmake --build . --config Release
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

echo.
echo === Step 3: Collect DLLs with windeployqt ===
mkdir %DIST_DIR% 2>nul
windeployqt --qmldir ..\src\views --dir %DIST_DIR% src\gui\Release\%APP_NAME%.exe
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

echo.
echo === Step 4: Copy executables ===
copy src\gui\Release\%APP_NAME%.exe %DIST_DIR%\ >nul
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
copy src\cli\Release\zowi_cli.exe %DIST_DIR%\ >nul
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

echo.
echo === Step 5: Create portable zip in %WIN_DIST_DIR% ===
if not exist "..\%WIN_DIST_DIR%" mkdir "..\%WIN_DIST_DIR%"
set ZIPNAME=%APP_NAME%-%VERSION%-windows-x86_64.zip
powershell -NoProfile -Command "Compress-Archive -Path '%CD%\dist\*' -DestinationPath '..\%WIN_DIST_DIR%\%ZIPNAME%' -Force"
if %ERRORLEVEL% neq 0 (
    echo.
    echo WARNING: Could not create zip. Manually zip the contents of:
    echo   %CD%\dist
) else (
    echo.
    echo === Done: %WIN_DIST_DIR%\%ZIPNAME% ===
)

REM --- Restore dev mode for the development tree ---
cd ..
copy /y "%TEMP%\zowi_config_backup.json" src\config.json >nul
del "%TEMP%\zowi_config_backup.json" >nul 2>nul

pause