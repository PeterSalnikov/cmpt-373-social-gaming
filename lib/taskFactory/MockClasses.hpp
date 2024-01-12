#ifndef MOCK_H
#define MOCK_H
class SCConverter;

namespace mock{
// [TEMP]
class GameInstance: public std::enable_shared_from_this<GameInstance>
{
    public:
        void setConverter(SCConverter &aConverter)
        {
            converter = std::shared_ptr<SCConverter>(&aConverter);
        }
        using msgType = std::map<std::string, std::string>;
        struct Player {
            public:
            Player(): name("def") {}
            Player(std::string n): name(n) {}
            std::string name;
            std::vector<msgType> messages = {};
        };
        class PlayerHandler {
            public:
                PlayerHandler()
                {
                    playerMap[0] = Player("owner");
                    playerMap[1] = Player("one");
                    playerMap[2] = Player("two");
                    playerMap[3] = Player("three");
                }
                void queueMessage(int id, msgType &msg)
                {
                    playerMap[id].messages.push_back(msg);
                }
                std::map<int, std::vector<msgType>> getAllMsgs()
                {
                    std::map<int, std::vector<msgType>> ret;
                    std::for_each(playerMap.begin(), playerMap.end(),
                        [&ret](const auto &kv)
                        {
                            if(kv.second.messages.empty())
                            { return; }
                            ret[kv.first] = kv.second.messages;
                        });
                    return ret;
                }
                void clearAllMessages()
                {
                    std::for_each(playerMap.begin(), playerMap.end(),
                        [](auto &kv)
                        {
                            kv.second.messages.clear();
                        });
                }
                bool operator==(const PlayerHandler& other) { return false; }
            private:
                std::map<int, Player> playerMap;
        };
        std::shared_ptr<PlayerHandler> playerHandler = std::make_shared<PlayerHandler>();

    private:
        std::shared_ptr<SCConverter> converter;
};
// [TEMP]
class Task {
public:
    enum Type
    {
        SERVER_MSG_WAIT= 0,
        INPUT,
        MESSAGE,
        SCORES,
        TIMER_INIT,
        EXTEND,
        REVERSE,
        SHUFFLE,
        SORT,
        DEAL,
        DISCARD
    };
    Type getType() const { return type; }
    std::vector<std::string_view> getArgs() const { return {"test"};}
    Type type;

protected:
  unsigned id;

};
};
#endif