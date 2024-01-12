#include "SocialGamingTaskFactory.hpp"
#include "gameinstance.h"
#include "RuleInterpreter.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>


// fixtures
class TasksBasicTestFixture: public testing::Test
{
    protected:
    std::shared_ptr<GameInstance> game;
    SCConverter converter;
    std::shared_ptr<RuleNode> parsedtask;
    std::shared_ptr<RunnableTask> runnableTask;

    TasksBasicTestFixture():
        game(std::make_shared<GameInstance>("test", 0)),
        converter(buildDefaultConverter(game)){ }

    void SetUp(NodeType taskType)
    {
        std::vector<std::vector<std::string>> list;
        parsedtask = std::make_shared<TaskRuleNode>(list, taskType);
        runnableTask = converter.convert(parsedtask);

        ASSERT_NE(runnableTask, nullptr);
    }
};


TEST_F(TasksBasicTestFixture, reverseTest)
{
    // init
    SetUp(NodeType::REVERSE);

    // precondition
    std::shared_ptr<Variable> eList = (converter.tempArgs.at(NodeType::REVERSE).at(0));
    ASSERT_EQ(10, eList->size());
    ASSERT_EQ(NodeType::REVERSE, runnableTask->getType());

    testing::internal::CaptureStdout();
    eList->print();
    std::string output = testing::internal::GetCapturedStdout();
    ASSERT_EQ("0, 1, 2, 3, 4, 5, 6, 7, 8, 9, ", output);

    // execute
    runnableTask->run();

    // assert
    ASSERT_EQ(10, eList->size());
    ASSERT_EQ(NodeType::REVERSE, runnableTask->getType());

    testing::internal::CaptureStdout();
    eList->print();
    output = testing::internal::GetCapturedStdout();
    ASSERT_NE("0, 1, 2, 3, 4, 5, 6, 7, 8, 9, ", output);
    ASSERT_EQ("9, 8, 7, 6, 5, 4, 3, 2, 1, 0, ", output);

}

// unit tests
TEST_F(TasksBasicTestFixture, shuffleTest)
{
    // init
    SetUp(NodeType::SHUFFLE);

    // precondition
    std::shared_ptr<Variable> eList = (converter.tempArgs.at(NodeType::SHUFFLE).at(0));
    ASSERT_EQ(10, eList->size());
    ASSERT_EQ(NodeType::SHUFFLE, runnableTask->getType());

    testing::internal::CaptureStdout();
    eList->print();
    std::string output = testing::internal::GetCapturedStdout();
    ASSERT_EQ("0, 1, 2, 3, 4, 5, 6, 7, 8, 9, ", output);

    // execute
    runnableTask->run();

    // assert
    ASSERT_EQ(10, eList->size());
    ASSERT_EQ(NodeType::SHUFFLE, runnableTask->getType());

    testing::internal::CaptureStdout();
    eList->print();
    output = testing::internal::GetCapturedStdout();
    ASSERT_NE("0, 1, 2, 3, 4, 5, 6, 7, 8, 9, ", output);
}

TEST_F(TasksBasicTestFixture, extendTest)
{
    // init
    SetUp(NodeType::EXTEND);

    // precondition
    std::shared_ptr<Variable> eList = (converter.tempArgs.at(NodeType::EXTEND).at(0));
    ASSERT_EQ(3, eList->size());
    ASSERT_EQ(NodeType::EXTEND, runnableTask->getType());

    testing::internal::CaptureStdout();
    eList->print();
    std::string output = testing::internal::GetCapturedStdout();
    ASSERT_EQ("1, 2, 3, ", output);

    // execute
    runnableTask->run();

    // assert
    ASSERT_EQ(6, eList->size());
    ASSERT_EQ(NodeType::EXTEND, runnableTask->getType());

    testing::internal::CaptureStdout();
    eList->print();
    output = testing::internal::GetCapturedStdout();
    ASSERT_EQ("1, 2, 3, 4, 5, 6, ", output);
}

TEST_F(TasksBasicTestFixture, messageTest)
{
    // init
    SetUp(NodeType::MESSAGE);

    // execute
    runnableTask->run();

    // assert
    ASSERT_EQ(NodeType::MESSAGE, runnableTask->getType());

    std::vector<GameInstance::Msg> expected;
    GameInstance::Msg msg("1,2,3", {{"type", "message"}, {"data", "test message!"}});
    expected.push_back(msg);

    ASSERT_EQ(expected, game->playerHandler->getAllMsgs());
}
TEST_F(TasksBasicTestFixture, inputTest)
{
    // init
    SetUp(NodeType::INPUT_CHOICE);

    // execute
    runnableTask->run();

    // assert
    ASSERT_EQ(NodeType::INPUT_CHOICE, runnableTask->getType());

    std::vector<GameInstance::Msg> expected;
    GameInstance::Msg msg("1,2,3", {{"type", "input"}, {"prompt", "test prompt"}});
    expected.push_back(msg);

    ASSERT_EQ(expected, game->playerHandler->getAllMsgs());
}
TEST_F(TasksBasicTestFixture, inputRangeTest)
{
    // init
    // test range
    auto inputArgs = converter.tempArgs.at(NodeType::INPUT_CHOICE);
    inputArgs.push_back(makeVarPtr(1));
    inputArgs.push_back(makeVarPtr(10));
    inputArgs.at(2) = makeVarPtr("range");
    converter.tempArgs[NodeType::INPUT_CHOICE] = inputArgs;

    SetUp(NodeType::INPUT_CHOICE);

    runnableTask->run();

    // assert
    ASSERT_EQ(NodeType::INPUT_CHOICE, runnableTask->getType());

    std::vector<GameInstance::Msg> expected;
    GameInstance::Msg msg("1,2,3", {
                                    {"type",        "range"},
                                    {"prompt",      "test prompt"},
                                    {"rangeStart",  "1"},
                                    {"rangeEnd",    "10"}
                                    });
    expected.push_back(msg);

    ASSERT_EQ(expected, game->playerHandler->getAllMsgs());
}
TEST_F(TasksBasicTestFixture, inputChoicesTest)
{
    // init
    // test range
    auto inputArgs = converter.tempArgs.at(NodeType::INPUT_CHOICE);
    inputArgs.push_back(makeVarPtr("option1", "option2", "option3"));
    inputArgs.at(2) = makeVarPtr("choice");
    converter.tempArgs[NodeType::INPUT_CHOICE] = inputArgs;

    SetUp(NodeType::INPUT_CHOICE);

    runnableTask->run();

    // assert
    ASSERT_EQ(NodeType::INPUT_CHOICE, runnableTask->getType());

    std::vector<GameInstance::Msg> expected;
    GameInstance::Msg msg("1,2,3", {{"type",            "choice"},
                                    {"prompt",          "test prompt"},
                                    {"choiceListSize",  "3"},
                                    {"0",               "option1"},
                                    {"1",               "option2"},
                                    {"2",               "option3"}
                                    });
    expected.push_back(msg);


    auto msges = game->playerHandler->getAllMsgs();
    ASSERT_EQ(expected, msges);
}
TEST_F(TasksBasicTestFixture, scoresTest)
{
    // init
    SetUp(NodeType::SCORES);
    runnableTask->run();

    // assert
    ASSERT_EQ(NodeType::SCORES, runnableTask->getType());

    std::vector<GameInstance::Msg> expected;
    GameInstance::Msg msg("0",
        {   {"type",    "scores"},
            {"attr",    "points"},
            {"count",   "3"},
            {"name0",   "player1"},
            {"name1",   "player2"},
            {"name2",   "player3"},
            {"score0",  "100"},
            {"score1",  "200"},
            {"score2",  "50"}
        });
    expected.push_back(msg);

    ASSERT_EQ(expected, game->playerHandler->getAllMsgs());
}

TEST_F(TasksBasicTestFixture, assignmentTest)
{
    // init
    game->envMgr->setVariable("test", makeVar(2));
    SetUp(NodeType::ASSIGNMENT);

    ASSERT_EQ(NodeType::ASSIGNMENT, runnableTask->getType());

    ASSERT_TRUE(game->envMgr->getVariable("test")->isEqual(2));
    runnableTask->run();
    ASSERT_TRUE(game->envMgr->getVariable("test")->isEqual(1));
}
