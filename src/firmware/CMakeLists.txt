# src/firmware/CMakeLists.txt
add_library(zowi_firmware STATIC
    src/stk500v1.cpp
)

target_include_directories(zowi_firmware
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(zowi_firmware PUBLIC cxx_std_20)

add_library(Zowi::firmware ALIAS zowi_firmware)
