#include "Variables.hpp"
#include <sstream>
#include <algorithm>
#include <random>

using namespace var;
template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
template<class... Ts> overload(Ts...) -> overload<Ts...>;


void debugPrint(const std::string_view &msg)
{
    #ifdef _DEBUG
        std::cout << msg << std::endl;
    #endif
}

// variable class
bool Variable::operator==(const Variable &var) const { return var.isEqual(*value); }
void Variable::set(const varType &val)
{
    varType temp(val);
    VariableUtils::setValue(*value, temp);
}
void Variable::set(const varType &val, const varType &val2)
{
    varType temp(val);
    varType temp2(val2);
    VariableUtils::setValue(*value, temp, temp2);
}
void Variable::set(const varType &val, Variable &val2)
{
    varType temp(val);
    VariableUtils::setValue(*value, temp, val2);
}
std::string Variable::get(const varType &key)
{
    varType temp(key);
    return VariableUtils::getWithKey(*value, temp);
}
void Variable::print() { VariableUtils::printValue(*value); }
Type Variable::type() { return VariableUtils::getType(*value); }
bool Variable::isEqual(const varType &other) const { return VariableUtils::compare(*value, other); }
bool Variable::isEqual(Variable &other) const { return isEqual(other.get()); }
size_t Variable::size() const { return VariableUtils::size(*value); }


// variable functions
void VariableUtils::setValue(varType &var, varType &val)
{
    if(var.index() != val.index())
    {
        throw BadVariableArgException();
        return;
    }
    var = val;
}
void VariableUtils::setValue(varType &var, varType &val, Variable &val2)
{
    varMapType* map;
    listObj* list;
    std::string* key;
    int* index;
    if ((map = std::get_if<varMapType>(&var)) != nullptr &&
        (key = std::get_if<std::string>(&val)) != nullptr)
    {
        (*map)[*key] = std::make_shared<Variable>(val2);
        return;
    }
    else if ((list = std::get_if<listObj>(&var)) != nullptr &&
        (index = std::get_if<int>(&val)) != nullptr)
    {
        list->at(*index) = std::shared_ptr<Variable>(&val2);
        return;
    }
    throw BadVariableArgException("Expected mapType, string, string");
}

void VariableUtils::setValue(varType &var, varType &val, varType &val2)
{
    listObj* list;
    int* index;
    mapType* orig;
    std::string* key;
    std::string* value;
    if ((orig = std::get_if<mapType>(&var)) != nullptr &&
        (key = std::get_if<std::string>(&val)) != nullptr &&
        (value = std::get_if<std::string>(&val2)) != nullptr)
    {
        (*orig)[*key] = std::make_shared<std::string>(*value);
        return;
    }
    else if ((list = std::get_if<listObj>(&var)) != nullptr &&
            (index = std::get_if<int>(&val)) != nullptr)
    {
        list->at(*index) = std::make_shared<Variable>(val2);
        return;
    }
    throw BadVariableArgException("Expected mapType, string, string");
}

void VariableUtils::printValue(const varType &var, std::string_view delim, std::ostream& stream)
{
    if (auto val = std::get_if<mapType>(&var))
    {
        std::for_each(val->begin(), val->end(), [&stream](const auto &item)
        {
            stream << "[" << item.first << "] = " << item.second;
        });
    }
    else if (auto val = std::get_if<listObj>(&var))
    {
        std::for_each(val->begin(), val->end(), [&stream](const auto &item)
        {
            VariableUtils::printValue(item->get(), ", ", stream);
        });
    }
    else if (auto val = std::get_if<int>(&var) ) { stream << *val; }
    else if (auto val = std::get_if<std::string>(&var) ) { stream << *val; }
    else if (auto val = std::get_if<bool>(&var) ) { stream << *val; }

    stream << delim;
}

Type VariableUtils::getType(const varType &var)
{
    return (Type)var.index();
}

bool VariableUtils::compare(const varType &var1, const varType &var2)
{
    if(var1.index() != var2.index())
    {
        throw BadVariableArgException("Expected arguments t be same type");
        return false;
    }

    // handle lists separately
    if(auto list1 = std::get_if<listObj>(&var1))
    {
        if(auto list2 = std::get_if<listObj>(&var1))
        {
            return std::equal(list1->begin(), list1->end(), list2->begin(),
                [](const auto&item1, const auto& item2)
                {
                    return item1->isEqual(item2->getRef());
                });
        }
    }
    return var1 == var2;
}
std::string VariableUtils::getWithKey(varType &var1, const varType &key)
{
    if (auto a = std::get_if<mapType>(&var1))
    {
        if(auto b = std::get_if<std::string>(&key))
        {
            // return *(a->at(*b));
            auto it = *(a->find(*b));
            if(it != *(a->end()))
            {
                return *it.second;
            }
            else
            {
                return "";
            }
        }
    }
    throw BadVariableArgException("Expected mapType, string");
    return "";
}
std::shared_ptr<Variable> VariableUtils::getVarWithKey(varType &var1, const varType &key)
{
    varMapType* map;
    const std::string* mapKey;
    const int* index;
    listObj* list;
    if((map = std::get_if<varMapType>(&var1)) != nullptr &&
        (mapKey = std::get_if<std::string>(&key)) != nullptr)
    {
        // return map->at(*mapKey);
        auto it = map->find(*mapKey);
        if(it != map->end())
        {
            return it->second;
        }
        else
        {
            return nullptr;
        }
    }
    else if((list = std::get_if<listObj>(&var1)) != nullptr &&
        (index = std::get_if<int>(&key)) != nullptr)
    {
        return ListObjUtils::get_at(var1, key);
    }

    throw BadVariableArgException("Expected mapType, string or listObj, int");
    return nullptr;
}

size_t VariableUtils::size(const varType &var1)
{
    if (auto a = std::get_if<listObj>(&var1))
    {
        return a->size();
    }
    else if (auto a = std::get_if<mapType>(&var1))
    {
        return a->size();
    }
    else if (auto a = std::get_if<varMapType>(&var1))
    {
        return a->size();
    }
    return sizeof(var1);
}


// list functions

void ListObjUtils::shuffle(varType &var)
{
    std::random_device rd;
    std::mt19937 g(rd());
    if (auto list = std::get_if<listObj>(&var))
    {
        std::shuffle(list->begin(), list->end(), std::default_random_engine());
        return;
    }
    throw BadVariableArgException("Expected listObj as first arg");
}

void ListObjUtils::reverse(varType &var)
{
    if (auto list = std::get_if<listObj>(&var))
    {
        std::reverse(list->begin(), list->end());
        return;
    }
    throw BadVariableArgException("Expected listObj as first arg");
}


void ListObjUtils::push_back(varType &var, const varType &val)
{
    if (auto list = std::get_if<listObj>(&var))
    {
        list->push_back(std::make_shared<Variable>(val));
        return;
    }
    throw BadVariableArgException("Expected listObj as first arg");
}


void ListObjUtils::remove(varType &var, const varType &val)
{
    if (auto vec = std::get_if<listObj>(&var))
    {
        if(auto numToDelete = std::get_if<int>(&val))
        {
            if(*numToDelete < 0) { return; }
            unsigned uNumToDelete = *numToDelete; // silence warnings
            if(uNumToDelete <= vec->size())
            {
                vec->erase(vec->begin(),vec->begin() + *numToDelete);
            }
        }
    }
    throw BadVariableArgException("expected listObj, int");
}

void insert_helper(listObj &vec, const Variable &var, const int &index)
{
    int tempIndex = index;
    if(tempIndex < 0)
    {
        tempIndex = vec.size() + tempIndex;
    }
    if(abs(tempIndex) > vec.size())
    {
        std::ostringstream stringStream;
        stringStream << "max index " << vec.size() << ": " << index << " out of range";
        throw std::out_of_range (stringStream.str());
    }
    unsigned int uIndex = tempIndex; // silence warnings
    auto it = uIndex == vec.size()? vec.end() : vec.begin() + uIndex;
    vec.insert(it, std::make_shared<Variable>(var));
}
void ListObjUtils::insert_at(varType &var, const varType &val, const varType &indx)
{
    if (auto vec = std::get_if<listObj>(&var))
    {
        if (auto index = std::get_if<int>(&indx))
        {
            insert_helper(*vec, val, *index);
        }
    }
}
void ListObjUtils::insert_at(varType &var, const Variable &val, const varType &indx)
{
    if (auto vec = std::get_if<listObj>(&var))
    {
        if (auto index = std::get_if<int>(&indx))
        {
            insert_helper(*vec, val, *index);
        }
    }
}

void ListObjUtils::remove_at(varType &var, const varType &val)
{
    auto helper = [](listObj &vec, const int &index)
    {
        int tempIndex = index;
        if(abs(tempIndex) > vec.size()) { return; }
        if(tempIndex < 0) { tempIndex = vec.size() + tempIndex; }

        unsigned int uIndex = tempIndex; // silence warnings
        auto it = uIndex >= vec.size()? vec.end(): vec.begin() + uIndex;
        vec.erase(it);
    };

    if (auto vec = std::get_if<listObj>(&var))
    {
        if (auto index = std::get_if<int>(&val))
        {
            helper(*vec, *index);
        }
    }
}

std::shared_ptr<Variable> ListObjUtils::get_at(varType &var1, const varType &key)
{
    std::shared_ptr<Variable> temp;
    auto helper = [&temp](listObj &vec, const int &index)
    {
        int tempIndex = index;
        if(abs(tempIndex) > vec.size()) { return; }
        if(tempIndex < 0) { tempIndex = vec.size() + tempIndex; }

        temp = vec.at(tempIndex);
    };

    if (auto vec = std::get_if<listObj>(&var1))
    {
        if (auto index = std::get_if<int>(&key))
        {
            helper(*vec, *index);
            return temp;
        }
    }

    throw BadVariableArgException("Expected listObj, int");
    return temp;
}

bool ListObjUtils::contains(varType &var1, const varType &key)
{
    auto helper = [](listObj &vec, const auto &var)
    {
        varType temp(var);
        return std::find_if(vec.begin(), vec.end(),
        [&temp](const auto &item)
        {
            return item->isEqual(temp);
        }) != vec.end();
    };
    if (auto vec = std::get_if<listObj>(&var1))
    {
        if (auto index = std::get_if<int>(&key))
        {
            return helper(*vec, *index);
        }
        else if (auto index = std::get_if<std::string>(&key))
        {
            return helper(*vec, *index);
        }
    }
    throw BadVariableArgException("Expected listObj as first arg");
    return false;
}
listObj::iterator ListObjUtils::begin(varType &var1)
{
    if (auto vec = std::get_if<listObj>(&var1))
    {
        return vec->begin();
    }
    throw BadVariableArgException("Expected listObj");
    return listObj::iterator{};
}
listObj::iterator ListObjUtils::end(varType &var1)
{
    if (auto vec = std::get_if<listObj>(&var1))
    {
        return vec->end();
    }
    return listObj::iterator{};
}