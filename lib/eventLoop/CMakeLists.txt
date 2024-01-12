add_subdirectory(tests)

# testing script
add_executable(eventLoop
    EventLoop.cpp)

find_package(Boost 1.72 COMPONENTS date_time REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()
target_link_libraries(eventLoop PRIVATE ambassador PUBLIC ${Boost_LIBRARIES})
target_include_directories(eventLoop PRIVATE ambassador PUBLIC ${Boost_LIBRARIES})
