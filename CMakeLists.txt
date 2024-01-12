cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 20)

project(socialgaming)

# set default to debug mode for development
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug") # change later
endif()

# -DCMAKE_BUILD_TYPE=Release or -DCMAKE_BUILD_TYPE=Debug
# building once in one mode will cache it. use ^ to overwrite cache
# omitting -fno-rtti to enable dynamic_cast
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall --std=c++20\
       -lpthread -fno-omit-frame-pointer -fsanitize=address,undefined,leak")

# set flag to enable debug mode specific c++
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("building in DEBUG mode")
    add_definitions(-D_DEBUG)
else()
    message("building in RELEASE mode")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

# ======================================third party libraries======================================

include(cmake/CPM.cmake)
include(FetchContent)
# unit testing
CPMAddPackage(
    NAME googletest
    GITHUB_REPOSITORY google/googletest
    GIT_TAG v1.14.0
)

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



# ======================================add subdirs======================================
# add subdirectories
add_subdirectory(tools)
add_subdirectory(lib/)
add_subdirectory(src/)
add_subdirectory(test/)


# Use the library in a demo program.
add_executable(socialgaming)
target_sources(socialgaming
  PRIVATE
  src/main.cpp
)

add_executable(demo)
target_sources(demo
  PRIVATE
  demo.cpp
)



file(COPY "games" DESTINATION "${PROJECT_BINARY_DIR}")

target_include_directories(socialgaming PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/include/")
target_link_libraries(socialgaming

  tree-sitter-socialgaming
  cpp-tree-sitter
  setupGameSettingLib
  socialgaming-lib
  gameInstanceManagerLib
)

target_link_libraries(demo
  variables
  environmentMgr
  tree-sitter-socialgaming
  cpp-tree-sitter
  setupGameSettingLib
)

find_package(Boost 1.72 COMPONENTS date_time REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

# ======================================make tests======================================
# automatically get all test files
file(GLOB_RECURSE TEST_FILES "${PROJECT_SOURCE_DIR}/lib/*/tests/*_test.cpp")
# add all test files to same exec so tests can be run with ./bin/testapp
# to run specific module's tests ./bin/testapp --gtest_filter=TestSuiteName

file(GLOB_RECURSE TEST_FILES "${PROJECT_SOURCE_DIR}/lib/*/tests/*_test.cpp")
file(GLOB_RECURSE TEST_FILES_ROOT "${PROJECT_SOURCE_DIR}/test/*.cpp")
list(APPEND TEST_FILES ${TEST_FILES_ROOT})
file(GLOB_RECURSE TEST_FILES_SRC "${PROJECT_SOURCE_DIR}/src/tests/*.cpp")
list(APPEND TEST_FILES ${TEST_FILES_SRC})

list(LENGTH TEST_FILES FILE_LEN)
if(NOT FILE_LEN EQUAL 0)
    add_executable(testapp)
    target_sources(testapp PRIVATE ${TEST_FILES})
    target_link_libraries(testapp
        environmentMgr
        ambassador
        gtest_main
        gtest
        gmock
        socialgaming-lib
        variables
        socialGamingTaskFactory
        ${Boost_LIBRARIES} )
    target_include_directories(testapp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" PRIVATE ${Boost_INCLUDE_DIRS})

    enable_testing()
    add_test(first_and_only_test testapp)
endif()


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

