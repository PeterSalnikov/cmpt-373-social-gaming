#include <SetupGameSetting.h>

using namespace var;

GameSettings::GameSettings()
{
}

void GameSettings::parse(ts::Node root, std::string_view sourcecode)
{
    configurationParser(root, sourcecode);

    constantsParser(root, sourcecode);

    variablesParser(root, sourcecode);

    perPlayerParser(root, sourcecode);

    perAudienceParser(root, sourcecode);
}

int GameSettings::parseInt(const ts::Node &intNode, const std::string_view sourcecode)
{
    try
    {
        return std::stoi(std::string(intNode.getSourceRange(sourcecode)).c_str());
    }
    catch(const std::exception& e)
    {
        throw e.what();
    }
}

bool GameSettings::parseBool(const ts::Node &intNode, const std::string_view sourcecode)
{
    if (intNode.getSourceRange(sourcecode) == "true")
    {
        return true;
    }
    else if (intNode.getSourceRange(sourcecode) == "false")
    {
        return false;
    }
    else
    {
        throw "boolean neither true nor false";
    }
}

void GameSettings::configurationParser(const ts::Node &root, const std::string_view sourcecode)
{
    ts::Node gameConfig = root.getChildByFieldName("configuration");

    varType map = varMapType();

    varType key = std::string("name");
    Variable variable = makeVar((std::string)gameConfig.getNamedChild(0).getSourceRange(sourcecode));
    VariableUtils::setValue(map, key, variable);

    key = std::string("player_range");

    varType list = listObj();

    varType lowerRange = parseInt(gameConfig.getNamedChild(1).getNamedChild(0), sourcecode);
    ListObjUtils::push_back(list, lowerRange);

    varType upperRange = parseInt(gameConfig.getNamedChild(1).getNamedChild(1), sourcecode);
    ListObjUtils::push_back(list, upperRange);

    variable = makeVar(std::get<listObj>(list));

    VariableUtils::setValue(map, key, variable);

    key = std::string("audience");
    variable = makeVar(parseBool(gameConfig.getNamedChild(2), sourcecode));
    VariableUtils::setValue(map, key, variable);

    //despite being under the setup {}, all setup settings are direct children of configuration
    for (uint32_t i = 3; i < gameConfig.getNumNamedChildren(); i++)
    {
        ts::Node setup = gameConfig.getNamedChild(i);

        key = std::string(setup.getChildByFieldName("name").getSourceRange(sourcecode));

        varType map2 = varMapType();

        varType key2 = std::string("kind");
        // kind is not a named child for some reason
        std::string kind = (std::string)setup.getChildByFieldName("kind").getSourceRange(sourcecode);
        Variable variable2 = makeVar(kind);
        VariableUtils::setValue(map2, key2, variable2);

        key2 = std::string("prompt");
        variable2 = makeVar((std::string)setup.getChildByFieldName("prompt").getSourceRange(sourcecode));
        VariableUtils::setValue(map2, key2, variable2);

        if (kind == "integer")
        {
            key2 = std::string("range");
            varType list2 = listObj();

            varType lowerRange2 = parseInt(setup.getNamedChild(2).getNamedChild(0), sourcecode);
            ListObjUtils::push_back(list2, lowerRange2);

            varType upperRange2 = parseInt(setup.getNamedChild(2).getNamedChild(1), sourcecode);
            ListObjUtils::push_back(list2, upperRange2);

            variable2 = makeVar(std::get<listObj>(list2));

            VariableUtils::setValue(map2, key2, variable2);

            //if default int exists
            if (setup.getNumNamedChildren() > 3)
            {
                key2 = std::string("default_int");
                variable2 = makeVar(parseInt(setup.getNamedChild(3), sourcecode));
                VariableUtils::setValue(map2, key2, variable2);
            }
        }
        // else if (kind == "enum")
        // {
        //     key2 = std::string("range");
        //     variable2 = range;
        //     VariableUtils::setValue(map2, key2, variable2);

        //     std::map<std::string, std::string> enumMap;
        //     for (size_t i = 2; i < setup.getNumNamedChildren(); i++)
        //     {
        //         ts::Node enumRoot = setup.getNamedChild(i); //enum description

        //         std::string_view enumName = enumRoot.getChildByFieldName("name").getSourceRange(sourcecode);
        //         std::string_view enumField = enumRoot.getChildByFieldName("description").getSourceRange(sourcecode);
        //         enumMap[std::string(enumName)] = std::string(enumField);
        //     }
        //     GameSettings::setupSettings[name] = std::make_shared<KindEnum>(KindEnum(kind, prompt, enumMap));
        // }

        Variable varMap = makeVar(std::get<varMapType>(map2));
        VariableUtils::setValue(map, key, varMap);
    }
    setupSettings = std::get<varMapType>(map);
}

varMapType GameSettings::valueMapParser(const ts::Node &node, const std::string_view sourcecode)
{
    ts::Node mapRoot = node.getNamedChild(0);
    varType map = varMapType();
    for (uint32_t i = 0; i < mapRoot.getNumNamedChildren(); i++)
    {
        ts::Node elementRoot = mapRoot.getNamedChild(i);
        varType key = std::string(elementRoot.getChildByFieldName("key").getSourceRange(sourcecode));
        Variable variable = recursiveExpressionParser(elementRoot.getChildByFieldName("value"), sourcecode);
        VariableUtils::setValue(map, key, variable);
    }

    return std::get<varMapType>(map);
}

// each node passed in should be of type expression
Variable GameSettings::recursiveExpressionParser(const ts::Node &node, const std::string_view sourcecode)
{
    if (node.getNamedChild(0).getType() == "value_map")
    {
        return var::makeVar(valueMapParser(node, sourcecode));
    }
    else if (node.getNamedChild(0).getType() == "list_literal")
    {
        varType listLiteral = listObj();
        if (!node.getNamedChild(0).getChildByFieldName("elements").isNull())
        {
            ts::Node elementsRoot = node.getNamedChild(0).getChildByFieldName("elements");
            for (uint32_t i = 0; i < elementsRoot.getNumNamedChildren(); i++)
            {
                varType value = recursiveExpressionParser(elementsRoot.getNamedChild(i), sourcecode).get();
                ListObjUtils::push_back(listLiteral, value);
            }
        }
        return var::makeVar(std::get<listObj>(listLiteral));
    }
    else if (node.getNamedChild(0).getType() == "quoted_string")
    {
        std::string value = std::string(node.getNamedChild(0).getSourceRange(sourcecode));
        return var::makeVar(value);
    }
    else if (node.getNamedChild(0).getType() == "number")
    {
        int value = parseInt(node.getNamedChild(0), sourcecode);
        return var::makeVar(value);
    }
    return var::makeVar(0);
}

void GameSettings::constantsParser(const ts::Node &root, const std::string_view sourcecode)
{
    ts::Node gameConstants = root.getChildByFieldName("constants");
    GameSettings::constants = valueMapParser(gameConstants, sourcecode);
}

void GameSettings::variablesParser(const ts::Node &root, const std::string_view sourcecode)
{
    ts::Node gameVariables = root.getChildByFieldName("variables");
    GameSettings::variables = valueMapParser(gameVariables, sourcecode);
}

void GameSettings::perPlayerParser(const ts::Node &root, const std::string_view sourcecode)
{
    ts::Node gamePlayerVariables = root.getChildByFieldName("per_player");
    GameSettings::perPlayerVariables = valueMapParser(gamePlayerVariables, sourcecode);
}

void GameSettings::perAudienceParser(const ts::Node &root, const std::string_view sourcecode)
{
    ts::Node gameAudienceVariables = root.getChildByFieldName("per_audience");
    GameSettings::perAudienceVariables = valueMapParser(gameAudienceVariables, sourcecode);
}