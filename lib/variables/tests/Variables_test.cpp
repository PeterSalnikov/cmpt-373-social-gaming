#include "Variables.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace var;

// Variables
// check integer
TEST(VariablesTest, intTest)
{
    // init
    std::shared_ptr<Variable> v = makeVarPtr(1);

    // asserts
    ASSERT_TRUE(v->isEqual(1));
    ASSERT_EQ(v->type(), Type::INT);

    v->set(200);

    ASSERT_TRUE(v->isEqual(200));
}
// check string
TEST(VariablesTest, stringTest)
{
    // init
    std::shared_ptr<Variable> v = makeVarPtr("zzz");

    // asserts
    ASSERT_TRUE(v->isEqual("zzz"));
    ASSERT_EQ(v->type(), Type::STRING);

    v->set("hello world");
    try
    {
        v->set(1);
        ASSERT_TRUE(false);
    }
    catch(BadVariableArgException &ex) { }

    ASSERT_TRUE(v->isEqual("hello world"));
}
// check map
TEST(VariablesTest, mapTest)
{
    // init
    mapType m;
    m["hello"] = std::make_shared<std::string>("world");
    // init
    std::shared_ptr<Variable> v = makeVarPtr(m);

    // asserts
    ASSERT_TRUE(v->isEqual(m));
    ASSERT_EQ(v->type(), Type::MAP);

    v->set("hello world", "test");
    try
    {
        v->set(1);
        ASSERT_TRUE(false);
    }
    catch(BadVariableArgException &ex) { }

    ASSERT_EQ(v->get("hello world"), "test");
}
// check bool
TEST(VariablesTest, boolTest)
{
    // init
    std::shared_ptr<Variable> v = makeVarPtr(true);

    // asserts
    ASSERT_TRUE(v->isEqual(true));
    ASSERT_EQ(v->type(), Type::BOOL);

    v->set(false);
    try
    {
        v->set(1);
        ASSERT_TRUE(false);
    }
    catch(BadVariableArgException &ex) { }

    ASSERT_TRUE(v->isEqual(false));
}
// check listObj
TEST(VariablesTest, listTest)
{
    // init
    std::shared_ptr<Variable> v = makeVarPtr(1, 2, 3, 4);
    // asserts
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->type(), Type::LIST);

    try
    {
        v->set(1);
        ASSERT_TRUE(false);
    }
    catch(BadVariableArgException &ex) { }

    listObj check{makeVarPtr(1), makeVarPtr(2), makeVarPtr(3), makeVarPtr(4)};
    ASSERT_TRUE(v->isEqual(check));

    varType index{2};
    ASSERT_EQ(Variable(3), *(VariableUtils::getVarWithKey(v->getRef(), index)));

    v->set(2, "test");
    ASSERT_EQ(Variable("test"), *(VariableUtils::getVarWithKey(v->getRef(), index)));
}
// check map
TEST(VariablesTest, varMapTest)
{
    // init
    varMapType m;
    m["hello"] = makeVarPtr("world");
    // init
    std::shared_ptr<Variable> v = makeVarPtr(m);

    // asserts
    ASSERT_TRUE(v->isEqual(m));
    ASSERT_EQ(v->type(), Type::VAR_MAP);

    Variable var = makeVar("test");
    v->set("hello world", var);
    try
    {
        v->set(1);
        ASSERT_TRUE(false);
    }
    catch(BadVariableArgException &ex) { }

    varType index{"hello world"};
    ASSERT_EQ(*(VariableUtils::getVarWithKey(v->getRef(), index)), var);
}
// check custom objs
TEST(VariablesTest, ptrTest)
{
    // init
    // class Test {
    //     public:
    //     Test() {}
    //     std::string val = "hello world";
    //     std::string test() { return val; }
    //     bool operator==(const Test& other) { return false;}
    // };

    // std::shared_ptr<Test> inst = std::make_shared<Test>();
    // std::shared_ptr<void> voidRef = static_pointer_cast<Test>(inst);

    // std::shared_ptr<Variable> item = makeVarPtr(voidRef);

    // std::shared_ptr<void> ref = std::get<std::shared_ptr<void>>(item->get());
    // std::shared_ptr<Test> testRef = static_pointer_cast<Test>(ref);

    // testRef->val = "test";

    // // assert
    // ASSERT_EQ(inst->val, "test");
    // ASSERT_EQ(inst, testRef);
}


// listObj
TEST(listObjTest, pushTest)
{
    // init
    std::shared_ptr<Variable> list1 = makeVarPtr(listObj());

    listObj list2{makeVarPtr(1), makeVarPtr(2),makeVarPtr(3)};
    ListObjUtils::push_back(list1->getRef(), 1);
    ListObjUtils::push_back(list1->getRef(), 2);
    ListObjUtils::push_back(list1->getRef(), 3);

    // asserts
    ASSERT_TRUE(list1->isEqual(list2));

    ASSERT_EQ(3, VariableUtils::size(list1->getRef()));
    ASSERT_TRUE(ListObjUtils::contains(list1->getRef(), 1));
}
// listObj
TEST(listObjTest, iterateTest)
{
    testing::internal::CaptureStdout();

    // init
    std::shared_ptr<Variable> list1 = makeVarPtr(1, 2, 3);

    // asserts
    std::for_each(ListObjUtils::begin(list1->getRef()),
        ListObjUtils::end(list1->getRef()),
        [](auto &item)
        {
            VariableUtils::printValue(item->getRef());
        });

    for(unsigned int i=0; i<list1->size(); i++)
    {
        std::shared_ptr<Variable> var = ListObjUtils::get_at(list1->getRef(), (int)i);
        VariableUtils::printValue(var->getRef());
    }

    std::string output = testing::internal::GetCapturedStdout();
    ASSERT_EQ("123123", output);
}
TEST(listObjTest, insertGetTest)
{
    // init
    listObj l;
    std::shared_ptr<Variable> item = makeVarPtr(1, 2, 3);

    // asserts
    try
    {
        ListObjUtils::insert_at(item->getRef(), 4, 5);
        ASSERT_TRUE(false);
    } catch(std::out_of_range &ex) {}

    ASSERT_EQ(3, item->size());
    ListObjUtils::insert_at(item->getRef(), 4, 3);
    ASSERT_EQ(4, item->size());
    ASSERT_TRUE(ListObjUtils::contains(item->getRef(), 4));
    ASSERT_TRUE(ListObjUtils::get_at(item->getRef(), 3)->isEqual(4));

    Variable var = makeVar(5);
    ListObjUtils::insert_at(item->getRef(), var, 0);
    ASSERT_EQ(5, item->size());
    ASSERT_TRUE(ListObjUtils::get_at(item->getRef(), 0)->isEqual(5));

    ASSERT_TRUE(ListObjUtils::get_at(item->getRef(), -1)->isEqual(4));
}