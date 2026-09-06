# src/core/CMakeLists.txt
add_library(zowi_core STATIC
    src/session_store.cpp
    src/config_store.cpp
    src/translation_engine.cpp
    src/robot_commands.cpp
    src/calibration_session.cpp
    src/message_parser.cpp
    src/movement_sequencer.cpp
    src/robot_state.cpp
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

# Core is framework-free (no Qt): used by the CLI, the GUI and, potentially,
# Android/Flutter hosts via FFI/JNI.
add_library(Zowi::core ALIAS zowi_core)
