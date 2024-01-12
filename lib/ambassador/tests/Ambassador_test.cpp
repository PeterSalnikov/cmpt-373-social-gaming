#include "Ambassador.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
using namespace ambassador;
// mocks
class mockMsgQ : public msgQ
{
    public:
        MOCK_METHOD(int, write, (std::string_view input), (override));
        MOCK_METHOD(std::string, read, (), (override));
};


// msgQImpl -> idk how to test as its a message queue between processes

// Response
// init empty and from string
TEST(ResponseTest, initTest)
{
    // default
    Response res;
    ASSERT_EQ(res.getType(), msgType::EMPTY);
    ASSERT_TRUE(res.getAllAttrs().empty());
    // copy
    Response res2("{\"resType\":1, \"attrs\":{\"test\":\"success\"}}");
    ASSERT_EQ(res2.getType(), msgType::INPUT_REQ);
    ASSERT_EQ(res2.getAttr("test"), "success");
}
// toString
TEST(ResponseTest, toStringTest)
{
    // normal
    std::string expected2 = "{\"attrs\":{\"test\":\"success\"},\"resType\":1}";
    Response res2(expected2);
    ASSERT_EQ(expected2, res2.toString());
    // empty
    std::string expected = "{\"attrs\":{},\"resType\":0}";
    Response res;
    ASSERT_EQ(res.toString(), expected);
}
// setType + getType
TEST(ResponseTest, typeTest)
{
    msgType expected = msgType::EMPTY;
    Response res;
    ASSERT_EQ(res.getType(), expected);

    expected = msgType::DISPLAY_MSG;
    res.setType(expected);
    ASSERT_EQ(res.getType(), expected);
}
// setAttr + getAttr + getAllAttrs
TEST(ResponseTest, attrTest)
{
    std::string testAttr = "test";
    std::string expected = "success";
    Response res;

    // check not present
    ASSERT_EQ(res.getAttr("test"), "");
    // check present
    res.setAttr(testAttr, expected);
    ASSERT_EQ(res.getAttr("test"), "success");
    // make sure added correct
    std::map<std::string, std::string> expectedMap;
    expectedMap[testAttr.data()] = expected.data();
    ASSERT_EQ(res.getAllAttrs(), expectedMap);
}


// Ambassador
// sendMsg
TEST(AmbassadorTest, writeTest)
{
    // setup
    std::shared_ptr<mockMsgQ> wQ = std::make_shared<mockMsgQ>();
    std::shared_ptr<mockMsgQ> rQ = std::make_shared<mockMsgQ>();
    Ambassador a(wQ, rQ);

    // expects
    testing::Sequence seq;
    std::string expected = "{\"attrs\":{\"test\":\"success\"},\"resType\":1}";
    EXPECT_CALL(*wQ.get(), write(expected))
        .Times(1)
        .InSequence(seq);
    std::string destructor = "{\"attrs\":{},\"resType\":-1}";
    EXPECT_CALL(*wQ.get(), write(destructor))
        .Times(1)
        .InSequence(seq);

    // execute
    Response res(expected);
    a.sendMsg(res);
}
// getAllMsg + getOneMsg
TEST(AmbassadorTest, readTest)
{
    // setup
    std::shared_ptr<mockMsgQ> wQ = std::make_shared<mockMsgQ>();
    std::shared_ptr<mockMsgQ> rQ = std::make_shared<mockMsgQ>();
    Ambassador a(wQ, rQ);

    // expects
    EXPECT_CALL(*rQ.get(), read()).
        WillOnce(testing::Return("{\"resType\":1, \"attrs\":{\"test\":\"0\"}}")).
        WillOnce(testing::Return("{\"resType\":1, \"attrs\":{\"test\":\"1\"}}")).
        WillOnce(testing::Return("{\"resType\":1, \"attrs\":{\"test\":\"2\"}}")).
        WillRepeatedly(testing::Return(""));

    // execute
    // get all messages
    for(int i=0; i<3; i++)
    {
        std::shared_ptr<Response> ret = a.getOneMsg();
        ASSERT_EQ(ret->getType(), msgType::INPUT_REQ);
        ASSERT_EQ(ret->getAttr("test"), std::to_string(i));
    }
    // check no more
    std::shared_ptr<Response> ret = a.getOneMsg();
    ASSERT_EQ(ret->getType(), msgType::EMPTY);
}
// getAllMsg + getOneMsg
TEST(AmbassadorTest, closeTest)
{
    // setup
    std::shared_ptr<mockMsgQ> wQ = std::make_shared<mockMsgQ>();
    std::shared_ptr<mockMsgQ> rQ = std::make_shared<mockMsgQ>();
    Ambassador a(wQ, rQ);

    // simulate throwing QueueClosedError
    EXPECT_CALL(*rQ.get(), read()).
        WillOnce(testing::Throw(QueueClosedError()));

    // check
    std::shared_ptr<Response> ret = a.getOneMsg();
    ASSERT_EQ(ret->getType(), msgType::QUEUE_CLOSED);
}
