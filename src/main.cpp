#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "RuleInterpreter.h"
#include <cpp-tree-sitter.h>

extern "C" {
  TSLanguage* tree_sitter_socialgaming();
}

int main(int argc, char *argv[]) {

	ts::Language language = tree_sitter_socialgaming();
	ts::Parser parser{ language };

	const std::string defaultGameDirectory = "games/";

	std::vector<RuleInterpreter> ruleInterpreters;
	for (const auto& entry :  std::filesystem::directory_iterator(defaultGameDirectory)) {

		std::ifstream gameFile(entry.path());
		std::stringstream buffer;
		buffer << gameFile.rdbuf();

		RuleInterpreter ruleInterpreter { std::filesystem::path(entry).stem().string(), buffer.str() };
		ruleInterpreters.push_back((ruleInterpreter));
	}

	std::string_view source = ruleInterpreters[0].getSource();

	ts::Tree tree = parser.parseString(source);
	ts::Node root = tree.getRootNode();

	ts::Node configuration = root.getChildByFieldName("configuration");
	ts::Node rules = root.getChildByFieldName("rules");

	std::string_view gameName = getChildNodeValue(source, "name", configuration);
	std::cout << "game name: " << gameName << std::endl;

	std::shared_ptr<RuleTree> ruleTree = parseRules(rules, source);

	
	printRuleTreeRecursively(ruleTree.get()->getRules().get(), 0);
}