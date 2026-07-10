cmake_minimum_required(VERSION 3.16)
project(ZowiDesktop VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick Bluetooth)

# ── Core library: pure data models ───────────────────────────────────
add_library(zowi_core INTERFACE)
target_include_directories(zowi_core INTERFACE src)
target_link_libraries(zowi_core INTERFACE Qt6::Core)

# ── Services library: business logic ─────────────────────────────────
add_library(zowi_services STATIC
    src/services/TranslationEngine.cpp
    src/services/SessionService.cpp
    src/services/BluetoothService.cpp
)
target_include_directories(zowi_services PUBLIC src)
target_link_libraries(zowi_services PUBLIC Qt6::Core Qt6::Bluetooth zowi_core)

# ── Controllers library: QML bridge ──────────────────────────────────
add_library(zowi_controllers STATIC
    src/controllers/TranslatorController.cpp
    src/controllers/SessionController.cpp
    src/controllers/BluetoothController.cpp
)
target_include_directories(zowi_controllers PUBLIC src)
target_link_libraries(zowi_controllers PUBLIC Qt6::Core Qt6::Quick zowi_core zowi_services)

# ── Resources ────────────────────────────────────────────────────────
qt6_add_resources(RESOURCES resources.qrc)

# ── Main executable ──────────────────────────────────────────────────
add_executable(ZowiDesktop
    src/main.cpp
    ${RESOURCES}
)
target_link_libraries(ZowiDesktop PRIVATE zowi_controllers)

# ── Legacy test executables ──────────────────────────────────────────
add_executable(bt_test
    src/BtTest.cpp
)
target_link_libraries(bt_test PRIVATE Qt6::Core Qt6::Bluetooth)

# ── Unit tests ───────────────────────────────────────────────────────
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(src/tests)
endif()

# ── Install ──────────────────────────────────────────────────────────
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
