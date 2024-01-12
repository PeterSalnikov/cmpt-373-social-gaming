#ifndef SETUP_GAME_SETTING_H
#define SETUP_GAME_SETTING_H

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <any>
#include <iostream>
#include <cpp-tree-sitter.h>
#include <Variables.hpp>

class GameSettings
{
private:
	int parseInt(const ts::Node &intNode, const std::string_view sourcecode);
	bool parseBool(const ts::Node &intNode, const std::string_view sourcecode);
	void configurationParser(const ts::Node &root, const std::string_view sourcecode);
	var::varMapType valueMapParser(const ts::Node &node, const std::string_view sourcecode);
	var::Variable recursiveExpressionParser(const ts::Node &node, const std::string_view sourcecode);

	void constantsParser(const ts::Node &root, const std::string_view sourcecode);
	void variablesParser(const ts::Node &root, const std::string_view sourcecode);
	void perPlayerParser(const ts::Node &root, const std::string_view sourcecode);
	void perAudienceParser(const ts::Node &root, const std::string_view sourcecode);

public:
	var::varMapType setupSettings;
	var::varMapType constants;
	var::varMapType variables;
	var::varMapType perPlayerVariables;
	var::varMapType perAudienceVariables;

	GameSettings();
	void parse(ts::Node root, std::string_view sourcecode);
};

#endif