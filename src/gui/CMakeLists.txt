# src/gui/CMakeLists.txt
if(ZOWI_QT_VERSION EQUAL 6)
    qt6_add_resources(GUI_RESOURCES ${CMAKE_SOURCE_DIR}/resources.qrc)
else()
    qt5_add_resources(GUI_RESOURCES ${CMAKE_SOURCE_DIR}/resources.qrc)
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

target_link_libraries(ZowiDesktop PRIVATE
    Zowi::core
    Zowi::bt_qt
    Qt${ZOWI_QT_VERSION}::Core
    Qt${ZOWI_QT_VERSION}::Quick
    Qt${ZOWI_QT_VERSION}::QuickControls2
)

add_executable(zowi_screen_preview
    preview_main.cpp
    controllers/TranslatorController.cpp
    controllers/ConfigController.cpp
    ${GUI_RESOURCES}
)

target_include_directories(zowi_screen_preview PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(zowi_screen_preview PRIVATE
    Zowi::core
    Qt${ZOWI_QT_VERSION}::Core
    Qt${ZOWI_QT_VERSION}::Gui
    Qt${ZOWI_QT_VERSION}::Qml
    Qt${ZOWI_QT_VERSION}::Quick
    Qt${ZOWI_QT_VERSION}::QuickControls2
)
