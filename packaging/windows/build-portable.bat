@echo off
REM ======================================================
REM  Build Zowi Desktop portable .zip for Windows
REM  Run this from "Qt 5.15 MinGW" shell (Start menu)
REM ======================================================

set APP_NAME=ZowiDesktop
set BUILD_DIR=build-win
set DIST_DIR=dist

echo === Step 1: Configure with CMake ===
mkdir %BUILD_DIR% 2>nul
cd %BUILD_DIR%
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

echo.
echo === Step 2: Build ===
mingw32-make -j%NUMBER_OF_PROCESSORS%
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

echo.
echo === Step 3: Collect DLLs with windeployqt ===
mkdir %DIST_DIR% 2>nul
windeployqt --dir %DIST_DIR% %APP_NAME%.exe
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

echo.
echo === Step 4: Copy executable ===
copy %APP_NAME%.exe %DIST_DIR%\
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

echo.
echo === Step 5: Create portable zip ===
cd %DIST_DIR%
if exist "%APP_NAME%-windows-x86_64.zip" del "%APP_NAME%-windows-x86_64.zip"
..\..\packaging\windows\zip.exe -r "%APP_NAME%-windows-x86_64.zip" . 2>nul
if %ERRORLEVEL% neq 0 (
    echo.
    echo WARNING: zip not found. Manually zip the contents of:
    echo   %CD%
) else (
    echo.
    echo === Done: %APP_NAME%-windows-x86_64.zip ===
)

pause
