add_subdirectory(tests)


# libraries
add_library(variables)
target_sources(variables
    PUBLIC
    Variables.cpp
    )
target_include_directories(variables PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(variables PROPERTIES LINKER_LANGUAGE CXX)

# libraries
add_library(environmentMgr)
target_sources(environmentMgr
    PUBLIC
    EnvironmentMgr.cpp
    )
target_include_directories(environmentMgr PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(environmentMgr PROPERTIES LINKER_LANGUAGE CXX)
target_link_directories(environmentMgr PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(environmentMgr PUBLIC variables socialgaming-lib
  tree-sitter-socialgaming
  cpp-tree-sitter)

