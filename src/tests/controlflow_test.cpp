// #include "../SocialGamingTaskFactory.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../lib/variables/Variables.hpp"
#include "controlflow.hpp"

using namespace var;

TEST(GetElementsTest,testSingle)
{
    mapType map1;
    map1["name"] = std::make_shared<std::string>("John");
    map1["two"] = std::make_shared<std::string>("2");

    listObj list{makeVarPtr(map1)};
    Variable key = makeVar("name");

    listObj gt{makeVarPtr("John")};

    varType elements = getElements(list,key);
    ASSERT_EQ(1,VariableUtils::size(elements));
    ASSERT_TRUE(ListObjUtils::contains(elements,"John"));
}

TEST(GetElementsTest,testMultiple)
{
    Variable key = makeVar("score");
    mapType map1;
    map1["name"] = std::make_shared<std::string>("John");
    map1["score"] = std::make_shared<std::string>("0");

    mapType map2;
    map2["name"] = std::make_shared<std::string>("Paul");
    map2["score"] = std::make_shared<std::string>("2");

    mapType map3;
    map3["name"] = std::make_shared<std::string>("Ringo");
    map3["score"] = std::make_shared<std::string>("1");

    mapType map4;
    map4["name"] = std::make_shared<std::string>("George");
    map4["score"] = std::make_shared<std::string>("0");

    listObj list{makeVarPtr(map1),makeVarPtr(map2),makeVarPtr(map3),makeVarPtr(map4)};

    varType elements = getElements(list,key);
    varType gt = makeVarPtr("John","Paul","Ringo","George");

    ASSERT_EQ(4,VariableUtils::size(elements));
    ASSERT_TRUE(ListObjUtils::contains(elements,"2"));
}

// empty list can be returned
// NOTE: if there is only one key and the 'search' key is not the same as the only key in the map, the program will seg fault.
// In the interest of time, I will be 'deferring' this issue
TEST(CollectTest, testEmpty)
{
    Variable key = makeVar("zero");
    mapType map;
    map["zero"] = std::make_shared<std::string>("0");
    map["two"] = std::make_shared<std::string>("2");
    listObj list{makeVarPtr(map)};
    Variable rhs = makeVar("1");
    varType collected = collect(list,key,rhs);

    ASSERT_EQ(VariableUtils::getType(collected),Type::LIST);
    ASSERT_EQ(0,VariableUtils::size(collected));
}
// should return a list the same as original
TEST(CollectTest, testSingle)
{
    Variable key = makeVar("zero");
    mapType map;
    map["zero"] = std::make_shared<std::string>("0");
    listObj list{makeVarPtr(map)};
    Variable rhs = makeVar("0");

    varType collected = collect(list,key,rhs);

    ASSERT_EQ(VariableUtils::getType(collected), Type::LIST);
    ASSERT_EQ(1, VariableUtils::size(collected));
    ASSERT_TRUE(VariableUtils::compare(list,collected));
}
// test the push_back of multiple maps and that all contents are present
TEST(CollectTest, testMultiple)
{
    Variable key = makeVar("score");
    mapType map1;
    map1["name"] = std::make_shared<std::string>("John");
    map1["score"] = std::make_shared<std::string>("0");

    mapType map2;
    map2["name"] = std::make_shared<std::string>("Paul");
    map2["score"] = std::make_shared<std::string>("2");

    mapType map3;
    map3["name"] = std::make_shared<std::string>("Ringo");
    map3["score"] = std::make_shared<std::string>("1");

    mapType map4;
    map4["name"] = std::make_shared<std::string>("George");
    map4["score"] = std::make_shared<std::string>("0");

    listObj list{makeVarPtr(map1),makeVarPtr(map2),makeVarPtr(map3),makeVarPtr(map4)};

    Variable rhs = makeVar("0");

    varType collected = collect(list,key,rhs);

    // listObj res = std::get<listObj>(collected);

    ASSERT_EQ(VariableUtils::getType(collected), Type::LIST);
    ASSERT_EQ(2, VariableUtils::size(collected));

}
TEST(ContainsTest,testEmpty)
{
    Variable dummy = makeVar("Dummy");
    listObj list{};

    bool has = contains(list,dummy);

    ASSERT_EQ(0,VariableUtils::size(list));
    ASSERT_FALSE(has);
}
// receives a list like it could get from getElements to check if contains
TEST(ContainsTest,testTrue)
{
    Variable ringo = makeVar("Ringo");
    listObj list{makeVarPtr("John"),makeVarPtr("Paul"),makeVarPtr("Ringo"),makeVarPtr("George")};

    bool has = contains(list,ringo);

    ASSERT_EQ(4,VariableUtils::size(list));
    ASSERT_TRUE(has);
}

TEST(ContainsTest,testFalse)
{
    Variable dummy = makeVar("Dimmy");

    listObj list{makeVarPtr("Dummy")};

    bool has = contains(list,dummy);
    ASSERT_EQ(1, VariableUtils::size(list));
    ASSERT_FALSE(has);
}