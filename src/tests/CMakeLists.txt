find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(tst_SessionService
    tst_SessionService.cpp
)
target_link_libraries(tst_SessionService PRIVATE zowi_services Qt6::Test)
add_test(NAME tst_SessionService COMMAND tst_SessionService)

add_executable(tst_TranslationEngine
    tst_TranslationEngine.cpp
)
target_link_libraries(tst_TranslationEngine PRIVATE zowi_services Qt6::Test)
add_test(NAME tst_TranslationEngine COMMAND tst_TranslationEngine)
