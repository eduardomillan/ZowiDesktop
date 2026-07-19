# src/gui/CMakeLists.txt
set(GUI_QRC_FILES
    ${CMAKE_SOURCE_DIR}/views.qrc
    ${CMAKE_SOURCE_DIR}/app.qrc
    ${CMAKE_SOURCE_DIR}/images.qrc
    ${CMAKE_SOURCE_DIR}/i18n.qrc
)
if(ZOWI_QT_VERSION EQUAL 6)
    qt6_add_resources(GUI_RESOURCES ${GUI_QRC_FILES})
else()
    qt5_add_resources(GUI_RESOURCES ${GUI_QRC_FILES})
endif()

add_executable(ZowiDesktop
    main.cpp
    controllers/SessionController.cpp
    controllers/TranslatorController.cpp
    controllers/BluetoothController.cpp
    controllers/ConfigController.cpp
    ${GUI_RESOURCES}
)

target_include_directories(ZowiDesktop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(ZowiDesktop PRIVATE ZOWI_VERSION="${PROJECT_VERSION}")

target_link_libraries(ZowiDesktop PRIVATE
    Zowi::core
    Zowi::bt_qt
    Zowi::firmware
    Qt${ZOWI_QT_VERSION}::Core
    Qt${ZOWI_QT_VERSION}::Quick
    Qt${ZOWI_QT_VERSION}::QuickControls2
)

# Serial/USB transport (POSIX-only). Provides the USB connection option.
if(NOT WIN32)
    target_link_libraries(ZowiDesktop PRIVATE Zowi::bt_serial)
    target_compile_definitions(ZowiDesktop PRIVATE ZOWI_HAVE_SERIAL=1)
endif()

add_executable(zowi_screen_preview
    preview_main.cpp
    controllers/TranslatorController.cpp
    controllers/ConfigController.cpp
    ${GUI_RESOURCES}
)

target_include_directories(zowi_screen_preview PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(zowi_screen_preview PRIVATE ZOWI_VERSION="${PROJECT_VERSION}")

target_link_libraries(zowi_screen_preview PRIVATE
    Zowi::core
    Qt${ZOWI_QT_VERSION}::Core
    Qt${ZOWI_QT_VERSION}::Gui
    Qt${ZOWI_QT_VERSION}::Qml
    Qt${ZOWI_QT_VERSION}::Quick
    Qt${ZOWI_QT_VERSION}::QuickControls2
)

# Grant CAP_NET_ADMIN so the GUI can connect via BlueZ SPP without root.
find_program(SETCAP_PROGRAM setcap)
if(SETCAP_PROGRAM)
    add_custom_command(TARGET ZowiDesktop POST_BUILD
        COMMAND bash -c "sudo -n ${SETCAP_PROGRAM} cap_net_admin+ep '$<TARGET_FILE:ZowiDesktop>' 2>/dev/null || echo 'NOTE: could not auto-apply CAP_NET_ADMIN (run: sudo setcap cap_net_admin+ep $<TARGET_FILE:ZowiDesktop>)'"
        COMMENT "Granting CAP_NET_ADMIN to ZowiDesktop (needs sudo)"
        VERBATIM)
endif()
