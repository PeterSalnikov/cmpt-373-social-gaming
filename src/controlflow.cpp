#include "controlflow.hpp"
#include<variant>
using namespace var;

varType
getElements(listObj& list, Variable& key)
{
    varType elements = listObj();

    for(const auto& map : list)
    {
        std::string candidate = VariableUtils::getWithKey(map->getRef(),key.get());
        ListObjUtils::push_back(elements,candidate);
    }

    return elements;
}

/*
contains(): is an any_of operation which returns true if any element in a list satisfies a condition.

collect(): is a filtering operation which returns a filtered list of maps of an original list of maps
whose certain key satisfies a condition
*/

var::varType
collect(listObj& list, Variable& key, Variable& rhs)
{
    std::string* rhsStr;
    rhsStr = std::get_if<std::string>(&(rhs.getRef()));

    varType collected = listObj();
    
        for(const auto& map : list)
        {
            // this conditional would only work for rockpaper as == is hardcoded
            // there is the possibility that other games would use other comparators
            
            if(VariableUtils::getWithKey(map->getRef(),key.get()) == std::get<std::string>(rhs.get()))
            {
                ListObjUtils::push_back(collected, map);
            }
        }    
    return collected;
}

bool
contains(listObj& list, Variable& value)
{

    for(const auto& item : list)
    {
        if(item->getRef() == value.get())
            return true;
    }
    return false;
}

std::pair<int,int> upFrom(int upFrom, int to)
{
    return std::make_pair(upFrom, to);
}

bool evalAnd(bool& s1, bool& s2)
{
    return s1 && s2;
}

bool evalOr(bool& s1, bool& s2)
{
    return s1 || s2;
}

bool evalNot(bool& statement)
{
    return !statement;
}
