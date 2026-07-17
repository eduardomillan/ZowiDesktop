cmake_minimum_required(VERSION 3.16)
project(ZowiDesktop VERSION 0.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# ── Qt version selection ──────────────────────────────────────────
set(ZOWI_QT_VERSION 6 CACHE STRING "Qt version to build against (5 or 6)")
if(NOT ZOWI_QT_VERSION MATCHES "^[56]$")
    message(FATAL_ERROR "ZOWI_QT_VERSION must be 5 or 6, got: ${ZOWI_QT_VERSION}")
endif()

# C++20 for Qt6, C++17 for Qt5
if(ZOWI_QT_VERSION EQUAL 6)
    set(CMAKE_CXX_STANDARD 20)
else()
    set(CMAKE_CXX_STANDARD 17)
endif()

# ── Build options ────────────────────────────────────────────────
option(ZOWI_BUILD_GUI   "Build Qt${ZOWI_QT_VERSION} GUI application" ON)
option(ZOWI_BUILD_CLI   "Build command-line tool"                    ON)
option(BUILD_TESTS      "Build unit tests"                           ON)

# ── Third-party ─────────────────────────────────────────────────
# Prefer system packages (required for offline Debian packaging);
# fall back to FetchContent when they are not available (local dev builds).
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json
        GIT_TAG        v3.11.3
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(json)
endif()

find_package(CLI11 QUIET)
if(NOT CLI11_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        cli11
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11
        GIT_TAG        v2.4.2
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(cli11)
endif()

# ── Qt Core (needed by core's TranslationEngine, which loads the
#    compiled-in i18n resources via QFile) ───────────────────────
find_package(Qt${ZOWI_QT_VERSION} REQUIRED COMPONENTS Core)

# ── Core library ───────────────────────────────────────────────
add_subdirectory(src/core)

# ── Firmware upload library (STK500v1 / Optiboot, pure C++) ─────
add_subdirectory(src/firmware)

# ── Qt Bluetooth (needed by GUI and CLI's BT backend) ───────────
if(ZOWI_BUILD_GUI OR ZOWI_BUILD_CLI)
    find_package(Qt${ZOWI_QT_VERSION} REQUIRED COMPONENTS Bluetooth)
endif()

if(ZOWI_BUILD_GUI)
    find_package(Qt${ZOWI_QT_VERSION} REQUIRED COMPONENTS Gui Qml Quick QuickControls2)
endif()

# ── Qt Bluetooth backend ─────────────────────────────────────────
if(ZOWI_BUILD_GUI OR ZOWI_BUILD_CLI)
    add_subdirectory(src/backends/bt_qt)
endif()

# ── Serial (RFCOMM TTY + DTR auto-reset) backend ────────────────
# POSIX-only (termios, unistd, ioctl). Excluded on Windows. Used by the CLI
# for USB flashing and by the GUI for the USB transport.
if((ZOWI_BUILD_CLI OR ZOWI_BUILD_GUI) AND NOT WIN32)
    add_subdirectory(src/backends/bt_serial)
endif()

# ── CLI executable ──────────────────────────────────────────────
if(ZOWI_BUILD_CLI)
    add_subdirectory(src/cli)
endif()

# ── Tests (enable_testing must come before any add_test) ────────
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(src/core/tests)
endif()

# ── GUI application ─────────────────────────────────────────────
if(ZOWI_BUILD_GUI)
    add_subdirectory(src/gui)
endif()

# ── Install ──────────────────────────────────────────────────────
if(ZOWI_BUILD_GUI)
    install(TARGETS ZowiDesktop
        RUNTIME DESTINATION bin
    )

    install(FILES packaging/linux/zowi-desktop.desktop
        DESTINATION share/applications
    )

    install(FILES images/app_icon.png
        DESTINATION share/icons/hicolor/256x256/apps
        RENAME zowi-desktop.png
    )

    install(FILES packaging/linux/com.zowi.desktop.appdata.xml
        DESTINATION share/metainfo
    )
endif()

if(ZOWI_BUILD_CLI)
    install(TARGETS zowi_cli
        RUNTIME DESTINATION bin
    )
endif()
