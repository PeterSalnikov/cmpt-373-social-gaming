#include "EnvironmentMgr.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <filesystem>
#include <fstream>
#include <gmock/gmock.h>


using namespace var;
using namespace env_mgr;


extern "C" {
  TSLanguage* tree_sitter_socialgaming();
}

class EnvMgrFixture: public testing::Test
{
    protected:
    EnvironmentManager mgr;
    std::shared_ptr<RuleTree> ruleTree;

    EnvMgrFixture()
    {
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

        ts::Node rules = root.getChildByFieldName("rules");
        ruleTree = parseRules(rules, source);


        // env manager
        // hard code for now
        mgr.setVariable("configuration.rounds.upfrom(1)", makeVarPtr("round1", "round2", "round3"));
        listObj a = {makeVarPtr("player1")};
        mgr.setVariable("players", makeVarPtr(a));

        mgr.enterScope(ruleTree->getRules());
    }
};


TEST_F(EnvMgrFixture, getSetTest)
{
    // init
    std::string testKey = "hello";
    std::string testStr1 = "world";
    std::string testStr2 = "test";

    // asserts
    mgr.setVariable(testKey, makeVar(testStr1));
    auto var = mgr.getVariable(testKey);
    ASSERT_TRUE(var && var->isEqual(varType(testStr1)));

    mgr.setVariable(testKey, testStr2);
    var = mgr.getVariable(testKey);
    ASSERT_TRUE(var->isEqual(varType(testStr2)));
    ASSERT_EQ(nullptr, mgr.getVariable("no key"));
}

TEST_F(EnvMgrFixture, scopeTest)
{
    // init
    std::string testKey = "hello";
    std::string testKey2 = "test";

    std::string testStr1 = "world";
    std::string testStr2 = "test2";
    std::string testStr3 = "test3";

    // asserts
    // scope 1
    mgr.setVariable(testKey, makeVar(testStr1));
    ASSERT_NE(nullptr, mgr.getVariable(testKey));

    // goto next control flow node
    std::shared_ptr<RuleNode> discard = ruleTree->getRules()->getChildWithKey({"true"}).child;
    ASSERT_NE(nullptr, discard);
    std::shared_ptr<RuleNode> message = discard->getNextNode();
    ASSERT_NE(nullptr, message);
    std::shared_ptr<RuleNode> next = message->getNextNode();
    ASSERT_NE(nullptr, next);

    // scope 2
    ASSERT_EQ(NodeType::PARALLEL, next->getType());
    next = mgr.enterScope(next);

    mgr.setVariable(testKey2, makeVar(testStr2));
    auto var = mgr.getVariable(testKey2);
    ASSERT_TRUE(var && var->isEqual(varType(testStr2)));

    var = mgr.getVariable(testKey);
    ASSERT_TRUE(var && var->isEqual(varType(testStr1)));

    mgr.setVariable(testKey, makeVar(testStr3));

    // scope 1
    bool shouldBlock;
    next = mgr.exitScope(shouldBlock);
    ASSERT_FALSE(shouldBlock);

    ASSERT_EQ(nullptr, mgr.getVariable(testKey2));

    var = mgr.getVariable(testKey);
    ASSERT_TRUE(var && var->isEqual(varType(testStr3)));
}

TEST_F(EnvMgrFixture, timerTest)
{
    std::string flagName = "test";
    mgr.setVariable(flagName, makeVar(true));

    auto node = std::make_shared<ControlFlowRuleNode>(std::vector<std::vector<std::string>>(), NodeType::FOR);
    // auto node = ruleTree->getRules();
    Timer timer(1, node, 3, Timer::Type::STOP, flagName);

    ASSERT_EQ(2, mgr.depth());
    mgr.enterScope(node, timer);
    ASSERT_EQ(3, mgr.depth());

    auto timerPtr = mgr.getTimer(1);
    ASSERT_NE(nullptr, timerPtr);

    ASSERT_EQ(makeVar(false), *(mgr.getVariable(flagName)));
    ASSERT_FALSE(timerPtr->isExpired());

    sleep(2);
    ASSERT_FALSE(timerPtr->isExpired());
    sleep(2); // allow one second leeway
    ASSERT_TRUE(timerPtr->isExpired());

    ASSERT_FALSE(mgr.getVariable(flagName)->isEqual(true));
    auto newnode = mgr.updateTimers();
    ASSERT_EQ(2, mgr.depth());
    ASSERT_TRUE(mgr.getVariable(flagName)->isEqual(true));

    ASSERT_EQ(newnode, node);
}


TEST_F(EnvMgrFixture, forloopTest)
{
    // init
    auto first = ruleTree->getRules();
    auto forChild = first->getChildWithKey({"true"}).child;
    auto next = forChild;

    std::string loopVar = "round";
    std::string loopRange = "configuration.rounds.upfrom(1)";

    // asserts
    ASSERT_EQ(first->getType(), NodeType::FOR);
    ASSERT_NE(nullptr, mgr.getVariable(loopRange));

    std::vector<std::string> expected = {"round1", "round2", "round3"};
    std::for_each(expected.begin(), expected.end(),
    [&](const auto &result)
    {
        // scope 1
        // in for loop
        ASSERT_EQ(forChild, next);
        ASSERT_NE(nullptr, mgr.getVariable(loopVar));
        ASSERT_TRUE(mgr.getVariable(loopVar)->isEqual(result));
        // execute for loop...

        // scope 0
        bool shouldBlock;
        next = mgr.exitScope(shouldBlock); // loop -> at for
        ASSERT_EQ(nullptr, mgr.getVariable(loopVar));

        // next is different for last iteration and we dont want to enterScope
        if (&result == &expected.back())
            return;

        // scope 1
        // in for loop
        ASSERT_EQ(first, next);
        next = mgr.enterScope(next);
    });

    ASSERT_EQ(first->getNextNode(), next);
    ASSERT_EQ(nullptr, mgr.getVariable(loopVar));

    // continue after for loop...
}
