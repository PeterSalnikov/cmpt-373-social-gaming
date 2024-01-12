#ifndef AMBASSADOR
#define AMBASSADOR

#include <string>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <queue>
#include <iostream>
#include <string_view>
#include "../nlohmann/json.hpp"
#include <boost/interprocess/ipc/message_queue.hpp>

#define MSG_MAX_SIZE 600
#define MSG_MAX_COUNT 100

namespace ambassador {
// message queue names
#define EVENT_LOOP_QUEUE "loopQueue.txt"
#define SERVER_QUEUE "serverQueue.txt"

using json = nlohmann::json;


class QueueClosedError: public std::exception
{
    public:
    const char* what()
    {
        return "ERROR: queue closed";
    }
};

// * = optional fields, depends on type field
// (...) = list of attributes in Response obj
// [...] = indicates what process will make/send this response
// name[...] = name of array field with array content structure (tuple or value)
enum msgType
{
    QUEUE_CLOSED = -1,  // set by msgQImpl
    EMPTY = 0,          // empty -> should never be received, indicates error

    INPUT_REQ,          // [loop]   request input from client
                        //          (instanceId, playerIdsSize, playerIds[playerIds], type, prompt, *rangeStart, *rangeEnd, *options, timeout)
    INPUT_RES,          // [server] input from client
                        //          (instanceId, dataSize, data[(playerId, type, value)])

    CONFIG_REQ,         // [loop]   list of configurable fields to give to client
                        //          (instanceId, fields)
                        //          fields = list of (name, type, *rangeStart, *rangeEnd, *options, *filetype)
    CONFIG_RES,         // [server] client returns ^ with inputted values
                        //          (instanceId, fields[(name, value)])
                        //          fields = list of (name, value)

    GAME_INIT,          // [server] gives gameName for instance manager to make new instance
                        //          (instanceId, gameName)
    GAME_MADE,          // [loop]   gives game id so server can make/return a join code
                        //          (instanceId)
    GAME_START,         // [server] gives game id so manager can push instance to active queue
                        //          (instanceId)
    GAME_END,           // [loop]   tells clients game's done
                        //          (instanceId)

    PLAYER_JOIN,        // [server] gives playerid and game instance id so manager can add player
                        //          (instanceId, playerIdsSize, playerIds[playerIds])
    PLAYER_ACK,         // [loop]   player successfully joined
                        //          (instanceId, playerIdsSize, playerIds[playerIds])
    PLAYER_NACK,        // [loop]   player failed to join
                        //          (instanceId, playerIdsSize, playerIds[playerIds])
    // PLAYER_QUIT,        // gives playerid and game instance id so manager can remove player

    DISPLAY_MSG,         // [loop]   tells player message to display
                        //          (playerId, message)
    DISPLAY_SCORES     // [loop]   tells owner scores to display
                        //          (ownerId, attributeName, playerNames, scores)
};


struct msgQ // interface for easy implementation changing
{
    public:
        virtual std::string read() = 0;
        virtual int write(std::string_view input) = 0;
};
// makes, reads and writes to message queue
struct msgQImpl : public msgQ
{
    public:
        msgQImpl(std::string_view aName, bool writeVal, bool readVal);
        ~msgQImpl();

        std::string read();
        int write(std::string_view input);
    private:
        boost::interprocess::message_queue mq;
        std::string name;
        bool writeBool;
        bool readBool;
};


// container for messages from message queue
// [TODO:] convert to interface + subclasses?
class Response
{
    private:
        msgType resType;                            // message type
        std::map<std::string, std::string> attrs;   // attributes, changes according to type
    public:
        Response():resType(msgType::EMPTY) {}
        Response(std::string_view src);

        std::string toString() const;
        // setters
        void setType(const msgType &aType);
        void setAttr(std::string_view aKey, std::string_view aVal);
        // getters
        std::string getAttr(std::string_view aKey);
        msgType getType();
        std::map<std::string, std::string> getAllAttrs();

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Response, resType, attrs) // enable json de/serialization
};


// handles communicating with other processes
class Ambassador
{
    public:
        Ambassador(const std::shared_ptr<msgQ> &writeQName, const std::shared_ptr<msgQ> &readQName);
        ~Ambassador();
        int sendMsg(const Response &input) const;
        std::queue<std::shared_ptr<Response>> getAllMsg();
        std::shared_ptr<Response> getOneMsg();
    private:
        std::shared_ptr<msgQ> writeQ;
        std::shared_ptr<msgQ> readQ;
        std::queue<std::shared_ptr<Response>> readContentQ;
};
}
#endif