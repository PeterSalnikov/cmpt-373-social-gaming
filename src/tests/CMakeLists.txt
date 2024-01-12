# -------------------------------------------------------------------------------
# https://google.github.io/googletest/quickstart-cmake.html
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
# -------------------------------------------------------------------------------


# Downloads this wrapper library and tree-sitter.
# Makes them available via the `cpp-tree-sitter` CMake library target.
CPMAddPackage(
  NAME cpp-tree-sitter
  GIT_REPOSITORY https://github.com/nsumner/cpp-tree-sitter.git
  GIT_TAG v0.0.2
)

# Downloads a tree-sitter grammar from github and makes it available as a
# cmake library target.
add_grammar_from_repo(tree-sitter-socialgaming                 # Defines the library name for a grammar
  https://github.com/nsumner/tree-sitter-socialgaming.git  # Repository URL of a tree-sitter grammar
  0.0.1                                               # Version tag for the grammar
)

enable_testing()

add_executable(controlflow-test
  controlflow_test.cpp

)

target_include_directories(controlflow-test PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../src/include/")
target_link_libraries(controlflow-test
  gmock gtest gtest_main
  socialgaming-lib
  tree-sitter-socialgaming
  cpp-tree-sitter
  variables
)
include(GoogleTest)
gtest_discover_tests(controlflow-test)
# add_test(NAME AllTests COMMAND socialgaming-test)