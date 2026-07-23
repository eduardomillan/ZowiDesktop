@echo off
REM ======================================================
REM  Build Zowi Desktop portable .zip for Windows
REM  Run from a Developer Command Prompt (MSVC) with Qt
REM  and CMake on PATH, or set QT_PATH / CMAKE_PATH below.
REM ======================================================

set APP_NAME=ZowiDesktop
set BUILD_DIR=build-win-portable
set DIST_DIR=dist

REM --- Locate tools (adjust if paths differ) ---
if not defined QT_PATH     set QT_PATH=C:\Qt\6.11.1\msvc2022_64
if not defined CMAKE_PATH  set CMAKE_PATH=C:\Qt\Tools\CMake_64\bin

set "PATH=%CMAKE_PATH%;%QT_PATH%\bin;%PATH%"

echo === Step 1: Configure with CMake ===
mkdir %BUILD_DIR% 2>nul
cd %BUILD_DIR%
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DZOWI_BUILD_CLI=OFF
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
echo === Step 4: Copy executable ===
copy src\gui\Release\%APP_NAME%.exe %DIST_DIR%\
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

echo.
echo === Step 5: Create portable zip ===
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value') do set DATETIME=%%I
set TIMESTAMP=%DATETIME:~0,8%
set ZIPNAME=%APP_NAME%-windows-x86_64-build-%TIMESTAMP%.zip
cd %DIST_DIR%
if exist "%ZIPNAME%" del "%ZIPNAME%"
powershell -command "Compress-Archive -Path * -DestinationPath '%ZIPNAME%'"
if %ERRORLEVEL% neq 0 (
    echo.
    echo WARNING: Could not create zip. Manually zip the contents of:
    echo   %CD%
) else (
    echo.
    echo === Done: %ZIPNAME% ===
)

pause
