#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include<sstream>
#include <iterator>

#include <cpp-tree-sitter.h>

#include "SetupGameSetting.h"
#include "EnvironmentMgr.hpp"

using namespace var;

extern "C" {
TSLanguage* tree_sitter_socialgaming();
}

int main() {
  // Create a language and parser.
  ts::Language language = tree_sitter_socialgaming();
  ts::Parser parser{language};

  std::ifstream MyReadFile("games/rockpaper.json");

  std::string myText;
  std::ostringstream ss;
  ss << MyReadFile.rdbuf(); // reading data
  myText = ss.str();

  // Parse the provided string into a syntax tree.
  std::string sourcecode = myText;
  ts::Tree tree = parser.parseString(sourcecode);

  // Get the root node of the syntax tree. 
  ts::Node root = tree.getRootNode();

  std::cout << sourcecode;

  GameSettings settings = GameSettings();

  settings.parse(root, sourcecode);

  assert(root.getType() == "game");

  env_mgr::EnvironmentManager mgr;

  mgr.setVariable("configuration", settings.setupSettings);
  mgr.setVariable("constants", settings.constants);
  mgr.setVariable("variables", settings.variables);
  mgr.setVariable("per_player", settings.perPlayerVariables);
  mgr.setVariable("per_audience", settings.perAudienceVariables);

  
/*
  std::cout << root.getChildByFieldName("constants").getSExpr().get();
  std::cout << root.getChildByFieldName("variables").getSExpr().get();

  std::cout << root.getChildByFieldName("per_player").getSExpr().get();
  std::cout << root.getChildByFieldName("per_audience").getSExpr().get();

  //std::cout << root.getChildByFieldName("rules").getSExpr().get();

  bool game = true;
  while(game){
    root.getNumNamedChildren()
  }

  // Get some child nodes.
  ts::Node array = root.getNamedChild(0);
  ts::Node number = array.getNamedChild(0);
  

  // Check that the nodes have the expected types.
  assert(root.getType() == "document");
  assert(array.getType() == "array");
  assert(number.getType() == "number");

  // Check that the nodes have the expected child counts.
  assert(root.getNumChildren() == 1);
  assert(array.getNumChildren() == 5);
  assert(array.getNumNamedChildren() == 2);
  assert(number.getNumChildren() == 0);

  // Print the syntax tree as an S-expression.
  auto treestring = root.getSExpr();
  printf("Syntax tree: %s\n", treestring.get());
*/
  return 0;
}