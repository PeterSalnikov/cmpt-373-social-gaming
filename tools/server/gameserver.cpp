/////////////////////////////////////////////////////////////////////////////
//                         Single Threaded Networking
//
// This file is distributed under the MIT License. See the LICENSE file
// for details.
/////////////////////////////////////////////////////////////////////////////

// code base from Nick Sumner github repo: https://github.com/nsumner/web-socket-networking"
// code modified by Anh Vo

#include "Server.h"
#include "Ambassador.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <map>
#include <string_view>


using networking::Server;
using networking::Connection;
using networking::Message;
using networking::ActionClient;
using networking::ActionEventLoop;
using networking::MessageResult;
using ambassador::msgType;
using ambassador::Response;
using ambassador::Ambassador;

std::vector<Connection> clients;


void
onConnect(Connection c) {
    std::cout << "New connection found: " << c.id << "\n";
    clients.push_back(c);
}


void
onDisconnect(Connection c) {
    std::cout << "Connection lost: " << c.id << "\n";
    auto eraseBegin = std::remove(std::begin(clients), std::end(clients), c);
    clients.erase(eraseBegin, std::end(clients));
}




class QuitAction : public ActionClient {
public:
    MessageResult executeClientMsg(Server& server, const Message& message) {
        std::ostringstream result;
        bool quit = false;
        std::cout << "Quit...\n";
        server.disconnect(message.connection);
        return MessageResult{result.str(), quit};
    }
};

class ShutDownAction : public ActionClient {
public:
    MessageResult executeClientMsg(Server& server, const Message& message) {
        std::ostringstream result;
        std::cout << "Shutting down.\n";
        bool quit = true;
        return MessageResult{result.str(), quit};
    }
};

// TODO: Add more action classes for different messages (CONFIG_REQ, PLAYER_ACK ..) from eventloop
class InputRequestAction : public ActionEventLoop {
public:
    void executeEventLoopMsg(Server& server, ambassador::Ambassador& loopAmbassador, ambassador::Response res) {
        // Get input from client: instanceId, playerIdsSize, playerIds[playerIds], type, prompt, *rangeStart, *rangeEnd, *options, timeout
        // Reference: "Ambassador.h"
        std::cout << "Please enter information: ";
        std::string input;
        std::cin >> input;

        // Create a response and send back to the event loop
        Response newRes;
        newRes.setType(msgType::INPUT_RES);
        newRes.setAttr("val", input);
        loopAmbassador.sendMsg(newRes);
    }
};

class ConfigRequestAction : public ActionEventLoop {
public:
    void executeEventLoopMsg(Server& server, ambassador::Ambassador& loopAmbassador, ambassador::Response res) {
        // Get input from client: instanceId, fields
        // fields = list of (name, type, *rangeStart, *rangeEnd, *options, *filetype)
        // Reference: "Ambassador.h"
        std::cout << "Please enter configuration information: ";
        std::string input;
        std::cin >> input;

        // Create a response and send back to the event loop
        Response newRes;
        newRes.setType(msgType::CONFIG_RES);
        newRes.setAttr("val", input);
        loopAmbassador.sendMsg(newRes);
    }
};

class GameMadeAction : public ActionEventLoop {
public:
    void executeEventLoopMsg(Server& server, ambassador::Ambassador& loopAmbassador, ambassador::Response res) {
        // Receive game instanceId for server to  make/return a join code
        // Reference: "Ambassador.h"
        std::string_view attrKey("gameInstanceID");
        std::cout << "Game made! Game instanceId is " << res.getAttr(attrKey);

        // TODO: send it to the event loop when a player wants to join the game
        // store in the game instanceId map
    }
};

class PlayerAckAction : public ActionEventLoop {
public:
    void executeEventLoopMsg(Server& server, ambassador::Ambassador& loopAmbassador, ambassador::Response res) {
        // Receive player info: instanceId, playerIdsSize, playerIds[playerIds]
        // Reference: "Ambassador.h"
        auto attrs = res.getAllAttrs();
        for (auto& attr :  attrs){
            std::cout << "Player attr " << attr.first << ": " << attr.second << '\n';
        }
    }
};


MessageResult
processMessages(Server& server, const std::deque<Message>& incoming) {
    std::map<std::string, std::unique_ptr<ActionClient>> actions;
    actions["quit"] = std::make_unique<QuitAction>();
    actions["shutdown"] = std::make_unique<ShutDownAction>();

    std::ostringstream result;
    bool quit = false;

    for (const auto& message : incoming) {
        std::string msg = message.text;
        auto actionKey = actions.find(msg);
        if (actionKey != actions.end()) {
            auto messageResult = actions[msg]->executeClientMsg(server, message);
            return messageResult;
        } else {
            result << message.connection.id << "> " << message.text << "\n";
        }
    }
    return MessageResult{result.str(), quit};
}

void
processEventLoopMessages(Server& server, std::queue<std::shared_ptr<ambassador::Response>>& messages) {
    // auto eventLoopMessages = (server.getAmbassador())->getAllMsg();
    auto eventLoopMessages = messages;
    std::map<ambassador::msgType, std::unique_ptr<ActionEventLoop>> actions;
    actions[ambassador::msgType::INPUT_REQ] = std::make_unique<InputRequestAction>();
    actions[ambassador::msgType::CONFIG_RES] = std::make_unique<ConfigRequestAction>();
    actions[ambassador::msgType::CONFIG_RES] = std::make_unique<GameMadeAction>();
    actions[ambassador::msgType::CONFIG_RES] = std::make_unique<PlayerAckAction>();

    while (!eventLoopMessages.empty()) {
        auto eventLoopMessage = eventLoopMessages.front();
        ambassador::msgType msgType = (*eventLoopMessage).getType();
        std::cout << "event loop message: " << (*eventLoopMessage).toString() << '\n';

        auto actionKey = actions.find(msgType);
        if (actionKey != actions.end()) {
            actions[msgType]->executeEventLoopMsg(server, *(server.getAmbassador()), (*eventLoopMessage));
        } else {
            std::cout << "Invalid Event Loop Message Type.\n";
        }
        eventLoopMessages.pop();
    }
}


std::deque<Message>
buildOutgoing(const std::string& log) {
    std::deque<Message> outgoing;
    for (auto client : clients) {
        outgoing.push_back({client, log});
    }
    return outgoing;
}


std::string
getHTTPMessage(const char* htmlLocation) {
    if (access(htmlLocation, R_OK ) != -1) {
        std::ifstream infile{htmlLocation};
        return std::string{std::istreambuf_iterator<char>(infile),
            std::istreambuf_iterator<char>()};

    }
    std::cerr << "Unable to open HTML index file:\n"
        << htmlLocation << "\n";
    std::exit(-1);
}


int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage:\n" << argv[0] << " <port> <html response>\n"
                            << "e.g. " << argv[0] << " 4002 ./webgame.html\n";
        return 1;
    }

    const unsigned short port = std::stoi(argv[1]);
    Server server{port, getHTTPMessage(argv[2]), onConnect, onDisconnect};

    while (true) {
        bool errorWhileUpdating = false;
        try {
            server.update();
        } catch (std::exception& e) {
            std::cerr << "Exception from Server update:\n"
                << " " << e.what() << "\n\n";
            errorWhileUpdating = true;
        }

        const auto incoming = server.receive();
        const auto [log, shouldQuit] = processMessages(server, incoming);

        // [Temp] fake event loop messages
        auto ambassador = server.getAmbassador();
        auto eventLoopMessages = ambassador->getAllMsg();

        std::queue<std::shared_ptr<ambassador::Response>> fakeMessages;
        Response fakeResponseOne;
        fakeResponseOne.setType(msgType::CONFIG_REQ);
        fakeResponseOne.setAttr("val", "Config info: ");
        fakeMessages.push(std::make_shared<ambassador::Response>(fakeResponseOne));

        Response fakeResponsetwo;
        fakeResponsetwo.setType(msgType::INPUT_REQ);
        fakeResponsetwo.setAttr("val", "PlayerId");
        fakeMessages.push(std::make_shared<ambassador::Response>(fakeResponsetwo));

        // get messages from the event loop and process the messages
        processEventLoopMessages(server, fakeMessages);

        const auto outgoing = buildOutgoing(log);
        server.send(outgoing);

        if (errorWhileUpdating) {
            break;
        }
        sleep(1);
    }
    return 0;
}
