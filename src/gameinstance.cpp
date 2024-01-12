#include "include/gameinstance.h"


// =======================================GameInstance=============================================
GameInstance::GameInstance(std::string_view gameInstanceName, int gameInstanceId) :
    gameInstanceName(gameInstanceName),
    gameInstanceId(gameInstanceId)
{}
std::string_view GameInstance::getGameInstanceName()
{ return gameInstanceName; }
void GameInstance::removePlayerFromGame(const int& playerId){
    // [TODO] ignore for now, do if time
}
std::shared_ptr<taskFactory::RunnableTask> GameInstance::getTask()
{ return currTask; }
int GameInstance::getGameInstanceId()
{ return gameInstanceId; }

void GameInstance::addPlayerToGame(int playerId, std::string_view username)
{
    if(currPlayerCount >= numMaxPlayers)
    { debugPrint("Failed to add player: maximum players reached.");  return; }

    // init list if needed
    auto playerList = envMgr->getVariable("player");
    if(playerList == nullptr)
    {
        listObj newListObj;
        envMgr->setVariable("player", Variable(newListObj));
        playerList = envMgr->getVariable("player");
    }

    // init new player
    varMapType newPlayer;
    newPlayer["id"] = makeVarPtr(playerId);
    newPlayer["name"] = makeVarPtr(std::string(username));

    std::for_each(playerVars.begin(), playerVars.end(),
    [&](const auto &pair)
    {
        newPlayer[pair.first] = std::make_shared<Variable>(pair.second);
    });

    ListObjUtils::push_back(playerList->getRef(), varType{newPlayer});
}

void GameInstance::setConverter(SCConverter &aConverter)
{
    converter = std::shared_ptr<SCConverter>(&aConverter);
}
std::shared_ptr<taskFactory::RunnableTask> GameInstance::convertTask(const std::shared_ptr<RuleNode> &node)
{
    // return converter->convert(node);
    return nullptr;
}


// =======================================PlayerHandler=============================================
GameInstance::Msg::Msg(std::string_view ids, const msgType &msg):
    playerIds(ids), message(msg) {}

void PlayerHandler::queueMessage(std::string_view ids, const GameInstance::msgType &msg)
{
    GameInstance::Msg newMessage(ids, msg);
    messages.push_back(newMessage);
}
std::vector<GameInstance::Msg> PlayerHandler::getAllMsgs()
{ return messages; }
void PlayerHandler::clearAllMessages()
{ messages.clear(); }
