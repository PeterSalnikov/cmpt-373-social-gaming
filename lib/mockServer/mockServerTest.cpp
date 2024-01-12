#include "Ambassador.h"
#include <unistd.h>
#include <sys/wait.h>


using namespace ambassador;

// ===================================== formatting functions =====================================
#define VAR_NAME_HELPER(name) #name
#define VAR_NAME(x) VAR_NAME_HELPER(x)
#define CHECK_STATE_STR(x) case(x):return VAR_NAME(x);
std::string printMsgType(msgType val)
{
    switch(val)
    {
        CHECK_STATE_STR(QUEUE_CLOSED);
        CHECK_STATE_STR(EMPTY);
        CHECK_STATE_STR(INPUT_REQ);
        CHECK_STATE_STR(INPUT_RES);
        CHECK_STATE_STR(CONFIG_REQ);
        CHECK_STATE_STR(CONFIG_RES);
        CHECK_STATE_STR(GAME_INIT);
        CHECK_STATE_STR(GAME_MADE);
        CHECK_STATE_STR(GAME_START);
        CHECK_STATE_STR(GAME_END);
        CHECK_STATE_STR(PLAYER_JOIN);
        CHECK_STATE_STR(PLAYER_ACK);
        CHECK_STATE_STR(PLAYER_NACK);
        CHECK_STATE_STR(DISPLAY_MSG);
        CHECK_STATE_STR(DISPLAY_SCORES);
        default:
        return "Invalid";
    }
}
void format(std::string val1, std::string val2, int width)
{
    std::cout << std::left;
    std::cout << std::setw(width) << std::setfill('-') << val1;
    std::cout << std::right << std::setw(3) << val2;
    std::cout << std::endl;
}
void printMsgTypes()
{
    std::cout << std::left << "msgType: int vlaue" << std::endl;
    std::cout << std::string(25, '=') << std::endl;
    for(int i=msgType::QUEUE_CLOSED; i != msgType::DISPLAY_SCORES; i++)
    {
        format(printMsgType(static_cast<msgType>(i)), std::to_string(i), 20);
    }
}

int createResponse(Response &newRes)
{
    // get type
    std::string input = "";
    int type = -1;
    do
    {
        std::cout << std::endl << "Enter type (int 0-13) (end to quit program): ";
        getline(std::cin, input);
        if(input == "end") { return 0; }
        try{
            type = std::stoi(input);
        } catch(std::invalid_argument &ignore) {}
    } while(type > 13 || type < 0);

    newRes.setType(static_cast<msgType>(type));

    // get attributes
    while(true)
    {
        std::cout << "Enter attribute (\"end\" to quit): ";
        getline(std::cin, input);
        if(input == "end") { break; }

        std::string input2 = "";
        std::cout << "Enter value: " << std::endl;
        getline(std::cin, input2);

        newRes.setAttr(input, input2);
    }

    return 1;
}


/**
 * @brief TEMPORARY -> for prototyping/testing purposes; will be removed in future
 *
 * run both ./bin/mockServer and ./bin/mockAnswer in separate terminals
 * mockAnswer will mirror back whatever mockServer sends
 */
int main()
{
    // open queuees
    std::shared_ptr<msgQImpl> writeQ = std::make_shared<msgQImpl>(SERVER_QUEUE, true, false);
    std::shared_ptr<msgQImpl> readQ = std::make_shared<msgQImpl>(EVENT_LOOP_QUEUE, false, true);
    // setup ambassador
    Ambassador loopAmbassador(writeQ, readQ);

    printMsgTypes();
    while(true)
    {
        // send to other process
        Response newRes;
        if (createResponse(newRes) == 0)
        { break; }

        loopAmbassador.sendMsg(newRes);
        std::cout << "1 sent: " << newRes.toString() << std::endl;

        // check if anything from mirror
        std::shared_ptr<Response> res = loopAmbassador.getOneMsg();
        while(res->getType() != msgType::EMPTY)
        {
            std::cout << "1 got: " << res->toString() << std::endl;
            res = loopAmbassador.getOneMsg();
        }
        // sleep(1);
    }
    return 0;
}