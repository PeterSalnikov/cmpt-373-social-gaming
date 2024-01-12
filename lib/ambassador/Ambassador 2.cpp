#include "Ambassador.h"

using namespace ambassador;
//==========================================message queue==========================================
msgQImpl::msgQImpl(std::string_view aName, bool writeVal, bool readVal):
    // mq(boost::interprocess::open_or_create, aName.data(), MSG_MAX_COUNT, MSG_MAX_SIZE),
    name(aName), writeBool(writeVal), readBool(readVal) { }

std::string_view msgQImpl::read()
{
    if(!readBool)
        return "";

    // read one msg from queue
    char newBuf[MSG_MAX_SIZE];
    unsigned int priority;
    // boost::interprocess::message_queue::size_type recvd_size;
    // try_receive so doesnt block if queue empty
    // if(!mq.try_receive(newBuf, MSG_MAX_SIZE, recvd_size, priority))
        return "";

    // return std::string_view(newBuf, newBuf + recvd_size);
}

int msgQImpl::write(std::string_view input)
{
    if(!writeBool)
        return -1;

    // set up send struct
    char newBuf[input.size()];
    strcpy(newBuf, input.data());

    // try_send so doesnt block if queue busy
    // if(!mq.try_send(newBuf, sizeof(newBuf), 0))
        return -1;

    return 0;
}

msgQImpl::~msgQImpl()
{
    // clean up queue
    // [TODO:] not called on program crash -> make cleanup daemon to make sure deleted?
    // boost::interprocess::message_queue::remove(name.data());
}


//===========================================Response============================================
Response::Response(std::string_view src)
{
    try
    {
        // get response obj from string
        json j = json::parse(src);
        Response resObj = j.template get<Response>();
        // copy data
        resType = resObj.getType();
        attrs = resObj.getAllAttrs();
    }
    catch(const json::exception &e) { std::cout << e.what() << std::endl;}
}

std::string Response::toString() const
{
    // convert to json string
    json j = *this;
    std::string val = j.dump();
    return val;
}

void Response::setType(const msgType &aType)
{
    resType = aType;
}
msgType Response::getType()
{
    return resType;
}

void Response::setAttr(std::string_view aKey, std::string_view aVal)
{
    attrs[aKey.data()] = aVal.data();
}
std::map<std::string, std::string> Response::getAllAttrs()
{
    return attrs;
}
std::string Response::getAttr(std::string_view aKey)
{
    if(attrs.count(aKey.data()))
    {
        return attrs[aKey.data()];
    }
    return "";
}


//===========================================ambassador============================================
Ambassador::Ambassador(const std::shared_ptr<msgQ> &writeQName, const std::shared_ptr<msgQ> &readQName):
    writeQ(writeQName), readQ(readQName) { }

int Ambassador::sendMsg(const Response &input) const
{
    return writeQ->write(input.toString());
}

std::queue<std::shared_ptr<Response>> Ambassador::getAllMsg()
{
    try
    {
        std::string content;
        while((content = readQ->read()) != "")
        {
            // add onto queue
            std::shared_ptr<Response> newRes = std::make_shared<Response>(content);
            readContentQ.push(newRes);
        }
    }
    catch(const QueueClosedError &exception)
    {
        // if queue closed, empty queue and put error message in queue
        // handling this is responsibility of user
        readContentQ = std::queue<std::shared_ptr<Response>>();
        std::shared_ptr<Response> newRes = std::make_shared<Response>();
        newRes->setType(msgType::QUEUE_CLOSED);
        readContentQ.push(newRes);
    }
    return readContentQ;
}

std::shared_ptr<Response> Ambassador::getOneMsg()
{
    // get all so queue doesnt fill up
    getAllMsg();
    if(readContentQ.empty())
    {
        std::shared_ptr<Response> res = std::make_shared<Response>();
        return res;
    }
    // return only first msg
    std::shared_ptr<Response> res = readContentQ.front();
    readContentQ.pop();
    return res;
}
