#include "Ambassador.h"
#include <unistd.h>
using namespace ambassador;
/**
 * @brief TEMPORARY -> for prototyping/testing purposes; will be removed in future
 */
int main()
{
    std::cout << "starting answer process" << std::endl;

    // open queues
    std::shared_ptr<msgQImpl> writeQ = std::make_shared<msgQImpl>(EVENT_LOOP_QUEUE, true, false);
    std::shared_ptr<msgQImpl> readQ = std::make_shared<msgQImpl>(SERVER_QUEUE, false, true);
    // setup ambassador
    Ambassador loopAmbassador(writeQ, readQ);
    while(true)
    {
        // get messages from mockServer
        std::shared_ptr<Response> res = loopAmbassador.getOneMsg();
        if(res->getType() == msgType::QUEUE_CLOSED)
        {
            break;
        }
        else if(res->getType() != msgType::EMPTY)
        {
            std::cout << "2 got: " << res->toString() << std::endl;
            if(res->getAttr("val") == "end")
                break;

            // mirror back message
            // std::string ret = "returning " + res->getAttr("val");
            // Response newRes;
            // newRes.setType(res->getType());
            // newRes.setAttr("val", ret);

            loopAmbassador.sendMsg(*res);
            std::cout << "2 sent: " << res->toString() << std::endl;
        }
        sleep(1);
    }
    return 0;
}