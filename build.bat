@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
set "BUILD_DIR=%SCRIPT_DIR%\build"
set QT_VERSION=6
set TARGET=all
set RUN_DEMO=0

rem Locate cmake
set "CMAKE_EXE=cmake"
where cmake >nul 2>&1
if %errorlevel%==0 goto :cmake_found
if exist "C:\Qt\Tools\CMake_64\bin\cmake.exe" (
    set "CMAKE_EXE=C:\Qt\Tools\CMake_64\bin\cmake.exe"
    goto :cmake_ok
)
if exist "%USERPROFILE%\Qt\Tools\CMake_64\bin\cmake.exe" (
    set "CMAKE_EXE=%USERPROFILE%\Qt\Tools\CMake_64\bin\cmake.exe"
    goto :cmake_ok
)
echo ERROR: cmake not found. Install CMake or add it to PATH.
exit /b 1

:cmake_ok
set CMAKE_OK=1

:cmake_found
if not defined CMAKE_OK set CMAKE_OK=1

rem Locate Visual Studio Build Tools
set "VCVARSALL="
set "VSROOT=C:\Program Files (x86)\Microsoft Visual Studio"
if exist "%VSROOT%\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=%VSROOT%\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
if exist "%VSROOT%\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=%VSROOT%\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
if exist "%VSROOT%\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=%VSROOT%\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"
if exist "%VSROOT%\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=%VSROOT%\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
if exist "%VSROOT%\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=%VSROOT%\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
if exist "%VSROOT%\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=%VSROOT%\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
if exist "%VSROOT%\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=%VSROOT%\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat"
if exist "%VSROOT%\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=%VSROOT%\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
set "VSROOT=C:\Program Files\Microsoft Visual Studio"
if exist "%VSROOT%\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=%VSROOT%\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
if exist "%VSROOT%\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=%VSROOT%\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"
if exist "%VSROOT%\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=%VSROOT%\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
if exist "%VSROOT%\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=%VSROOT%\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
if exist "%VSROOT%\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=%VSROOT%\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat"
if exist "%VSROOT%\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=%VSROOT%\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
if not defined VCVARSALL goto :novs
goto :vsfound
:novs
echo ERROR: Visual Studio Build Tools (vcvarsall.bat) not found.
echo Install "Desktop development with C++" workload from Visual Studio Installer.
exit /b 1
:vsfound
call "%VCVARSALL%" amd64 >nul 2>&1

rem Parse arguments
:parse_args
if "%~1"=="" goto :done_args
if "%~1"=="-5" ( set "QT_VERSION=5" & shift & goto :parse_args )
if "%~1"=="-6" ( set "QT_VERSION=6" & shift & goto :parse_args )
if "%~1"=="--gui" ( set "TARGET=gui" & shift & goto :parse_args )
if "%~1"=="--cli" ( set "TARGET=cli" & shift & goto :parse_args )
if "%~1"=="--demo" ( set "TARGET=cli" & set "RUN_DEMO=1" & shift & goto :parse_args )
if "%~1"=="--all" ( set "TARGET=all" & shift & goto :parse_args )
if "%~1"=="-h" goto :usage
if "%~1"=="--help" goto :usage
echo Unknown option: %~1
goto :usage

:usage
echo Usage: build.bat [OPTIONS]
echo.
echo Build Zowi Desktop (GUI, CLI, or both).
echo.
echo Options:
echo   -5            Build against Qt 5 (default: Qt 6)
echo   -6            Build against Qt 6 (default)
echo.
echo   --gui         Build GUI only (ZowiDesktop)
echo   --cli         Build CLI only (zowi_cli)
echo   --demo        Build CLI and run demo commands
echo   --all         Build everything (default)
echo   -h            Show this help message
echo.
echo Environment:
echo   QT_PATH       Path to Qt installation (e.g. C:\Qt\6.5.2\msvc2022_64)
echo.
echo Examples:
echo   build.bat                  Build GUI + CLI with Qt 6
echo   build.bat --gui            Build only the GUI
echo   build.bat --cli            Build only the CLI
echo   build.bat --demo           Build CLI and run demo commands
echo   build.bat -5 --cli         Build CLI with Qt 5
echo   build.bat -6 --gui         Build GUI with Qt 6 (explicit)
echo   set QT_PATH=C:\Qt\5.15.2\msvc2019_64 ^&^& build.bat -5 --all
echo.
echo Targets:
echo   ZowiDesktop    Qt Quick GUI application  (build\src\gui\Release\ZowiDesktop.exe)
echo   zowi_cli       Command-line tool          (build\src\cli\Release\zowi_cli.exe)
exit /b 0

:done_args
set "BUILD_GUI=OFF"
set "BUILD_CLI=OFF"
if "%TARGET%"=="all" ( set "BUILD_GUI=ON" & set "BUILD_CLI=ON" )
if "%TARGET%"=="gui" ( set "BUILD_GUI=ON" )
if "%TARGET%"=="cli" ( set "BUILD_CLI=ON" )

set "CMAKE_EXTRA_ARGS="
if defined QT_PATH (
    set "CMAKE_EXTRA_ARGS=-DCMAKE_PREFIX_PATH=%QT_PATH%"
)

echo === Building Zowi Desktop (Qt %QT_VERSION%) ===
echo     GUI: %BUILD_GUI% ^| CLI: %BUILD_CLI%

"%CMAKE_EXE%" -S "%SCRIPT_DIR%" -B "%BUILD_DIR%" -DZOWI_QT_VERSION=%QT_VERSION% -DZOWI_BUILD_GUI=%BUILD_GUI% -DZOWI_BUILD_CLI=%BUILD_CLI% %CMAKE_EXTRA_ARGS%
if %errorlevel% neq 0 (
    echo CMake configuration failed.
    exit /b 1
)

if "%BUILD_GUI%"=="ON" (
    "%CMAKE_EXE%" --build "%BUILD_DIR%" --target ZowiDesktop --config Release
    if %errorlevel% neq 0 (
        echo GUI build failed.
        exit /b 1
    )
    rem Deploy Qt DLLs alongside the executable
    set "WINDEPLOYQT="
    if defined QT_PATH (
        if exist "%QT_PATH%\bin\windeployqt.exe" set "WINDEPLOYQT=%QT_PATH%\bin\windeployqt.exe"
    )
    if not defined WINDEPLOYQT if exist "C:\Qt\6.11.1\msvc2022_64\bin\windeployqt.exe" set "WINDEPLOYQT=C:\Qt\6.11.1\msvc2022_64\bin\windeployqt.exe"
    if not defined WINDEPLOYQT if exist "C:\Qt\6.5.2\msvc2022_64\bin\windeployqt.exe" set "WINDEPLOYQT=C:\Qt\6.5.2\msvc2022_64\bin\windeployqt.exe"
    if not defined WINDEPLOYQT if exist "C:\Qt\5.15.2\msvc2019_64\bin\windeployqt.exe" set "WINDEPLOYQT=C:\Qt\5.15.2\msvc2019_64\bin\windeployqt.exe"
    if defined WINDEPLOYQT (
        echo Deploying Qt runtime to build output...
        "%WINDEPLOYQT%" "%BUILD_DIR%\src\gui\Release\ZowiDesktop.exe" >nul 2>&1
    )
)

if "%BUILD_CLI%"=="ON" (
    "%CMAKE_EXE%" --build "%BUILD_DIR%" --target zowi_cli --config Release
    if %errorlevel% neq 0 (
        echo CLI build failed.
        exit /b 1
    )
)

echo.
echo Build complete (Qt %QT_VERSION%).

if "%BUILD_GUI%"=="ON" (
    echo   GUI: %BUILD_DIR%\src\gui\Release\ZowiDesktop.exe
)
if "%BUILD_CLI%"=="ON" (
    echo   CLI: %BUILD_DIR%\src\cli\Release\zowi_cli.exe
)

rem Demo mode
if "%RUN_DEMO%"=="1" (
    set "CLI=%BUILD_DIR%\src\cli\Release\zowi_cli.exe"

    echo.
    echo === session ===
    echo $ %CLI% session list
    call "!CLI!" session list
    echo.

    echo $ %CLI% session get wizardDismissed
    call "!CLI!" session get wizardDismissed
    echo.

    echo === config ===
    echo $ %CLI% config list
    call "!CLI!" config list
    echo.

    echo $ %CLI% config get know_more
    call "!CLI!" config get know_more
    echo.

    echo === translate ===
    echo $ %CLI% translate -s "Hola mundo"
    call "!CLI!" translate -s "Hola mundo"
    echo.

    echo $ %CLI% translate -l en_US -s "Hola mundo"
    call "!CLI!" translate -l en_US -s "Hola mundo"
    echo.

    echo === scan ===
    echo $ %CLI% scan
    call "!CLI!" scan
    echo.

    call "!CLI!" --help
)

endlocal
