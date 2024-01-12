
find_package(Boost 1.72 COMPONENTS date_time REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

add_executable(mockServer)
target_sources(mockServer PRIVATE mockServerTest.cpp)
target_link_libraries(mockServer ambassador ${Boost_LIBRARIES})
target_include_directories(mockServer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" PRIVATE ${Boost_INCLUDE_DIRS})


add_executable(mockAnswer)
target_sources(mockAnswer PRIVATE mockAnswer.cpp)
target_link_libraries(mockAnswer ambassador ${Boost_LIBRARIES})
target_include_directories(mockAnswer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" PRIVATE ${Boost_INCLUDE_DIRS})