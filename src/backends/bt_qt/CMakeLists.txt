# src/backends/bt_qt/CMakeLists.txt
add_library(zowi_bt_qt STATIC
    qt_bluetooth_backend.cpp
)

target_link_libraries(zowi_bt_qt
    PUBLIC  Zowi::core Qt${ZOWI_QT_VERSION}::Core Qt${ZOWI_QT_VERSION}::Bluetooth
)

# QtDBus / BlueZ is only available on Linux; Windows uses the WinRT backend.
if(UNIX AND NOT APPLE)
    target_link_libraries(zowi_bt_qt PUBLIC Qt${ZOWI_QT_VERSION}::DBus)
endif()

target_include_directories(zowi_bt_qt
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(Zowi::bt_qt ALIAS zowi_bt_qt)
