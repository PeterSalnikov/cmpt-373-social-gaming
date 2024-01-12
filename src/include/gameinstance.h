#pragma once
#include <iostream>
#include <vector>
#include <string_view>
#include <unordered_map>
#include <memory>
#include "SocialGamingTaskFactory.hpp"
#include "EnvironmentMgr.hpp"


class SCConverter;
class GameInstance;
class PlayerHandler;

class GameInstance
{
    public:
        GameInstance(std::string_view gameInstanceName, int gameInstanceId);
        std::string_view getGameInstanceName(); // remove later
        int getGameInstanceId();

        void addPlayerToGame(int playerId, std::string_view username);
        void removePlayerFromGame(const int& playerId); // [TODO]

        std::shared_ptr<taskFactory::RunnableTask> getTask();

        void setConverter(SCConverter &aConverter);
        std::shared_ptr<taskFactory::RunnableTask> convertTask(const std::shared_ptr<RuleNode> &node);

        using msgType = std::map<std::string, std::string>;
        struct Msg
        {
            Msg(std::string_view ids, const msgType &msg);
            std::string playerIds;
            msgType message;
            bool operator==(const Msg& other) const = default;
        };
        std::shared_ptr<env_mgr::EnvironmentManager> envMgr = std::make_shared<env_mgr::EnvironmentManager>();
        std::shared_ptr<PlayerHandler> playerHandler = std::make_shared<PlayerHandler>();

    private:
        std::string gameInstanceName;
        int gameInstanceId;
        size_t numMaxPlayers = 6;
        size_t currPlayerCount = 0;

        std::shared_ptr<taskFactory::RunnableTask> currTask;
        std::shared_ptr<SCConverter> converter;

        std::map<std::string, varType> playerVars; // [TODO] init from tree
};

class PlayerHandler
{
    public:
        void queueMessage(std::string_view ids, const GameInstance::msgType &msg);
        std::vector<GameInstance::Msg> getAllMsgs();
        void clearAllMessages();
        bool operator==(const PlayerHandler& other) { return false; } // [TODO] ignore for now, do if time
    private:
        std::vector<GameInstance::Msg> messages;
};
