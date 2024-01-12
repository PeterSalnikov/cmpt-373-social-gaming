
find_package(Boost 1.72 COMPONENTS date_time REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

add_executable(gameserver
  gameserver.cpp
)

set_target_properties(gameserver
                      PROPERTIES
                      LINKER_LANGUAGE CXX
                      CXX_STANDARD 20
                      PREFIX ""
)

find_package(Threads REQUIRED)

target_link_libraries(gameserver
  networking
  ambassador
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
)
target_include_directories(gameserver PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" PRIVATE ${Boost_INCLUDE_DIRS})

install(TARGETS gameserver
  RUNTIME DESTINATION bin
)

