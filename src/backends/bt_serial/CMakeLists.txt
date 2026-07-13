# src/backends/bt_serial/CMakeLists.txt
add_library(zowi_bt_serial STATIC
    serial_bluetooth_backend.cpp
)

target_link_libraries(zowi_bt_serial
    PUBLIC  Zowi::core Qt${ZOWI_QT_VERSION}::Core
)

target_include_directories(zowi_bt_serial
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(Zowi::bt_serial ALIAS zowi_bt_serial)
