#include "SocialGamingTaskFactory.hpp"
#include "Variables.hpp"
#include <sstream>
#include <iostream>
#include <numeric>
#include <algorithm>
#include <ranges>
#include "gameinstance.h"
#include "EnvironmentMgr.hpp"

using namespace taskFactory;
using namespace var;
using namespace env_mgr;

using namespace var;
template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
template<class... Ts> overload(Ts...) -> overload<Ts...>;


// helper functions
template <typename T>
std::vector<T> copyVec(const listObj &vec)
{
    namespace views = std::ranges::views;
    auto view = vec
                | views::transform([](const auto &item) { return std::get_if<T>(item->getBorrowPtr()); })
                | views::filter([](const auto &item) { return item != nullptr; })
                | views::transform([](const auto &item) { return *(item); });
    return {view.begin(), view.end()};
}

struct Min
{
    Min(size_t aval): val(aval) {}
    size_t val;
};
struct Max
{
    Max(size_t aval): val(aval) {}
    size_t val;
};
bool checkPreconditions(Min sizeMin, Max sizeMax, std::string_view sizeErrorMsg,
    const mutableVarPointerVector &vars)
{
    std::string errorMsg = std::string(sizeErrorMsg);
    bool ret = false;
    bool noNullptr = std::all_of(vars.begin(), vars.end(), [](const auto &val) { return val != nullptr; });

    if(sizeMin.val > vars.size() || sizeMax.val < vars.size())
    { }
    else if(!noNullptr)
    { errorMsg = "nullptr passed to TaskFactory";}
    else
    { ret = true; }

    if(!ret)
    { throw BadVariableArgException(errorMsg); }

    return ret;
}
bool checkPreconditions(Min sizeMin, Max sizeMax, std::string_view sizeErrorMsg,
    const mutableVarPointerVector &vars, const playerHandlerPtr &pHndlr)
{
    if(pHndlr == nullptr)
    {
        throw BadVariableArgException("Expected player Handler to be not null");
        return false;
    }
    return checkPreconditions(sizeMin, sizeMax, sizeErrorMsg, vars);
}
bool checkPreconditions(Min sizeMin, Max sizeMax, std::string_view sizeErrorMsg,
    const mutableVarPointerVector &vars, const environmentMgrPtr &eMgr)
{
    if(eMgr == nullptr)
    {
        throw BadVariableArgException("Expected environment Manager to be not null");
        return false;
    }
    return checkPreconditions(sizeMin, sizeMax, sizeErrorMsg, vars);
}


// ===========================================tasks===========================================
// ========================================reverse definitions========================================
ReverseTask::ReverseTask(Variable &listToReverse):
    list(listToReverse.getBorrowPtr())
{ }
void ReverseTask::run()
{
    if(list == nullptr)
    {
        debugPrint("list is null, cannot reverse");
        return;
    }
    ListObjUtils::reverse(*list);

}

// ========================================shuffle definitions========================================
ShuffleTask::ShuffleTask(Variable &listToShuffle):
    list(listToShuffle.getBorrowPtr())
{ }
void ShuffleTask::run()
{
    if(list == nullptr)
    {
        debugPrint("list is null, cannot shuffle");
        return;
    }
    ListObjUtils::shuffle(*list);

}

// ========================================extend definitions========================================
ExtendTask::ExtendTask(Variable &anAddList, const listObj &someAddElems):
    addList(anAddList.getBorrowPtr()),
    addElems(someAddElems)
{ }
void ExtendTask::run()
{
    if(addList == nullptr)
    {
        debugPrint("list is null, cannot extend");
        return;
    }

    std::for_each(addElems.begin(), addElems.end(),
    [addList = this->addList](const auto &item)
    {
        ListObjUtils::push_back(*addList, item->getRef());
    }); // for_each
}

// ========================================input definitions========================================
InputTask::InputTask(const listObj &pList, std::string_view aPrompt, std::string_view aType,
    const playerHandlerPtr &handler):
    playerList(copyVec<int>(pList)),
    prompt(aPrompt),
    type(aType),
    playerHandler(handler)
{  }
InputTask::InputTask(const listObj &pList, std::string_view aPrompt, std::string_view aType,
            const playerHandlerPtr &handler, int rangeStart, int rangeEnd):
    playerList(copyVec<int>(pList)),
    prompt(aPrompt),
    type(aType),
    playerHandler(handler),
    start(rangeStart),
    end(rangeEnd)
{  }
InputTask::InputTask(const listObj &pList, std::string_view aPrompt, std::string_view aType,
    const playerHandlerPtr &handler, const listObj &vals):
    playerList(copyVec<int>(pList)),
    prompt(aPrompt),
    type(aType),
    playerHandler(handler),
    choices(copyVec<std::string>(vals))
{ }
void InputTask::run()
{
    // functions
    auto setRange = [](GameInstance::msgType &msg, int start, int end)
    {
        if(start == -1 || end == -1) { return; }

        msg["rangeStart"] = std::to_string(start);
        msg["rangeEnd"] = std::to_string(end);
    };
    auto setChoices = [](GameInstance::msgType &msg, const std::vector<std::string> &choices)
    {
        if(choices.size() <= 0) { return; }

        msg["choiceListSize"] = std::to_string(choices.size());
        std::accumulate(choices.begin(), choices.end(), 0,
        [&msg](int i, const auto &choice)
        {
            msg[std::to_string(i)] = choice;
            return i+1;
        }); // accumulate
    };

    std::string ids = std::accumulate(playerList.begin(), playerList.end(), std::string{},
        [](std::string curr, const auto &id) { return curr + "," + std::to_string(id); });
    ids.erase(ids.begin()); // remove first comma

    GameInstance::msgType msg;
    msg["prompt"] = prompt;
    msg["type"] = type;

    setRange(msg, start, end);
    setChoices(msg, choices);

    playerHandler->queueMessage(ids, msg);
}

// ========================================message definitions========================================
MessageTask::MessageTask(const playerHandlerPtr &aPlayerHandler, const listObj &aPlayerList,
    std::string_view anMessage):
    playerHandler(std::move(aPlayerHandler)),
    playerList(copyVec<int>(aPlayerList)),
    message(anMessage)
{}
void MessageTask::run()
{
    std::string ids = std::accumulate(playerList.begin(), playerList.end(), std::string{},
        [](std::string curr, const auto &id) { return curr + "," + std::to_string(id); });
    ids.erase(ids.begin()); // remove first comma

    GameInstance::msgType msg;
    msg["type"] = "message";
    msg["data"] = message;
    playerHandler->queueMessage(ids, msg);
}

// ========================================scores definitions========================================
ScoresTask::ScoresTask(const playerHandlerPtr &aPlayerHandler, const int anOwnerId,
    const listObj &somePlayerNames, const listObj &someScores, std::string_view anAttribute):
    playerHandler(aPlayerHandler),
    playerNames(copyVec<std::string>(somePlayerNames)),
    playerScores(someScores),
    attrName(anAttribute),
    ownerId(anOwnerId) {}
void ScoresTask::run()
{
    GameInstance::msgType msg;
    msg["type"] = "scores";
    msg["attr"] = attrName;
    msg["count"] = std::to_string(playerNames.size());
    std::accumulate(playerNames.begin(), playerNames.end(), 0,
        [&](int curr, const auto& name)
        {
            msg["name" + std::to_string(curr)] = name;
            return curr + 1;
        });
    std::accumulate(playerScores.begin(), playerScores.end(), 0,
        [&](int curr, const auto& scorePtr)
        {
            std::stringstream stream;
            VariableUtils::printValue(scorePtr->getRef(), "", stream);
            msg["score" + std::to_string(curr)] = stream.str();
            return curr + 1;
        });
    playerHandler->queueMessage(std::to_string(ownerId), msg);
}

// ========================================assignment definitions========================================
AssignmentTask::AssignmentTask(const std::shared_ptr<EnvironmentManager> &amgr, const std::string &aname,
    const std::shared_ptr<Variable> &aval):
    mgr(amgr), name(aname), val(aval)
{ }

void AssignmentTask::run()
{
    mgr->setVariable(name, val);
}


// void DiscardTask::run()
// {
//     // discard the # of elements, not a particular element
//     discardFrom->remove(std::move(numToDiscard));
// }


// ============================================factories===========================================
std::shared_ptr<RunnableTask> DefaultFactory<ReverseTask>::create(mutableVarPointerVector &vars) const
{
    // preconditions
    if(vars.size() != 1)
    { 
        throw BadVariableArgException("Expected a single argument"); return nullptr; 
    }

    return (std::shared_ptr<RunnableTask>) std::make_shared<ReverseTask>(*vars.at(0));
}

std::shared_ptr<RunnableTask> DefaultFactory<ShuffleTask>::create(mutableVarPointerVector &vars) const
{
    // preconditions
    if(vars.size() != 1)
    {
        throw BadVariableArgException("Expected a single argument"); return nullptr;
    }

    return (std::shared_ptr<RunnableTask>) std::make_shared<ShuffleTask>(*vars.at(0));
}

std::shared_ptr<RunnableTask> DefaultFactory<ExtendTask>::create(mutableVarPointerVector &vars) const
{
    // preconditions
    if(!checkPreconditions(Min(2), Max(2), "Expected two Variable arguments", vars))
    { return nullptr; }

    listObj* addElems;
    if ((           std::get_if<listObj>(&vars.at(0)->getRef())) == nullptr ||
        (addElems = std::get_if<listObj>(&vars.at(1)->getRef())) == nullptr   )
    { throw BadVariableArgException("Expected two listObjs");           return nullptr; }

    // we need to modify existing variable s pass first arg as reference (and store in resulting object)
    return (std::shared_ptr<RunnableTask>) std::make_shared<ExtendTask>(*vars.at(0), *addElems);
}


std::shared_ptr<RunnableTask> DefaultFactory<InputTask>::create(mutableVarPointerVector &vars) const
{
    // preconditions
    if(!checkPreconditions(Min(3), Max(5), "Expected between 3 and 5 Variable arguments", vars, playerHandler))
    { return nullptr; }

    size_t varsSize = vars.size();
    listObj* playerList;
    std::string* prompt;
    std::string* type;
    // check minimum required args
    if ((playerList = std::get_if   <listObj>       (&vars.at(0)->getRef())) == nullptr ||
        (prompt     = std::get_if   <std::string>   (&vars.at(1)->getRef())) == nullptr ||
        (type       = std::get_if   <std::string>   (&vars.at(2)->getRef())) == nullptr   )
    { throw BadVariableArgException("Expected listObj, str, str");                  return nullptr; }

    // make object depending on arguments passed in
    std::shared_ptr<RunnableTask> ret = nullptr;
    enum inputTypes { DEFAULT=3, CHOICES=4, RANGE=5 };
    switch(varsSize)
    {
        case inputTypes::CHOICES:
        {
            if(auto choices = std::get_if<listObj>(&vars.at(varsSize - 1)->getRef()))
            {
                ret = std::make_shared<InputTask>(*playerList, *prompt, *type, playerHandler,
                                                *choices);
                break;
            }
            throw BadVariableArgException("Expected listObj, str, str, listObj");
        } break;
        case inputTypes::RANGE:
        {
            int* start;
            int* end;
            if((start  = std::get_if<int>(&vars.at(varsSize - 2)->getRef())) &&
               (end    = std::get_if<int>(&vars.at(varsSize - 1)->getRef()))   )
            {
                ret = std::make_shared<InputTask>(*playerList, *prompt, *type, playerHandler,
                                                *start, *end);
                break;
            }
            throw BadVariableArgException("Expected listObj, str, str, listObj");
        } break;
        default:
            ret = std::make_shared<InputTask>(*playerList, *prompt, *type, playerHandler);
    }
    return ret;
}

std::shared_ptr<RunnableTask> DefaultFactory<MessageTask>::create(mutableVarPointerVector &vars) const
{
    // preconditions
    if(!checkPreconditions(Min(2), Max(2), "Expected two Variable arguments", vars, playerHandler))
    { return nullptr; }

    listObj* list;
    std::string* addElems;
    if ((list       = std::get_if<listObj>       (&vars.at(0)->getRef())) &&
        (addElems   = std::get_if<std::string>   (&vars.at(1)->getRef()))   )
    {
        return (std::shared_ptr<RunnableTask>) std::make_shared<MessageTask>(playerHandler, *list, *addElems);
    }

    throw BadVariableArgException("Expected two listObjs");
    return nullptr;
}

std::shared_ptr<RunnableTask> DefaultFactory<ScoresTask>::create(mutableVarPointerVector &vars) const
{
    // preconditions
    if(!checkPreconditions(Min(4), Max(4), "Expected four Variable arguments", vars, playerHandler))
    { return nullptr; }

    listObj* nameList;
    listObj* scoreList;
    int* ownerId;
    std::string* attrName;
    if ((ownerId   = std::get_if<int>           (&vars.at(0)->getRef())) &&
        (nameList  = std::get_if<listObj>       (&vars.at(1)->getRef())) &&
        (scoreList = std::get_if<listObj>       (&vars.at(2)->getRef())) &&
        (attrName  = std::get_if<std::string>   (&vars.at(3)->getRef())))
    {
        return (std::shared_ptr<RunnableTask>) std::make_shared<ScoresTask>(playerHandler, *ownerId, *nameList, *scoreList, *attrName);
    }

    throw BadVariableArgException("Expected listObj, listObj, int, string");
    return nullptr;
}

std::shared_ptr<RunnableTask> DefaultFactory<AssignmentTask>::create(mutableVarPointerVector &vars) const
{
    if(!checkPreconditions(Min(2), Max(2), "Expected two Variable arguments", vars, envMgr))
    { return nullptr; }

    std::string* name;
    if ((name   = std::get_if<std::string>           (&vars.at(0)->getRef())))
    {
        return (std::shared_ptr<RunnableTask>) std::make_shared<AssignmentTask>(envMgr, *name, vars.at(1));
    }

    throw BadVariableArgException("Expected string, Variable");
    return nullptr;
}

// std::shared_ptr<RunnableTask> DefaultFactory<DiscardTask>::create(mutableVarPointerVector vars) const
// {
//     if(vars.size() != 2 ||
//         (vars.at(0))->type() != variables::Variable::Type::LIST ||
//         (vars.at(1))->type() != variables::Variable::Type::INT)
//     {
//         return nullptr;
//     }

//     std::shared_ptr<variables::listObj> origList = static_pointer_cast<variables::listObj>((vars.at(0))->getReference());

//     std::shared_ptr<int> numToDiscard = static_pointer_cast<int>((vars.at(1))->getReference());

//     return std::make_shared<DiscardTask>(static_pointer_cast<variables::listObj>(origList), *numToDiscard);

// }


// ===========================================converter===========================================
std::shared_ptr<RunnableTask> SCConverter::convert(std::shared_ptr<RuleNode> task)
{
    if(factories[task->getType()])
    {
        std::vector<std::vector<std::string>> neededArgs = task->getData();
        mutableVarPointerVector args = tempArgs[task->getType()];  // [TEMP]
        // [TODO]
        // for(std::string_view arg: neededArgs)
        // {
        //     args.push_back(src->getVar(arg));
        // }
        return factories[task->getType()]->create(args);
    }
    throw std::runtime_error("unsupported task type: cannot convert");
    return nullptr;
}
void SCConverter::addFactory(NodeType e, std::shared_ptr<TaskFactory> factory)
{
    factories[e] = std::move(factory);
}


SCConverter buildDefaultConverter(std::shared_ptr<GameInstance> game)
{
    if(game == nullptr || game->playerHandler == nullptr || game->envMgr == nullptr)
    {
        throw std::runtime_error("game, player handler or envMgr is null");
    }

    SCConverter converter(std::move(game));
    converter.addFactory(NodeType::REVERSE, std::make_shared<ReverseFactory>());
    converter.addFactory(NodeType::SHUFFLE, std::make_shared<ShuffleFactory>());
    converter.addFactory(NodeType::EXTEND, std::make_shared<ExtendFactory>());
    converter.addFactory(NodeType::INPUT_CHOICE, std::make_shared<InputFactory>(converter.src->playerHandler));
    converter.addFactory(NodeType::MESSAGE, std::make_shared<MessageFactory>(converter.src->playerHandler));
    converter.addFactory(NodeType::SCORES, std::make_shared<ScoresFactory>(converter.src->playerHandler));
    converter.addFactory(NodeType::ASSIGNMENT, std::make_shared<AssignmentFactory>(converter.src->envMgr));
    // converter.addFactory(Task::Type::DISCARD, std::make_shared<DiscardFactory>());
    return converter;
}
