#ifndef VARIABLES_H
#define VARIABLES_H
#include <string>
#include <iostream>
#include <map>
#include <variant>
#include <vector>
#include <iostream>
#include <memory>
#include <algorithm>
#include <typeindex>
#include <type_traits>
#include<random>
// dont print in release mode. define here so can be used in many classes
void debugPrint(const std::string_view &msg);


// Variable implementation
namespace var {
class Variable;
using mapType = std::map<std::string, std::shared_ptr<std::string>>;
using varMapType = std::map<std::string, std::shared_ptr<Variable>>;
// shared_ptr bc listObjs can be used to collect/group pre-existing variables to perform operations on
using listObj = std::vector<std::shared_ptr<Variable>>;
using varType = std::variant<
                int,
                std::string,
                bool,
                mapType,
                listObj,
                varMapType,
                std::shared_ptr<void>, // technically support all types, should be avoided where possible
                std::monostate>; // NULL
using varTypeBorrowedPtr = varType*;


enum Type
{
    INT, STRING, BOOL, MAP, LIST, VAR_MAP, POINTER, NONE // ensure order matches order in variant
};
class BadVariableArgException : public std::exception {
    public:
        BadVariableArgException() {}
        BadVariableArgException(const std::string &message):msg(message) {}
        char * what () { return (char*)msg.c_str(); }
    private:
        std::string msg = "Unsupported arguments";
};


namespace VariableUtils
{
    void setValue(varType &var, varType &val);
    void setValue(varType &var, varType &val, varType &val2); // mapType and listObj only
    void setValue(varType &var, varType &val, Variable &val2); // varMapType and listObj only

    void printValue(const varType &var, std::string_view delim="", std::ostream& stream=std::cout);
    Type getType(const varType &var);
    bool compare(const varType &var1, const varType &var2);

    std::string getWithKey(varType &var1, const varType &key); // only mapType
    std::shared_ptr<Variable> getVarWithKey(varType &var1, const varType &key); // only varMapType and listObj

    size_t size(const varType &var1);
};


class Variable // in class to make future implementation changes easier
{
    public:
        Variable(varType val):value(std::make_shared<varType>(val)) {}
        Variable();//temporary
        bool operator==(const Variable &var) const;
        bool isEqual(const varType &other) const;
        bool isEqual(Variable &other) const;

        void set(const varType &val);
        void set(const varType &val, const varType &val2); // mapType and listObj only
        void set(const varType &val, Variable &val2); // varMapType and listObj only

        varType get() const { return *value; }
        varType& getRef() const { return *value; }
        std::shared_ptr<varType> getPtr() const { return value; }
        varTypeBorrowedPtr getBorrowPtr() const { return value.get(); }
        std::string get(const varType &key); // mapType only

        size_t size() const;

        void print();
        Type type();
    protected:
        std::shared_ptr<varType> value;

};


std::shared_ptr<Variable> makeVarPtr(varType val);
template<class... Args>
std::shared_ptr<Variable> makeVarPtr(Args && ... args);
Variable makeVar(varType val);


// everything is passed by reference bc it's not guaranteed to be the supported types
// and we dont want to copy complex types
namespace ListObjUtils
{
    void push_back(varType &var, const varType &val);
    void remove(varType &var, const varType &val);

    void insert_at(varType &var, const varType &val, const varType &indx);
    void insert_at(varType &var, const Variable &val, const varType &indx);

    void remove_at(varType &var, const varType &val);
    std::shared_ptr<Variable> get_at(varType &var1, const varType &key);
    bool contains(varType &var1, const varType &key);
    listObj::iterator begin(varType &var1);
    listObj::iterator end(varType &var1);

    void shuffle(varType &var);
    void reverse(varType &var);
};


// helpful functions
template<class... Args>
static Variable makeVar(Args && ... args)
{
    if(sizeof...(args) == 1)
    {
        auto& first = [](auto& first, auto&...) -> auto& { return first; }(args...);
        return Variable(first);
    }

    varType list = listObj();
    for(const auto &item: {args...})
    {
        ListObjUtils::push_back(list, item);
    }
    return list;
}

template<class... Args>
std::shared_ptr<Variable> makeVarPtr(Args && ... args)
{
    //temporary
    if(sizeof...(args)==0)
        return std::make_shared<Variable>();
    if(sizeof...(args) == 1)
    {
        auto& first = [](auto& first, auto&...) -> auto& { return first; }(args...);
        return std::make_shared<Variable>(first);
    }
    varType list = listObj();
    for(const auto &item: {args...})
    {
        varType temp(item);
        ListObjUtils::push_back(list, temp);
    }
    return std::make_shared<Variable>(list);
}
};

#endif