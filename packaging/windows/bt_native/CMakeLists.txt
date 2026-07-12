cmake_minimum_required(VERSION 3.16)
project(bt_native VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export symbols for DLL
add_definitions(-DBT_NATIVE_EXPORTS)

# Create the DLL
add_library(bt_native SHARED
    bt_native.cpp
    bt_native.def
)

target_include_directories(bt_native PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link WinRT Bluetooth libraries (Windows 10+)
target_link_libraries(bt_native PRIVATE
    windowsapp
    runtimeobject
    ole32
)

# Set output name (no 'lib' prefix)
set_target_properties(bt_native PROPERTIES
    PREFIX ""
    OUTPUT_NAME "bt_native"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install
install(TARGETS bt_native
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)
