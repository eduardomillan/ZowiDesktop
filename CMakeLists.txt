cmake_minimum_required(VERSION 3.16)
project(ZowiDesktop VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Quick)

qt5_add_resources(RESOURCES resources.qrc)

add_executable(ZowiDesktop
    src/main.cpp
    src/translator.cpp
    src/sessioncontroller.cpp
    ${RESOURCES}
)

target_include_directories(ZowiDesktop PRIVATE src)
target_link_libraries(ZowiDesktop PRIVATE Qt5::Core Qt5::Quick)

install(TARGETS ZowiDesktop
    RUNTIME DESTINATION bin
)

install(FILES packaging/linux/zowi-desktop.desktop
    DESTINATION share/applications
)

install(FILES packaging/linux/zowi-desktop.png
    DESTINATION share/icons/hicolor/256x256/apps
)
