#include <iostream>
#include <any>
#include <map>
#include <vector>
#include <queue>
#include <regex>
#include <unordered_map>
#include <unistd.h>
#include "Ambassador.h"

// dont print in release mode
void debugPrint(const std::string_view &msg)
{
    #ifdef _DEBUG
        std::cout << msg << std::endl;
    #endif
}


enum TaskType
{
    CONTROL_FLOW,       // if, loops, etc
    EXECUTABLE,         // executable statement
    SERVER_MSG_WAIT,    // wait for server response
    SERVER_MSG_CONTINUE,// continue with execution
    SERVER_DISPLAY_MSG  // give message for server to display
};

// [TEMP]
class Task
{
    private:
        TaskType type;
    public:
        Task(TaskType a, int i): type(a), id(i) {};
        int run()
        {
            std::cout << "ran task " << id << std::endl;
            return 0;
        };
        int id;
        TaskType getType() {return type;}
};

// [TEMP]
struct Player
{
    std::any getVar(std::string key){return vars.at(key);}
    void setVar(std::string key, std::any val){vars[key] = val;}
    std::map<std::string, std::any> vars;
};

//[TEMP]
class GameInstance
{
    public:
        GameInstance(std::vector<Player> somePlayers, int gameInstanceID)
        {
            for(int i=0; i<10; i++){
                taskQ.push(std::make_shared<Task>(TaskType::EXECUTABLE, i));
            }
            for(int i=0; i<5; i++){
                taskQ.push(std::make_shared<Task>(TaskType::SERVER_MSG_CONTINUE, i+10));
            }
            taskQ.push(std::make_shared<Task>(TaskType::SERVER_MSG_WAIT, 21));
            taskQ.push(std::make_shared<Task>(TaskType::SERVER_DISPLAY_MSG, 22));
        }
        std::shared_ptr<Task> getTask()
        {
            if(taskQ.empty())
            {
                return nullptr;
            }
            std::shared_ptr<Task> tmp = taskQ.front();
            std::cout << "popping task" << tmp->id << std::endl;
            taskQ.pop();
            return tmp;
        }
        void setPlayerVar(int id, const std::string &key, const std::string &val)
        {
            // players.at(id).setVar(key, val);
        }
    private:
        std::shared_ptr<Task> currTask = nullptr;
        std::vector<Player> players;
        std::queue<std::shared_ptr<Task>> taskQ;
};

// [TEMP]
class GameInstanceManager
{
    public:
        GameInstanceManager(std::queue<GameInstance> q, std::unordered_map<int, GameInstance> m, int c) {}

        std::shared_ptr<GameInstance> getGameInstanceFromQueue() { return game;}
        void putGameInstanceInQueue(const std::shared_ptr<GameInstance> instance) { isInactive = false;}
        void putGameInstanceInMap(const std::shared_ptr<GameInstance> instance) { isInactive = true;}
        void createGameInstance(std::string_view name){ }

        void playerInput(int id, const std::string &key, const std::string &input)
        {
            inputCount++;
            game->setPlayerVar(id, key, input);
            if(neededInputCount <= inputCount)
            {
                isInactive = false;
                inputCount = 0;
            }
        }
        bool isInactive = false;
    private:
        std::shared_ptr<GameInstance> game = std::make_shared<GameInstance>(std::vector<Player>(), 1);
        int inputCount = 0;
        int neededInputCount = 5;
};

/**
std::shared_ptr<Response> handleMessageToServer(const Task &task, int instanceId)
{
    std::shared_ptr<Response> newRes = std::make_shared<Response>();
    newRes->setAttr("instanceId", std::to_string(instanceId));
    switch(task->getType())
    {
        case INPUT_REQ:
        {
            if(!(InputTask* iTask = dynamic_cast<InputTask*>(&task)))
                break;

            newRes->setType(msgType::INPUT_REQ);

            newRes->setAttr("playerIdsSize", "1"); // [TEMP]
            newRes->setAttr("playerIds", iTask->getTargetId()); // [TEMP]
            neRes->setAttr("type", iTask->getInputType());
            neRes->setAttr("prompt", iTask->getPrompt());
            neRes->setAttr("rangeStart", ""); // [TEMP]
            neRes->setAttr("rangeEnd", ""); // [TEMP]
            neRes->setAttr("options", iTask->getChoices());
            neRes->setAttr("timeout", iTask->getTimeout());
        }
        break;
        case DISPLAY_MSG:
        {
            if(!(DisplayTask* dTask = dynamic_cast<DisplayTask*>(&task)))
                break;
            newRes->setType(msgType::DISPLAY_MSG);

            newRes->setAttr("message", dTask->getMessage());
        }
        break;
    }
    return newRes;
}
**/


int main()
{
    // // setup message queues
    // std::shared_ptr<msgQImpl> writeQ = std::make_shared<msgQImpl>(EVENT_LOOP_QUEUE, true, false);
    // std::shared_ptr<msgQImpl> readQ = std::make_shared<msgQImpl>(SERVER_QUEUE, false, true);
    // // setup ambassador
    // Ambassador serverAmbassador(writeQ, readQ);
    // setup game manager
    std::queue<GameInstance> gameQ;
    std::unordered_map<int, GameInstance> gameUMap;
    GameInstanceManager manager(gameQ, gameUMap, 1);

    // configure game instance
    manager.createGameInstance("Rock, Paper, Scissors");

    int maxProcessCount = 1;
    bool exit = false;

    // loop forever, processing tasks and server messages
    while(!exit)
    {
        // pop manager queue
        std::shared_ptr<GameInstance> instance = manager.getGameInstanceFromQueue();

        // process task if not on inactive queue
        if(!manager.isInactive) // [TEMP]
        {
            bool dontPushActive = false;
            for(int i=0; i<maxProcessCount; i++)
            {
                // get task to run
                std::shared_ptr<Task> task = instance->getTask();

                // check if end of program
                if(task == nullptr)
                {
                    // [TODO:] server message
                    std::cout << "Game End" << std::endl; // [TEMP]
                    exit = true;
                    break;
                }
                // check if server message
                else if(task->getType() == TaskType::SERVER_MSG_WAIT ||
                        task->getType() == TaskType::SERVER_MSG_CONTINUE ||
                        task->getType() == TaskType::SERVER_DISPLAY_MSG)
                {
                    // [TODO:] set expected ressponse count in instance
                    // put on waiting queue if needed
                    if(task->getType() == TaskType::SERVER_MSG_WAIT)
                    {
                        manager.putGameInstanceInMap(instance);
                        dontPushActive = true;
                        manager.isInactive = true; // [TEMP]
                    }
                    std::string msg = "[prompt id]";
                    msg += task->id;
                    debugPrint(msg);

                    // std::shared_ptr<Response> newRes = handleMessageToServer(task);
                    // serverAmbassador.sendMsg(newRes);
                }
                // run
                else
                {
                    task->run();
                }

                // push onto active queue if needed
                if(dontPushActive)
                {
                    break;
                }
            } // for maxProcessCount

            if(!dontPushActive)
                manager.putGameInstanceInQueue(instance);
        } // if !manager.isInactive


        // check if server message
        // std::shared_ptr<Response> res = serverAmbassador.getOneMsg();
        // if(res->getType() == msgType::QUEUE_CLOSED) // queue closed, error state, exit
        // {
        //     break;
        // }
        // if(res->getType() != msgType::EMPTY)
        // {
        //     manager.playerInput(stoi(sm[1]), sm[2], sm[3]); // [TEMP]
        // }


        // [TEMP] get input through cmd line
        if(manager.isInactive && !exit)
        {
            std::string input = "";
            do
            {
                std::cout << "enter input:" << std::endl;
                getline(std::cin, input);
            }while(input == "");

            // [TODO:] change method called depending on message type
            // [TEMP] parse (should be done in server ambassador)
            std::regex reg("player:([0-9]+) key:(.*) value:(.*)");
            std::smatch sm;
            std::regex_search(input, sm, reg);
            if(sm.size() >= 4)
            {
                try
                {
                    // process input
                    manager.playerInput(stoi(sm[1]), sm[2], sm[3]);
                    debugPrint("good input");
                    continue;
                }
                catch(const std::exception& ignored) {}
            }
            std::cout << "malformed message" << std::endl;
        }
        // sleep(1);
    }
    return 0;
}
