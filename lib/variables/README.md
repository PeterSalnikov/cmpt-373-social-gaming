# Variables.hpp (namespace var)
this library provides a generic type (Variable) and functionalities
- [Variable](#variable)
    - [Aliases](#aliases)
    - [Available Types](#available-types)
    - [Basic Usage](#basic-usage)
        - [making Variables](#making-variables)
        - [accessing underlying varType](#accessing-underlying-vartype)
        - [comparing](#comparing)
        - [other](#other)
- [VariableUtils](#variableutils)
    - [Available functions](#available-functions)
    - [Basic Functions Usage](#basic-functions-usage)
    - [Adding Behaviors](#adding-behaviors)
- [ListObjUtils](#listobjutils)
    - [Available Functions](#available-functions-1)
    - [Basic Functions Usage](#basic-functions-usage-1)
    - [Iterting listObj](#iterating-listobj)
    - [Adding Behaviors](#adding-behaviors-1)
- [troubleshooting](#troubleshooting)

## Variable
wrapper around `varType` (std::variant with the [following types](#available-types))
### Aliases
| alias | underlying type
|:--|:--
|`varType`              | `variant`
|`mapType`              | `map<string, shared_ptr<string>`
|`varMapType`              | `map<string, shared_ptr<Variable>>`
|`listObj`              | `vector<std::shared_ptr<Variable>>`
|`varTypeBorrowedPtr`   | `varType*`
### Available Types
| `var::Type` | underlying primitive
| :---        | :---
| `INT`       | int
| `STRING`    | string or const char*
| `BOOL`      | bool
| `MAP`       | mapType
| `LIST`      | listObj
| `VAR_MAP`   | varMapType
| `POINTER`   | shared_ptr\<void>
| `NONE`      | std::monostate         (essentially NULL)

### Basic Usage
#### making Variables
|code| result type
| :---   | :---
| Variable v = `makeVar(1);` | INT
| Variable v = `makeVar(listObj());` | LIST
| Variable v = `makeVar(1, 2, 3);` | LIST
| mapType m; <br> m["hello"] = std::make_shared\<std::string>("world"); <br> Variable v = <code>makeVar(m);</code> |MAP
||
|std::shared_ptr<Variable> v = `makeVarPtr(true);` | BOOL

#### accessing underlying varType
|code| return type
| :---   | :---
| `getRef()`| varType`&`
| `get()` | `varType` (copy)
| `getPtr()` | `std::shared_ptr`\<varType>
| `get(const varType&)` | `std::string `get with key (currently for maps only)
| `getBorrowPtr()` | varTypeBorrowedPtr (`varType*`)


#### comparing
| code
| :-
| `Variable == Variable`
`Variable.isEqual(Variable)`
`Variable.isEqual(varType&)`

#### other
| code | |
| :- | :- |
| `size()` | number of items for `listObj`, sizeof(varType) for others
| `print()` | output values to std::cout
| `type() ` | return [Type](#available-types)

## VariableUtils
behaviors for all varType
### available functions
| return | name | args | description
|:-|:-|:-|:-|
|       |`setValue`     |(varType &var, varType &val)                       | `var = val`
|       |`setValue`     |(varType &var, varType &val, Variable &val2);      | `var[va] = val2` (for varMap and listObj only)
|       |`setValue`     |(varType &var, varType &val, varType &val2)        | `var[val] = val2` (for mapType and listObj only)
||||
|Type   |`getType`      |(const varType &var) |
|std::shared_ptr<Variable> |`getVarWithKey`|(varType &var1, varType &key);  | `var1[key]` (for varMap and listObj only)
|std::string |`getWithKey`|(varType &var1, const varType &key);             | `var1[key]` (only for mapType)
||||
|       |`printValue`   |(const varType &var, std::string_view delim="")    | `std::cout << var << delim;`
|bool   |`compare`      |(const varType&, const varType&)                   | `true/false` if equal
|size_t |`size`         |(const varType &var1);                             | `sizeof()` or number of items in vector for `listObj`, `mapType`, `varMapType`

### basic functions usage
``` cpp
var::VariableUtils::funcName(var.getRef(), ...); // Variable
var::VariableUtils::funcName(1, ...); // primitive
```
### adding behaviors
1. add declaration in `Variables.hpp` to namespace `VariableUtils`
2. define in `Variables.cpp`. Overload for specific parameter combinations
``` cpp
void VariableUtils::funcName(const varType &var1, const varType &var2)
{
    // if only checking and dont need underlying type, can use holds_alternative
    if (mapType* val = std::get_if<mapType>(&var)) // note val is a POINTER
    { ...  }
    // to have multiple checks in a single statement:
    if ((std::get_if<listObj>(&vars.at(0)->getRef())) == nullptr ||
        (prompt = std::get_if<std::string>(&vars.at(1)->getRef())) == nullptr ||
        (type = std::get_if<std::string>(&vars.at(2)->getRef())) == nullptr)
    { ... }
    // if don't know type/don't care, use std::visit (see below)
}
```
<font color="red">**WARNING:**</font> you can do the same with std::visit but it is *less efficient + will drastically slow compile time*
``` cpp
void VariableUtils::funcName(const varType &var1, const varType &var2)
{
    return std::visit(overload{ // omit return if no return value
        [](const int &a, const int &b) { ... },
        [](const listObj &a, const auto &b) { ... }, // auto if don't care about type
        [](auto, auto) { return false; } // behavior for unsupported
    }, var1, var2); // add parameters
}
```

## ListObjUtils
bahaviors for listObj only
### available functions
| return | name | args | description
|-|--|------------|-----------------
| |`push_back`|(varType &vec, const varType &val) | add val to end of vec
| |`remove`|(varType &vec, const varType &val)| remove val number of items from front of vec
| |`insert_at`|(varType &vec, const varType &val, const varType &indx);| insert val at index indx. Throws `std::out_of_range` error
| |`insert_at`|(varType &var, const Variable &val, const varType &indx) | insert Variable at index
| |`remove_at`|(varType &vec, const varType &index)| removes value at index. Throws `std::out_of_range` error
std::shared_ptr\<Variable>| `get_at`|(varType &vec, const varType &index)| returns shared pointer to Variable at index
bool |`contains`|(varType &vec, const varType &key)| returns true if key present in vec
### basic functions usage
see [VariableUtils](#basic-functions-usage)
### iterating listObj
``` cpp
std::for_each(ListObjUtils::begin(list->getRef()), ListObjUtils::end(list->getRef()),
    [](auto &item) // is shared_ptr<Variable>
    {
        ...
    });
```
``` cpp
for(unsigned int i=0; i<list->size(); i++)
{
    std::shared_ptr<Variable> item = ListObjUtils::get_at(list->getRef(), (int) i);
    ...
}
```
### adding behaviors
see [VariableUtils](#adding-behaviors) and use namespace ListObjUtils instead


## troubleshooting
| error | solution
|:-|:-
<code>error: static assertion failed: std::visit requires the visitor to have the same return type for all alternatives of a variant</code>| make sure the same type is being returned in all std::visit lambdas. (even if you throw an error, something needs to be returned if the function isn't void) If you're returning nullptr and a pointer, move the return value outside std::visit like: <pre>std::shared_ptr\<Variable> temp; <br>std::visit(overload{<br>\[&temp](...)<br>{<br>...<br>temp = std::make_shared<Variable>(vec.at(tempIndex));<br>},<br>[&temp](auto, auto) { temp = nullptr; }<br>}, var1, key);<br>return temp;</pre>
<code>error no matching function for call to ‘visit...</code>| make sure all lambdas (including the last auto) have the same number of arguments