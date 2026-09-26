# src/gui/CMakeLists.txt

# Auto-generate views.qrc, images.qrc, projects.qrc from disk globbing.
# app.qrc and i18n.qrc remain manually maintained (small, with custom aliases/locales).
include(${CMAKE_SOURCE_DIR}/cmake/GenerateQrc.cmake)

set(GENERATED_QRC_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_QRC_DIR})

# Generate views.qrc: all .qml files under src/views/
zowi_generate_qrc(${GENERATED_QRC_DIR}/views.qrc ${CMAKE_SOURCE_DIR} "/"
    ${CMAKE_SOURCE_DIR}/src/views/*.qml
    ${CMAKE_SOURCE_DIR}/src/views/screens/*.qml
    ${CMAKE_SOURCE_DIR}/src/views/components/*.qml
)

# Generate images.qrc: image assets under images/ (excludes other formats like .xcf)
zowi_generate_qrc(${GENERATED_QRC_DIR}/images.qrc ${CMAKE_SOURCE_DIR} "/"
    ${CMAKE_SOURCE_DIR}/images/*.png
    ${CMAKE_SOURCE_DIR}/images/*.jpg
    ${CMAKE_SOURCE_DIR}/images/*.jpeg
    ${CMAKE_SOURCE_DIR}/images/*/*.png
    ${CMAKE_SOURCE_DIR}/images/*/*.jpg
    ${CMAKE_SOURCE_DIR}/images/*/*.jpeg
)

# Generate projects.qrc: project data and locale pages/quizzes/strings
zowi_generate_qrc(${GENERATED_QRC_DIR}/projects.qrc ${CMAKE_SOURCE_DIR}/projects "/projects"
    ${CMAKE_SOURCE_DIR}/projects/*.json
    ${CMAKE_SOURCE_DIR}/projects/*/*.json
    ${CMAKE_SOURCE_DIR}/projects/*/*.html
)

set(GUI_QRC_FILES
    ${GENERATED_QRC_DIR}/views.qrc
    ${CMAKE_SOURCE_DIR}/app.qrc
    ${GENERATED_QRC_DIR}/images.qrc
    ${CMAKE_SOURCE_DIR}/i18n.qrc
    ${GENERATED_QRC_DIR}/projects.qrc
)
if(ZOWI_QT_VERSION EQUAL 6)
    qt6_add_resources(GUI_RESOURCES ${GUI_QRC_FILES} OPTIONS --no-zstd)
else()
    qt5_add_resources(GUI_RESOURCES ${GUI_QRC_FILES})
endif()

if(WIN32)
    set(ZOWI_ICON_PATH "${CMAKE_SOURCE_DIR}/images/app_icon.ico")
    configure_file(${CMAKE_SOURCE_DIR}/images/app_icon.rc.in
                   ${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc @ONLY)
endif()

add_executable(ZowiDesktop WIN32
    main.cpp
    controllers/SessionController.cpp
    controllers/TranslatorController.cpp
    controllers/RobotController.cpp
    controllers/ConfigController.cpp
    controllers/CalibrationSessionController.cpp
    controllers/CommandsController.cpp
    controllers/ProjectsController.cpp
    controllers/ZowiDiceController.cpp
    controllers/MouthsGameController.cpp
    controllers/TimelineController.cpp
    ${GUI_RESOURCES}
)

if(WIN32)
    target_sources(ZowiDesktop PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc")
endif()

target_include_directories(ZowiDesktop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(ZowiDesktop PRIVATE ZOWI_VERSION="${PROJECT_VERSION}")

target_link_libraries(ZowiDesktop PRIVATE
    Zowi::core
    Zowi::firmware
    Qt${ZOWI_QT_VERSION}::Core
    Qt${ZOWI_QT_VERSION}::Quick
    Qt${ZOWI_QT_VERSION}::QuickControls2
)

# Qt Bluetooth backend (Linux/macOS only)
if(NOT WIN32)
    target_link_libraries(ZowiDesktop PRIVATE Zowi::bt_qt)
endif()

# Serial/USB transport. Provides the USB connection option.
if(NOT WIN32)
    target_link_libraries(ZowiDesktop PRIVATE Zowi::bt_serial)
    target_compile_definitions(ZowiDesktop PRIVATE ZOWI_HAVE_SERIAL=1)
endif()
if(WIN32)
    target_link_libraries(ZowiDesktop PRIVATE Zowi::bt_serial_win)
    target_compile_definitions(ZowiDesktop PRIVATE ZOWI_HAVE_SERIAL=1)
endif()

# Native Windows Bluetooth (C++/WinRT SPP). Replaces BlueZ on Windows.
if(WIN32)
    target_link_libraries(ZowiDesktop PRIVATE Zowi::bt_native)
    target_compile_definitions(ZowiDesktop PRIVATE ZOWI_HAVE_NATIVE_BT=1)
endif()

add_executable(zowi_screen_preview
    preview_main.cpp
    controllers/TranslatorController.cpp
    controllers/ConfigController.cpp
    controllers/CalibrationSessionController.cpp
    controllers/CommandsController.cpp
    controllers/MouthsGameController.cpp
    controllers/ProjectsController.cpp
    controllers/SessionController.cpp
    controllers/RobotController.cpp
    controllers/TimelineController.cpp
    ${GUI_RESOURCES}
)

target_include_directories(zowi_screen_preview PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(zowi_screen_preview PRIVATE ZOWI_VERSION="${PROJECT_VERSION}")

target_link_libraries(zowi_screen_preview PRIVATE
    Zowi::core
    Zowi::firmware
    Qt${ZOWI_QT_VERSION}::Core
    Qt${ZOWI_QT_VERSION}::Gui
    Qt${ZOWI_QT_VERSION}::Qml
    Qt${ZOWI_QT_VERSION}::Quick
    Qt${ZOWI_QT_VERSION}::QuickControls2
)

# Qt Bluetooth backend (Linux/macOS only)
if(NOT WIN32)
    target_link_libraries(zowi_screen_preview PRIVATE Zowi::bt_qt)
endif()

# Serial/USB transport. Provides the USB connection option.
if(NOT WIN32)
    target_link_libraries(zowi_screen_preview PRIVATE Zowi::bt_serial)
    target_compile_definitions(zowi_screen_preview PRIVATE ZOWI_HAVE_SERIAL=1)
endif()
if(WIN32)
    target_link_libraries(zowi_screen_preview PRIVATE Zowi::bt_serial_win)
    target_compile_definitions(zowi_screen_preview PRIVATE ZOWI_HAVE_SERIAL=1)
endif()

# Native Windows Bluetooth (C++/WinRT SPP). Replaces BlueZ on Windows.
if(WIN32)
    target_link_libraries(zowi_screen_preview PRIVATE Zowi::bt_native)
    target_compile_definitions(zowi_screen_preview PRIVATE ZOWI_HAVE_NATIVE_BT=1)
endif()

# Grant CAP_NET_ADMIN so the GUI can connect via BlueZ SPP without root.
find_program(SETCAP_PROGRAM setcap)
if(SETCAP_PROGRAM)
    add_custom_command(TARGET ZowiDesktop POST_BUILD
        COMMAND bash -c "sudo -n ${SETCAP_PROGRAM} cap_net_admin+ep '$<TARGET_FILE:ZowiDesktop>' 2>/dev/null || echo 'NOTE: could not auto-apply CAP_NET_ADMIN (run: sudo setcap cap_net_admin+ep $<TARGET_FILE:ZowiDesktop>)'"
        COMMENT "Granting CAP_NET_ADMIN to ZowiDesktop (needs sudo)"
        VERBATIM)
endif()
