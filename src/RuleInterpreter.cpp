#include <RuleInterpreter.h>
#include <string>

std::map<std::string, enum NodeType> taskRules {
    {"extend", EXTEND},
    {"discard", DISCARD},
    {"shuffle", SHUFFLE},
    {"sort", SORT},
    {"deal", DEAL},
    {"input_choice", INPUT_CHOICE},
    {"message", MESSAGE},
    {"scores", SCORES},
    {"assignment", ASSIGNMENT},
    {"reverse", REVERSE},
};

std::string_view
getNodeValue(
	const std::string_view source,
	const ts::Node& node){

	return node.getSourceRange(source);
}

std::string_view
getChildNodeValue(
	const std::string_view source,
	const std::string_view childNodeName,
	const ts::Node& node){

	return node.getChildByFieldName(childNodeName).getSourceRange(source);
}

void
getAllChildNodeValuesHelper(
	const std::string_view source,
	const std::string_view childNodeType,
	const ts::Node& node, std::vector<std::string_view>& result){

	if(node.getNumNamedChildren() == 0) return;

	for(size_t i = 0; i < node.getNumNamedChildren(); i++){

		ts::Node current = node.getNamedChild(i);
		getAllChildNodeValuesHelper(source, childNodeType, current, result);

		if(current.getType() == childNodeType){
			result.push_back(current.getSourceRange(source));
		}
	}
}

std::vector<std::string_view>
getAllChildNodeValues(
	const std::string_view source,
	const std::string_view childNodeType,
	const ts::Node& node){

	std::vector<std::string_view> result;

	getAllChildNodeValuesHelper(source, childNodeType, node, result);

	return result;
}

void
printNodeRecursively(const ts::Node& node, int depth){

	for(size_t i = 0; i < node.getNumNamedChildren(); i++){

		ts::Node current = node.getNamedChild(i);
		printf("%*s%.*s\n", depth, "", static_cast<int>(current.getType().length()), current.getType().data());
		printNodeRecursively(current, ++depth);
	}
}

void printVector(std::vector<std::vector<std::string>> list){
	
	for(auto expressionList : list){
		for(auto element : expressionList){
			std::cout << element << std::endl;
		}	
	}
}

void
printRuleTreeRecursively(RuleNode* node, int depth){
	if(node->isControlFlow()){
		ControlFlowRuleNode* controlFlowNode = (ControlFlowRuleNode*)node;
		printf("%*stype:%d", depth*2, "", controlFlowNode->getType());
		printVector(node->getData());
		for(auto child : controlFlowNode->getBody()){
			printRuleTreeRecursively(child.child.get(), depth + 1);
		}
	} else {
		printf("%*stype:%d\n", depth*2, "", node->getType());
		printVector(node->getData());
	}
	if(node->getNextNode().get()){
		printRuleTreeRecursively(node->getNextNode().get(), depth);
	}
}

//pass in node with fieldname "rules"
std::shared_ptr<RuleTree> parseRules(const ts::Node &node, const std::string_view sourcecode){
	ts::Node bodyNode = node.getChildByFieldName("body");

	return std::shared_ptr<RuleTree>(new RuleTree(parseControlBody(bodyNode, sourcecode, nullptr)));
}

//pass in rule node
std::shared_ptr<RuleNode> parseRule(const ts::Node &node, const std::string_view sourcecode){
	if(node.getType() != "rule"){
		throw "parse rule passed non-rule";
	}

	if(node.getNamedChild(0).getType() == "for"){
		return parseForRule(node, sourcecode, FOR);
	} else if(node.getNamedChild(0).getType() == "parallel_for"){
		return parseForRule(node, sourcecode, PARALLEL);
	} else if(node.getNamedChild(0).getType() == "match"){
		return parseMatchRule(node, sourcecode);
	}

  	//has to be a task
	return parseTask(node, sourcecode);
}

//pass in body node
std::shared_ptr<RuleNode> parseControlBody(const ts::Node &bodyNode, const std::string_view sourcecode, std::shared_ptr<ControlFlowRuleNode> parent){
	std::shared_ptr<RuleNode> prevNode = nullptr;
	std::shared_ptr<RuleNode> firstNode;
	for (size_t i = 0; i < bodyNode.getNumNamedChildren(); i++)
	{
		//skip comments
		if(bodyNode.getNamedChild(i).getType() == "comment"){
			continue;
		}

		std::shared_ptr<RuleNode> node = parseRule(bodyNode.getNamedChild(i), sourcecode);

		node.get()->setParentNode(parent);

		// if not the first node, set current node to be the next node of the previous node
		if(prevNode != nullptr){
			prevNode.get()->setNextNode(node);
			prevNode = prevNode.get()->getNextNode();
		} else{
			firstNode = node;
			prevNode = firstNode;
		}

	}

	return firstNode;
}

void parseExpression(const ts::Node &node, const std::string_view sourcecode, std::vector<std::string>& expressionList){
	if(node.getType() == "expression_list"){
		for(size_t i = 0; i < node.getNumNamedChildren(); i++){
			parseExpression(node.getNamedChild(i), sourcecode, expressionList);
		}
	} else if(!node.getChildByFieldName("builtin").isNull()){
		expressionList.push_back((std::string)node.getNamedChild(0).getSourceRange(sourcecode));
		expressionList.push_back((std::string)node.getChildByFieldName("builtin").getSourceRange(sourcecode));
		if(node.getNamedChild(2).getNumNamedChildren() != 0){
			parseExpression(node.getNamedChild(2).getChildByFieldName("arguments"), sourcecode, expressionList);
		}
	} else if(!node.getChildByFieldName("operand").isNull()){
		expressionList.push_back("!");
		parseExpression(node.getChildByFieldName("operand"), sourcecode, expressionList);
	} else if(!node.getChildByFieldName("lhs").isNull()){
		//get operator in middle of string (scuffed but no real way around it)
		int length = node.getChildByFieldName("lhs").getSourceRange(sourcecode).length();
		auto operatorValue = node.getSourceRange(sourcecode)[length + 1];
		std::string charValue(1, operatorValue);
		expressionList.push_back(charValue);

		parseExpression(node.getChildByFieldName("lhs"), sourcecode, expressionList);
		parseExpression(node.getChildByFieldName("rhs"), sourcecode, expressionList);
	} else {
		if(node.getSourceRange(sourcecode) == "all"){
			expressionList.push_back("players");
		} else{
			expressionList.push_back((std::string)node.getSourceRange(sourcecode));
		}
		
	}
}

//pass in expression node
std::shared_ptr<TaskRuleNode> parseTask(const ts::Node &node, const std::string_view sourcecode){
	ts::Node ruleNode = node.getNamedChild(0);

	const auto it = taskRules.find(std::string { ruleNode.getType() });

	std::vector<std::vector<std::string>> list = {};
	int count = 0;

	list.push_back({(std::string)ruleNode.getType()});

	switch (it->second)
	{
		case EXTEND:
            break;
		case DISCARD:
            break;
		case SHUFFLE:
            break;
        case REVERSE:
            break;
		case SORT:
		case DEAL:
		case MESSAGE:
			count = 2;
			break;
		case ASSIGNMENT:{
			std::vector<std::string> expressionList = {};
			parseExpression(ruleNode.getNamedChild(0), sourcecode, expressionList);
			expressionList[0] = "\"" + expressionList[0] + "\"";
			list.push_back(expressionList);
			expressionList = {};
			parseExpression(ruleNode.getNamedChild(1), sourcecode, expressionList);
			list.push_back(expressionList);
			count = 0;
			break;
		}
		case INPUT_CHOICE:{
			std::vector<std::string> expressionList = {};
			parseExpression(ruleNode.getNamedChild(0), sourcecode, expressionList);
			expressionList[0] = expressionList[0] + ".id";
			list.push_back(expressionList);
			expressionList = {};
			parseExpression(ruleNode.getNamedChild(1), sourcecode, expressionList);
			list.push_back(expressionList);
			expressionList = {};
			parseExpression(ruleNode.getNamedChild(2), sourcecode, expressionList);
			list.push_back(expressionList);
			expressionList = {};
			parseExpression(ruleNode.getNamedChild(3), sourcecode, expressionList);
			expressionList[0] = "\"" + expressionList[0] + "\"";
			list.push_back(expressionList);
			expressionList = {};
			parseExpression(ruleNode.getNamedChild(4), sourcecode, expressionList);
			list.push_back(expressionList);
			count = 0;
			break;
		}
		case SCORES:
			count = 1;
			break;
	}

	for (int i = 0; i < count; i++)
	{
		std::vector<std::string> expressionList = {};
		parseExpression(ruleNode.getNamedChild(i), sourcecode, expressionList);
		list.push_back(expressionList);
	}

	return std::shared_ptr<TaskRuleNode> (new TaskRuleNode(list, it->second));
}

std::shared_ptr<ControlFlowRuleNode> parseForRule(const ts::Node &node, const std::string_view sourcecode, NodeType type){
	std::vector<std::vector<std::string>> list = {};

	std::vector<std::string> expressionList = {};
	parseExpression(node.getNamedChild(0).getChildByFieldName("element"), sourcecode, expressionList);
	list.push_back(expressionList);

	std::vector<std::string> expressionList2 = {};
	parseExpression(node.getNamedChild(0).getChildByFieldName("list"), sourcecode, expressionList2);
	list.push_back(expressionList2);	

	ts::Node bodyNode = node.getNamedChild(0).getChildByFieldName("body");

	std::shared_ptr<ControlFlowRuleNode> forNode = std::shared_ptr<ControlFlowRuleNode> (new ControlFlowRuleNode(list, type));

	forNode.get()->setChildren({"true"}, parseControlBody(bodyNode, sourcecode, forNode));

	return forNode;
}
std::shared_ptr<ControlFlowRuleNode> parseMatchRule(const ts::Node &node, const std::string_view sourcecode){
	ts::Node targetNode = node.getNamedChild(0).getChildByFieldName("target");

	std::vector<std::string> list = {};

	parseExpression(targetNode, sourcecode, list);

	int matchEntryCount = node.getNamedChild(0).getNumNamedChildren();

	std::shared_ptr<ControlFlowRuleNode> matchNode = std::shared_ptr<ControlFlowRuleNode> (new ControlFlowRuleNode({list}, MATCH));

	for (int i = 1; i < matchEntryCount; i++)
	{
		ts::Node matchEntry = node.getNamedChild(0).getNamedChild(i);
		std::vector<std::string> expressionList = {};

		parseExpression(matchEntry.getChildByFieldName("guard"), sourcecode, expressionList);

		ts::Node bodyNode = matchEntry.getChildByFieldName("body");

		matchNode.get()->setChildren(expressionList, parseControlBody(bodyNode, sourcecode, matchNode));
	}

	return matchNode;
}