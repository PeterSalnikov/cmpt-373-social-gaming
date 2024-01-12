add_library(gameInstanceManagerLib)
target_sources(gameInstanceManagerLib
  PRIVATE
  gameinstancemanager.cpp
)
target_include_directories(gameInstanceManagerLib
  PUBLIC
  include/
)
target_link_libraries(gameInstanceManagerLib PUBLIC GameInstanceLibrary)