# src/gui/CMakeLists.txt
qt6_add_resources(GUI_RESOURCES ${CMAKE_SOURCE_DIR}/resources.qrc)

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
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
)
