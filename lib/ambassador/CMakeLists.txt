add_subdirectory(tests)

set(Boost_USE_STATIC_LIBS        ON)  # only find static libs
set(Boost_USE_DEBUG_LIBS        OFF)  # ignore debug libs and
set(Boost_USE_RELEASE_LIBS       ON)  # only find release libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)

find_package(Boost 1.72 COMPONENTS date_time REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

add_library(ambassador)
target_sources(ambassador PRIVATE Ambassador.cpp)
if(NOT APPLE)
    target_link_libraries(ambassador PRIVATE nlohmann_json ${Boost_LIBRARIES} rt)
ELSE()
    target_link_libraries(ambassador PRIVATE ${Boost_LIBRARIES})
ENDIF()
target_include_directories(ambassador PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" PRIVATE ${Boost_INCLUDE_DIRS})
