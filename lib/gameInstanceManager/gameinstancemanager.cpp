#include "include/gameinstancemanager.h"

#include<map>
#include <iostream>
#include <memory>

void GameInstanceManager::createGameInstance(const std::string_view& name, const int& gameInstanceId){
    std::shared_ptr<GameInstance> newGame = std::make_shared<GameInstance>(name, gameInstanceId);
    putGameInstanceInMap(newGame);
}

std::shared_ptr<GameInstance> GameInstanceManager::getGameInstanceFromQueue() {

    std::shared_ptr<GameInstance> game;

    if (!activeGameQueue.empty()) {
        game = activeGameQueue.front();
        activeGameQueue.pop();
    } else {
        std::cout << "GameQueue has no active games!" << std::endl;
        // this will crash if we try to access members after this returns null
        game = nullptr;
    }

    return game;
}

void GameInstanceManager::putGameInstanceInQueue(std::shared_ptr<GameInstance> game) {
    activeGameQueue.push(game);
}


std::shared_ptr<GameInstance> GameInstanceManager::getGameInstanceFromMap(const int& gameInstanceId){
    auto it = waitingGameMap.find(gameInstanceId);

    std::shared_ptr<GameInstance> game;

    if(it == waitingGameMap.end()){
        std::cout << "A game with that gameID does not exist!" << std::endl;
        // this will crash if we try to access members after this returns null
        game = nullptr;
    } else {
        // game does exist
        game = it->second;
    }

    return game;
}

void GameInstanceManager::putGameInstanceInMap(std::shared_ptr<GameInstance> game) {
    waitingGameMap.emplace(game->getGameInstanceId(), game);
}

void GameInstanceManager::assignPlayerToGame(const int& gameInstanceId, const std::string_view& playerName, const int& playerId){

    auto game = getGameInstanceFromMap(gameInstanceId);

    if (game != nullptr){
        game->addPlayerToGame(playerId, playerName);
    }
    else{
        //**** CHANGE with a proper error thrown or exception ****//
        std::cout << "Game instance not found" << std::endl;
    }
}

void GameInstanceManager::deletePlayerFromGame(const int& gameInstanceId, const int& playerId){
    std::shared_ptr<GameInstance> game = getGameInstanceFromMap(gameInstanceId);

    if(game != nullptr){
        // game->removePlayerFromGame(playerId);
    }
    else{
        //**** CHANGE with a proper error thrown or exception ****//
        std::cout << "Game instance not found" << std::endl;
    }
}
