#pragma once

#include "gameinstance.h"
#include <string_view>
#include <queue>
#include <unordered_map>
#include <memory>

class GameInstanceManager {

private:
    std::queue<std::shared_ptr<GameInstance>> activeGameQueue;
    std::unordered_map<int, std::shared_ptr<GameInstance>> waitingGameMap;

public:
    GameInstanceManager() = default;

    void createGameInstance(const std::string_view& name, const int& gameInstanceId);

    std::shared_ptr<GameInstance> getGameInstanceFromQueue();
    void putGameInstanceInQueue(std::shared_ptr<GameInstance> game);

    std::shared_ptr<GameInstance> getGameInstanceFromMap(const int& gameInstanceId);
    void putGameInstanceInMap(std::shared_ptr<GameInstance> game);

    void assignPlayerToGame(const int& gameInstanceId, const std::string_view& playerName, const int& playerId);
    void deletePlayerFromGame(const int& gameInstanceId, const int& playerId); // ignore for now, implement if time
};
