include(GoogleTest)

add_executable(socialgaming-test
    RuleInterpreter.cpp
)
target_include_directories(socialgaming-test PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../src/include/")
target_link_libraries(socialgaming-test
    gmock gtest gtest_main
    socialgaming-lib
    tree-sitter-socialgaming
    cpp-tree-sitter
)
gtest_discover_tests(socialgaming-test)

add_executable(gameinstance-test
    gameinstance_test.cpp
)
target_link_libraries(gameinstance-test PRIVATE
    gmock gtest gtest_main
    GameInstanceLibrary
)
gtest_discover_tests(gameinstance-test)