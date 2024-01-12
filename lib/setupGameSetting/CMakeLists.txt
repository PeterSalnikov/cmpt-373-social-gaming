add_library(setupGameSettingLib)
target_sources(setupGameSettingLib
  PRIVATE
  SetupGameSetting.cpp
)
target_include_directories(setupGameSettingLib
  PUBLIC
  include/
)

target_link_libraries(setupGameSettingLib
  variables
  environmentMgr
  tree-sitter-socialgaming
  cpp-tree-sitter
)

set_target_properties(setupGameSettingLib
  PROPERTIES LINKER_LANGUAGE CXX CXX_STANDARD 20
)
