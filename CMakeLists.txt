cmake_minimum_required(VERSION 3.16)
project(ZowiDesktop VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Quick Bluetooth)

# When cross-compiling for Windows, the MinGW moc.exe can't run on Linux.
# Replace it with the native Linux moc.
if(CMAKE_CROSSCOMPILING)
    find_program(NATIVE_MOC moc
        PATHS /usr/lib/x86_64-linux-gnu/qt5/bin /usr/bin
        NO_DEFAULT_PATH
    )
    if(NATIVE_MOC)
        set_target_properties(Qt5::moc PROPERTIES IMPORTED_LOCATION "${NATIVE_MOC}")
    endif()
    find_program(NATIVE_RCC rcc
        PATHS /usr/lib/x86_64-linux-gnu/qt5/bin /usr/bin
        NO_DEFAULT_PATH
    )
    if(NATIVE_RCC)
        set_target_properties(Qt5::rcc PROPERTIES IMPORTED_LOCATION "${NATIVE_RCC}")
    endif()
endif()

qt5_add_resources(RESOURCES resources.qrc)

add_executable(ZowiDesktop
    src/main.cpp
    src/translator.cpp
    src/sessioncontroller.cpp
    src/zowibluetoothcontroller.cpp
    ${RESOURCES}
)

add_executable(bt_test
    src/bt_test.cpp
)
target_link_libraries(bt_test PRIVATE Qt5::Core Qt5::Bluetooth)

target_include_directories(ZowiDesktop PRIVATE src)
target_link_libraries(ZowiDesktop PRIVATE Qt5::Core Qt5::Quick Qt5::Bluetooth)

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
