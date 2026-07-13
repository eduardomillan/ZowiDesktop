cmake_minimum_required(VERSION 3.21)
project(ZowiDesktop VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# ── Build options ────────────────────────────────────────────────
option(ZOWI_BUILD_GUI   "Build Qt6 GUI application" ON)
option(ZOWI_BUILD_CLI   "Build command-line tool"    ON)
option(BUILD_TESTS      "Build unit tests"           ON)

# ── Third-party (via FetchContent) ──────────────────────────────
include(FetchContent)

FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11
    GIT_TAG        v2.4.2
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(cli11 json)

# ── Core library (pure C++ — no Qt) ─────────────────────────────
add_subdirectory(src/core)

# ── Qt (needed by GUI and CLI's BT backend) ──────────────────────
if(ZOWI_BUILD_GUI OR ZOWI_BUILD_CLI)
    find_package(Qt6 REQUIRED COMPONENTS Core Bluetooth)
endif()

if(ZOWI_BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2)
endif()

# ── Qt Bluetooth backend ─────────────────────────────────────────
if(ZOWI_BUILD_GUI OR ZOWI_BUILD_CLI)
    add_subdirectory(src/backends/bt_qt)
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
endif()
