# src/backends/bt_native/CMakeLists.txt
add_library(zowi_bt_native STATIC
    native_bluetooth_backend.cpp
)

target_link_libraries(zowi_bt_native
    PUBLIC Zowi::core
    PRIVATE runtimeobject ole32
)

target_include_directories(zowi_bt_native
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(zowi_bt_native PUBLIC cxx_std_20)

add_library(Zowi::bt_native ALIAS zowi_bt_native)
