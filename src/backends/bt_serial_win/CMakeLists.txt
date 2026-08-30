# src/backends/bt_serial_win/CMakeLists.txt
add_library(zowi_bt_serial_win STATIC
    win_serial_backend.cpp
)

target_link_libraries(zowi_bt_serial_win
    PUBLIC  Zowi::core Qt${ZOWI_QT_VERSION}::Core
)

target_include_directories(zowi_bt_serial_win
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(Zowi::bt_serial_win ALIAS zowi_bt_serial_win)
