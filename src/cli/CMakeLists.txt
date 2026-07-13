# src/cli/CMakeLists.txt
add_executable(zowi_cli main.cpp)

target_link_libraries(zowi_cli PRIVATE
    Zowi::core
    Zowi::bt_qt
    Zowi::bt_serial
    Zowi::firmware
    Qt${ZOWI_QT_VERSION}::Bluetooth
    CLI11::CLI11
)
