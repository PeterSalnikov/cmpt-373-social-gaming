#ifndef CONTROL_FLOW_HPP
#define CONTROL_FLOW_HPP
#include "../../lib/variables/Variables.hpp"

enum LogicalOperator
{
    AND, OR, NOT
};

var::varType 
getElements(var::listObj& list, var::Variable& key);

var::varType
collect(var::listObj& list, var::Variable& key, var::Variable& rhs);

bool
contains(var::listObj& list, var::Variable& value);

std::pair<int,int> 
upFrom(int upFrom, int to);

bool evalAnd(bool& s1, bool& s2);

bool evalOr(bool& s1, bool& s2);

bool evalNot(bool& statement);
#endif