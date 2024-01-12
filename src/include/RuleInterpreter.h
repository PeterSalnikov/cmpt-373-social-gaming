# pragma once

#include <algorithm>
#include <iostream>
#include <string_view>
#include <vector>
#include <map>
#include <numeric>
#include <memory>
#include <algorithm>
#include <ranges>

#include <cpp-tree-sitter.h>

#include "Message.h"
#include "Task.h"

class RuleNode;

// free function to help with parsing
std::string_view getNodeValue(const std::string_view source, const ts::Node& node);
std::string_view getChildNodeValue(const std::string_view source, const std::string_view childNodeName, const ts::Node& node);

//recursively finds all child nodes with the name/type == childNodeType
std::vector<std::string_view> getAllChildNodeValues(const std::string_view source, const std::string_view childNodeType, const ts::Node& node);


void printNodeRecursively(const ts::Node& node, int depth);

void printRuleTreeRecursively(RuleNode* node, int depth);

class RuleInterpreter {
public:
	RuleInterpreter(std::string_view game, std::string source) : game(game), source(source) { }

	std::string_view getSource() { return source; }

private:

	std::string game;
	std::string source;
};

enum NodeType
{
	FOR, PARALLEL, MATCH, EXTEND, DISCARD, SHUFFLE, SORT, DEAL, INPUT_CHOICE, MESSAGE, SCORES, ASSIGNMENT, REVERSE
};

struct ChildNode{
	std::vector<std::string> key;
	std::shared_ptr<RuleNode> child;
};

class RuleTree{
public:
	RuleTree(std::shared_ptr<RuleNode> firstNode) : firstNode(firstNode) { }
	std::shared_ptr<RuleNode> getRules() const {return firstNode;}
private:
	std::shared_ptr<RuleNode> firstNode;
};

class RuleNode{
public:
    RuleNode(std::vector<std::vector<std::string>> list, NodeType type) : list(list), type(type) {}
	virtual bool isControlFlow() const = 0;
	std::shared_ptr<RuleNode> getNextNode() {return nextNode;}
	std::weak_ptr<RuleNode> getParentNode() {return parentNode;}
    void setNextNode(std::shared_ptr<RuleNode> next) {nextNode = next;}
    void setParentNode(std::weak_ptr<RuleNode> parent) {parentNode = parent;}

    std::vector<std::vector<std::string>> getData() {return list;};
	NodeType getType() const {return type;}
	virtual std::vector<ChildNode> getBody() const = 0;
    virtual ChildNode getChildWithKey(std::vector<std::string> key) const = 0;
protected:
	std::shared_ptr<RuleNode> nextNode = nullptr;
	std::weak_ptr<RuleNode> parentNode;
	std::vector<std::vector<std::string>> list;
    NodeType type;
};

class TaskRuleNode : public RuleNode{
public:
    TaskRuleNode(std::vector<std::vector<std::string>> list, NodeType type) : RuleNode(list, type) {}
	bool isControlFlow() const {return false;}
	std::vector<ChildNode> getBody() const {return {};};
    ChildNode getChildWithKey(std::vector<std::string> key) const override { return ChildNode(); }
};

// if condition is false, go to nextNode
class ControlFlowRuleNode : public RuleNode{
public:
	ControlFlowRuleNode(std::vector<std::vector<std::string>> list, NodeType type) : RuleNode(list, type) {}
	bool isControlFlow() const {return true;}
    void setChildren(std::vector<std::string> guard, std::shared_ptr<RuleNode> childNode) {
		ChildNode node = {guard, childNode};
		children.push_back(node);
	}
	std::vector<ChildNode> getBody() const {return children;}
    ChildNode getChildWithKey(std::vector<std::string> key) const override
    {
        namespace views = std::ranges::views;
        auto view = children
                    | views::filter([&key](const auto &child) { return child.key == key; })
                    | views::take(1);
        return *view.begin();
    }
private:
	//using struct due to ordering of map not being chronological
	std::vector<ChildNode> children;
};

std::shared_ptr<RuleNode> parseControlBody(const ts::Node &bodyNode, const std::string_view sourcecode, std::shared_ptr<ControlFlowRuleNode> parent);

std::shared_ptr<ControlFlowRuleNode> parseForRule(const ts::Node &node, const std::string_view sourcecode, NodeType type);

std::shared_ptr<ControlFlowRuleNode> parseMatchRule(const ts::Node &node, const std::string_view sourcecode);

std::shared_ptr<TaskRuleNode> parseTask(const ts::Node &node, const std::string_view sourcecode);

std::shared_ptr<RuleTree> parseRules(const ts::Node &node, const std::string_view sourcecode);