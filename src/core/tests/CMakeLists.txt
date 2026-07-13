# src/core/tests/CMakeLists.txt

add_executable(test_session_store test_session_store.cpp)
target_link_libraries(test_session_store PRIVATE Zowi::core)
add_test(NAME test_session_store COMMAND test_session_store)

add_executable(test_config_store test_config_store.cpp)
target_link_libraries(test_config_store PRIVATE Zowi::core)
add_test(NAME test_config_store COMMAND test_config_store)

add_executable(test_translation_engine test_translation_engine.cpp)
target_link_libraries(test_translation_engine PRIVATE Zowi::core)
add_test(NAME test_translation_engine COMMAND test_translation_engine)
