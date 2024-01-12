#ifndef SG_TASK_FACTORY_H
#define SG_TASK_FACTORY_H
#include "TaskFactory.hpp"
#include "RuleInterpreter.h"
#include <iostream>
#include "gameinstance.h"
#include "EnvironmentMgr.hpp"


using namespace taskFactory;
using namespace var;
using namespace env_mgr;

class PlayerHandler;
class GameInstance;

using listSharedPtr = std::shared_ptr<listObj>;
using playerHandlerPtr = std::shared_ptr<PlayerHandler>;
using environmentMgrPtr = std::shared_ptr<EnvironmentManager>;
using ruleNodeType = RuleNode;
using nodeTypeEnum = NodeType;

// ======================================factory definitions======================================
template<typename TaskKind>
class DefaultFactory : public TaskFactory {
    public:
    std::shared_ptr<RunnableTask> create(mutableVarPointerVector &vars) const override;
};


// ========================================task definitions========================================
// to add a task:
// - create class newTask: public RunnableTask
// - create DefaultFactory<newTask>: public TaskFactory
// - add task to converter in buildDefaultConverter()
//
// ========================================reverse definitions========================================
class ReverseTask: public RunnableTask
{
    public:
        ReverseTask(Variable &list);
        void run() override;
        int getType() const override { return nodeTypeEnum::REVERSE; }
    private:
        varTypeBorrowedPtr list;
};
template<>
class DefaultFactory<ReverseTask>: public TaskFactory {
    public:
    std::shared_ptr<RunnableTask> create(mutableVarPointerVector &vars) const override;
};
using ReverseFactory       = DefaultFactory<ReverseTask>;


// ========================================shuffle definitions========================================
class ShuffleTask: public RunnableTask
{
    public:
        ShuffleTask(Variable &list);
        void run() override;
        int getType() const override { return nodeTypeEnum::SHUFFLE; }
    private:
        varTypeBorrowedPtr list;
};
template<>
class DefaultFactory<ShuffleTask>: public TaskFactory {
    public:
    std::shared_ptr<RunnableTask> create(mutableVarPointerVector &vars) const override;
};
using ShuffleFactory       = DefaultFactory<ShuffleTask>;


// ========================================extend definitions========================================
class ExtendTask: public RunnableTask
{
    public:
        ExtendTask(Variable &anAddList, const listObj &someAddElems);
        void run() override;
        int getType() const override { return nodeTypeEnum::EXTEND; }
    private:
        varTypeBorrowedPtr addList;
        const listObj addElems;
};
template<>
class DefaultFactory<ExtendTask>: public TaskFactory {
    public:
    std::shared_ptr<RunnableTask> create(mutableVarPointerVector &vars) const override;
};
using ExtendFactory       = DefaultFactory<ExtendTask>;

// ========================================input definitions========================================
class InputTask: public RunnableTask
{
    public:
        InputTask(const listObj &pList, std::string_view aPrompt, std::string_view aType, const playerHandlerPtr &handler);
        InputTask(const listObj &pList, std::string_view aPrompt, std::string_view aType, const playerHandlerPtr &handler,
                  int rangeStart, int rangeEnd);
        InputTask(const listObj &pList, std::string_view aPrompt, std::string_view aType, const playerHandlerPtr &handler,
                  const listObj &vals);

        void run() override;
        int getType() const override { return nodeTypeEnum::INPUT_CHOICE; }
    private:
        // required
        const std::vector<int> playerList;
        const std::string prompt;
        const std::string type;
        const playerHandlerPtr playerHandler;
        // optional
        const int start = -1;
        const int end = -1;
        const std::vector<std::string> choices;
};
template<>
class DefaultFactory<InputTask>: public TaskFactory {
    public:
    DefaultFactory(const playerHandlerPtr &phandler):playerHandler(phandler) {}
    std::shared_ptr<RunnableTask> create(mutableVarPointerVector &vars) const override;
    private:
    const playerHandlerPtr playerHandler;
};
using InputFactory       = DefaultFactory<InputTask>;

// ========================================message definitions========================================
class MessageTask: public RunnableTask
{
    public:
        MessageTask(const playerHandlerPtr &aPlayerHandler, const listObj &aPlayerList, std::string_view anMessage);
        void run() override;
        int getType() const override { return nodeTypeEnum::MESSAGE; }
    private:
        const playerHandlerPtr playerHandler;
        const std::vector<int> playerList;
        const std::string message;
};
template<>
class DefaultFactory<MessageTask>: public TaskFactory {
    public:
    DefaultFactory(const playerHandlerPtr &phandler):playerHandler(phandler) {}
    std::shared_ptr<RunnableTask> create(mutableVarPointerVector &vars) const override;
    private:
    const playerHandlerPtr playerHandler;
};
using MessageFactory       = DefaultFactory<MessageTask>;
// ========================================scores definitions========================================
class ScoresTask: public RunnableTask
{
    public:
        ScoresTask(const playerHandlerPtr &aPlayerHandler, const int anOwnerId,
                   const listObj &somePlayerNames, const listObj &someScores,
                   std::string_view anAttribute);
        void run() override;
        int getType() const override { return nodeTypeEnum::SCORES; }
    private:
        const playerHandlerPtr playerHandler;
        const std::vector<std::string> playerNames;
        const listObj playerScores; // could change so store pointer
        const std::string attrName;
        const int ownerId;
};
template<>
class DefaultFactory<ScoresTask>: public TaskFactory {
    public:
    DefaultFactory(const playerHandlerPtr &phandler):playerHandler(phandler) {}
    std::shared_ptr<RunnableTask> create(mutableVarPointerVector &vars) const override;
    private:
    const playerHandlerPtr playerHandler;
};
using ScoresFactory       = DefaultFactory<ScoresTask>;

// ========================================scores definitions========================================
class AssignmentTask: public RunnableTask
{
    public:
        AssignmentTask(const environmentMgrPtr &amgr, const std::string &aname, const std::shared_ptr<Variable> &aval);
        void run() override;
        int getType() const override { return nodeTypeEnum::ASSIGNMENT; }
    private:
        environmentMgrPtr mgr;
        std::string_view name;
        const std::shared_ptr<Variable> val;
};
template<>
class DefaultFactory<AssignmentTask>: public TaskFactory {
    public:
    DefaultFactory(const environmentMgrPtr &eMgr):envMgr(eMgr) {}
    std::shared_ptr<RunnableTask> create(mutableVarPointerVector &vars) const override;
    private:
    const environmentMgrPtr envMgr;
};
using AssignmentFactory       = DefaultFactory<AssignmentTask>;


// ========================================discard definitions========================================
// class DiscardTask: public RunnableTask
// {
//     public:
//         DiscardTask(listSharedPtr discardFrom, int aNumToDiscard):
//             discardFrom(discardFrom),
//             numToDiscard(aNumToDiscard) {}
//         void run() override;
//     private:
//         listSharedPtr discardFrom;
//         int numToDiscard;
// };

// template<>
// class DefaultFactory<DiscardTask>: public TaskFactory {
//     public:
//     std::shared_ptr<RunnableTask> create(mutableVarPointerVector vars) const override;
// };
// using DiscardFactory = DefaultFactory<DiscardTask>;

// converter
class SCConverter: public Converter<ruleNodeType, nodeTypeEnum, GameInstance>
{
    public:
        SCConverter(std::shared_ptr<GameInstance> aSrc): src(aSrc)
        {
            tempArgs[nodeTypeEnum::REVERSE] = {makeVarPtr(0,1,2,3,4,5,6,7,8,9)};
            tempArgs[nodeTypeEnum::SHUFFLE] = {makeVarPtr(0,1,2,3,4,5,6,7,8,9)};
            tempArgs[nodeTypeEnum::EXTEND] = {makeVarPtr(1,2,3), makeVarPtr(4,5,6)};
            tempArgs[nodeTypeEnum::INPUT_CHOICE] = {makeVarPtr(1,2,3), makeVarPtr("test prompt"), makeVarPtr("input")};
            tempArgs[nodeTypeEnum::SCORES] = {makeVarPtr(0), makeVarPtr("player1", "player2", "player3"),
                                            makeVarPtr(100, 200, 50), makeVarPtr("points")};
            tempArgs[nodeTypeEnum::MESSAGE] = {makeVarPtr(1,2,3), makeVarPtr("test message!")};
            tempArgs[nodeTypeEnum::DISCARD] = {makeVarPtr(1,2,3), makeVarPtr(1)};
            tempArgs[nodeTypeEnum::ASSIGNMENT] = {makeVarPtr("test"), makeVarPtr(1)};
        }
        std::shared_ptr<RunnableTask> convert(std::shared_ptr<ruleNodeType> task) override;
        void addFactory(nodeTypeEnum e, std::shared_ptr<TaskFactory> factory) override;
        // [TEMP]
        std::map<nodeTypeEnum, mutableVarPointerVector> tempArgs;
        std::shared_ptr<GameInstance> src;
    private:
        std::map<nodeTypeEnum, std::shared_ptr<TaskFactory>> factories;
};


SCConverter buildDefaultConverter(std::shared_ptr<GameInstance> game);

#endif