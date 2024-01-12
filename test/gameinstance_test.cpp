#include <gtest/gtest.h>
#include "gameinstance.h"


TEST(gameinstance, playeraddtest)
{
    std::string name = "rock,paper, scissors";
    int id = 0;
    auto game = GameInstance(name, id);
    ASSERT_EQ(game.getGameInstanceName(), name);
    ASSERT_EQ(game.getGameInstanceId(), id);

    game.addPlayerToGame(1, "player 1");
    auto players = game.envMgr->getVariable("player");

    ASSERT_NE(players, nullptr);

    ASSERT_EQ(1, players->size());
    varType key = 0;
    auto player = ListObjUtils::get_at(players->getRef(), key);

    key = "name";
    auto playerName = VariableUtils::getVarWithKey(player->getRef(), key);
    key = "id";
    auto playerId = VariableUtils::getVarWithKey(player->getRef(), key);

    ASSERT_EQ(varType{1}, playerId->get());
}