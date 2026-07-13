# src/core/CMakeLists.txt
add_library(zowi_core STATIC
    src/session_store.cpp
    src/config_store.cpp
    src/translation_engine.cpp
)

target_include_directories(zowi_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(zowi_core PUBLIC cxx_std_20)
target_link_libraries(zowi_core
    PUBLIC nlohmann_json::nlohmann_json
)

add_library(Zowi::core ALIAS zowi_core)
