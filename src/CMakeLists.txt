add_library(GameInstanceLibrary)
target_sources(GameInstanceLibrary
    PUBLIC
    gameinstance.cpp)

# Add the include directory to the include path
target_include_directories(GameInstanceLibrary PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(GameInstanceLibrary PUBLIC socialGamingTaskFactory environmentMgr)

# Configure the library to be tested
add_library(socialgaming-lib)
target_sources(socialgaming-lib
    PRIVATE
    RuleInterpreter.cpp
    Message.cpp
    controlflow.cpp
)
target_include_directories(socialgaming-lib
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(socialgaming-lib
    PRIVATE
    tree-sitter-socialgaming
    cpp-tree-sitter
)

set_target_properties(socialgaming-lib
    PROPERTIES LINKER_LANGUAGE CXX CXX_STANDARD 20
)

