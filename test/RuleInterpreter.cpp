#include <cpp-tree-sitter.h>
#include <gtest/gtest.h>
#include <string_view>

#include "RuleInterpreter.h"

extern "C" {
  TSLanguage* tree_sitter_socialgaming();
}


class RuleInterpreterTest : public ::testing::Test {
protected: 

	ts::Language language = tree_sitter_socialgaming();
	ts::Parser parser { language };
	// ts::Tree tree = NULL;
	std::string_view source =  R"""(configuration {
		name: "Rock, Paper, Scissors"
		player range: (2, 4)
		audience: false
		setup: {
		rounds {
			kind: integer
			prompt: "The number of rounds to play"
			range: (1, 20)
		}
		}
	}
	constants {} variables {} per-player{} per-audience {}

	rules {
		for round in configuration.rounds.upfrom(1) {

		discard winners.size() from winners;

		message all "Round {round}. Choose your weapon!";

		parallel for player in players {
			input choice to player {
			prompt: "{player.name}, choose your weapon!"
			choices: weapons.name
			target: player.name
			timeout: 10
			}
		}

		for weapon in weapons {
			match !players.elements.weapon.contains(weapon.name) {
			true => {
				extend winners with players.elements.collect(player, player.weapon = weapon.beats);
			}
			}
		}

		match true {
			winners.size() = players.size() || winners.size() = 0 => {
			message all "Tie game!";
			}
			true => {
			message all "Winners: {winners.elements.name}";
			for winner in winners {
				winner.wins <- winner.wins + 1;
			}
			}
		}
		}

		// Report the final scores!
		scores ["wins"];
	}

	)""";

	ts::Tree tree = parser.parseString(source);
	void SetUp() override {

	}
};

TEST_F(RuleInterpreterTest, getNodeValue) {

	ts::Node root = tree.getRootNode();
	ts::Node configuration = root.getChildByFieldName("configuration");
	ts::Node name = configuration.getChildByFieldName("name");

	// only because the test strings are escaped
	std::string expectedName = "\"Rock, Paper, Scissors\"";

	EXPECT_EQ(source, getNodeValue(source, root));

	EXPECT_EQ(expectedName, getNodeValue(source, configuration.getChildByFieldName("name")));
	EXPECT_EQ(expectedName, getNodeValue(source, name));
}


TEST_F(RuleInterpreterTest, getChildNodeValue) {

	ts::Node root = tree.getRootNode();

	// only because the test strings are escaped
	std::string expectedName = "\"Rock, Paper, Scissors\"";

	EXPECT_EQ(expectedName, getChildNodeValue(source, "name", root.getChildByFieldName("configuration")));

}

TEST_F(RuleInterpreterTest, getAllChildNodeValues) {

	ts::Node root = tree.getRootNode();

	auto result = getAllChildNodeValues(source, "message", root);

	std::vector<std::string_view> expected = {
	"message all \"Round {round}. Choose your weapon!\";", 
	"message all \"Tie game!\";", 
	"message all \"Winners: {winners.elements.name}\";"
	};

	EXPECT_EQ(expected, getAllChildNodeValues(source, "message", root.getChildByFieldName("rules")));

}
