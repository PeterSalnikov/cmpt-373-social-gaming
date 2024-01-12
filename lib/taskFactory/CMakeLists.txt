add_subdirectory(tests)


add_library(socialGamingTaskFactory)
target_sources(socialGamingTaskFactory
    PUBLIC
    SocialGamingTaskFactory.cpp
    TaskFactory.hpp
    MockClasses.hpp)

target_include_directories(socialGamingTaskFactory PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(socialGamingTaskFactory PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(socialGamingTaskFactory PUBLIC
    GameInstanceLibrary
    socialgaming-lib
    variables
)