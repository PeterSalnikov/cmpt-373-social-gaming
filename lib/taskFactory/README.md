# Runnable Tasks
- [Defining a New Task](#defining-a-new-task)
    - [add RunnableTask class](#add-runnabletask-class)
    - [define run()](#define-run)
    - [add create function](#add-create-function)
    - [add to converter](#add-to-converter)
    - [troubleshooting](#troubleshooting)

- [Refactoring from previous Design](#refactoring-from-previous-design)
    - [add RunnableTask class](#add-runnabletask-class-1)
    - [define run()](#define-run-1)
    - [add create function](#add-create-function-1)
    - [add to converter](#add-to-converter-1)


## Defining a New Task
### add RunnableTask and Factory classes
[SocialGamingTaskFactory.hpp](SocialGamingTaskFactory.hpp)
- `copy` any parameters that `aren't stored in EnvironmentManager` (aka temporary) and prefer to save them as `primitives` (not varType or Variable)
- you can use `copyVec<T>(const listObj&)` to convert `listObj` -> `std::vector<T>`
- prefer to store `non-temporary variables` (aka vars stored in EnvironmentManager) as `varTypeBorrowedPtr` instead of shared_ptr
    - (make sure to `check if they're nullptr` before using them in `run()`)
``` cpp
class NewTask: public RunnableTask
{
    public:
        // player handler only for server related tasks
        NewTask(const playerHandlerPtr &aPlayerHandler,
                Variable &var1,
                std::shared_ptr<varType> var2):
            variable1(var1.getBorrowPtr()), // for getting varType* from Variable
            variable2(std::move(var2))
            {}
        void run() override;
        int getType() const override { return Task::Type::NEW; } // change to right type
    private:
        const playerHandlerPtr playerHandler;
        varTypeBorrowedPtr variable1;
        const std::shared_ptr<varType> variable2;
};

template<>
class DefaultFactory<NewTask>: public TaskFactory {
    public:
    DefaultFactory(playerHandlerPtr phandler):playerHandler(phandler) {} // only define if player handler needed
    // make sure this returns RunnableTask
    std::shared_ptr<RunnableTask> create(mutableVarPointerVector &vars) const override;
    private:
    const playerHandlerPtr playerHandler;
};
using NewFactory       = DefaultFactory<NewTask>;
```
### define run()
[SocialGamingTaskFactory.cpp](SocialGamingTaskFactory.cpp)
- if many class members need to be accessed (or you have unique_ptrs), replace `[var2 = this->var2]` with `[&]`
- if `var1` isn't a `varType` or `shared_ptr<varType>`, can just treat as a `vector`
    - `ListObjUtils::begin(*var1)` -> `var1.begin()`
- if `var2` isn't `varType` or `shared_ptr<varType` can omit `std::get<>`
    - `std::get<std::string>(*var2)` -> `var2`
- to omit try-catch, can use `std::get_if<>` instead of `std::get<>`
``` cpp
void NewTask::run()
{
    // to iterate varType(listObjs), normal foreach loops can't be used. See lib/variables/README.md for more information
    // on how to use varType
    std::for_each(ListObjUtils::begin(*var1), ListObjUtils::end(*var1),
    // include any needed class members
    [var2 = this->var2](const auto &playerId)
    {
        try // wrap any std::get<>() with a try-catch
        {
            // for this example, var1 = a list of playerIds, var2 = string
            // this code is using the playerHandler to send players whose id is in var1 the string in var2
            GameInstance::msgType msg;
            msg["type"] = "message";
            msg["data"] = std::get<std::string>(*var2);
            pHandler->queueMessage(std::get<int>(playerId.get()), msg);
        } catch(std::bad_variant_access &ex) {}
    });
}
```
### define create function
[SocialGamingTaskFactory.hpp](SocialGamingTaskFactory.cpp)
``` cpp
std::shared_ptr<RunnableTask> DefaultFactory<NewTask>::create(mutableVarPointerVector &vars) const
{
    // check preconditions: number of arguments (and player handler if needed)
    if(vars.size() < 2)
    {
        throw BadVariableArgException("Expected two Variable arguments");
        return nullptr;
    }
    if(playerHandler == nullptr)
    {
        throw BadVariableArgException("Expected playerHandler to be not null");
        return nullptr;
    }

    std::shared_ptr<RunnableTask> ret = nullptr;
    // check args are right type + get underlying type if needed
    if (std::get_if<listObj>(&vars.at(0)->getRef()))
    {
        if (auto newVal = std::get_if<std::string>(&vars.at(1)->getRef())) // get_if always returns a POINTER
        {
            // pass any variables stored in EnvMgr as pointers
            ret = std::make_shared<NewTask>(playerHandler, vars.at(0)->getPtr(), *newVal);
            // need varType&        ? vars.at(0)->getRef()
            // need varType         ? vars.at(0)->get()
            // need varType*        ? vars.at(0)->getBorrowedPtr()
            // need underlying type ? auto val = std::get_if<type>(&vars.at(0)->getRef())
            // need Variable&       ? *vars.at(0)
        }
    }
    else
    {
        // change error message
        throw BadVariableArgException("Expected two listObjs");
    }
    return ret;
}
```
### add to converter
[SocialGamingTaskFactory.cpp](SocialGamingTaskFactory.cpp)
``` cpp
SCConverter buildDefaultConverter(std::shared_ptr<GameInstance> game)
{
    ...
    // only put player handler if needed
    converter.addFactory(Task::Type::NEW, std::make_shared<NewFactory>(converter.src->playerHandler));
    ...
}
```
### troubleshooting
| issue | solution
|:-|:-
<code>warning: relocation against `_ZTVN13sgTaskFactory19MessageRunnableTaskE' in read-only section `.text.</code>| make sure all virtual functions are defined (especially run())
<code>==53164==ERROR: AddressSanitizer: attempting free on address which was not malloc()-ed: 0x6060000010b0 in thread T0</code>| make sure you're properly using shared_ptrs (and aren't creating multiple shared_ptrs for the same memory address)
<code>error: capture of non-variable ‘sgTaskFactory::MessageRunnableTask::pHandler’</code>|make sure class members are passed to lambdas as `pHandler = this->pHandler`


## Refactoring from previous Design
### add RunnableTask class
``` diff
class ExtendTask: public RunnableTask
{
    public:
-       ExtendTask(listSharedPtr anAddList, listSharedPtr someAddElems):
+       ExtendRunnableTask(std::shared_ptr<varType> anAddList, std::shared_ptr<varType> someAddElems):
            addList(std::move(anAddList)),
            addElems(std::move(someAddElems)) {}
        void run() override;
        int getType() const override { return Task::Type::EXTEND; }
    private:
-       listSharedPtr addList;
-       listSharedPtr addElems;
+       std::shared_ptr<varType> addList;
+       std::shared_ptr<varType> addElems;
};
```

### define run()
``` diff
void ExtendTask::run()
{
-   for(auto &elem: *addElems)
+   std::for_each(ListObjUtils::begin(*addElems), ListObjUtils::end(*addElems),
+   [addList = this->addList](const auto &item)
    {
-       addList->push_back(std::move(elem));
+       ListObjUtils::push_back(*addList, item.getRef());
-   }
    });
}
```
### add create function
``` diff
- std::shared_ptr<RunnableTask> DefaultFactory<ExtendTask>::create(mutableVarPointerVector vars) const
+ std::shared_ptr<RunnableTask> DefaultFactory<ExtendTask>::create(mutableVarPointerVector &vars) const
{
     // check preconditions (extend should have 2 list arguments)
-     if(vars.size() != 2 ||
-         (vars.at(0))->type() != variables::Variable::Type::LIST ||
-         (vars.at(1))->type() != variables::Variable::Type::LIST )
-     {
-         return nullptr;
-     }
+    if(vars.size() < 2)
+    {
+        throw BadVariableArgException("Expected two Variable arguments");
+        return nullptr;
+    }

-     std::shared_ptr<void> aList = (vars.at(0))->getReference();
-     std::shared_ptr<void> eList = (vars.at(1))->getReference();
-     std::shared_ptr<variables::listObj> eList2 = static_pointer_cast<variables::listObj>(eList);
-     std::shared_ptr<variables::Variable> item = variables::makeVar(1);
-     eList2->push_back(item);
+     std::shared_ptr<RunnableTask> ret = nullptr;
+     std::visit(overload{
+     [&vars, &ret](listObj &orig, listObj &newVal) // arg list matches expected
+     {
        // make RunnableTask object
+         ret = std::make_shared<ExtendTask>(vars.at(0)->getPtr(), vars.at(1)->getPtr());
-     return std::make_shared<ExtendTask>(static_pointer_cast<variables::listObj>(aList), eList2);

+     },
+     [&vars, &ret](auto, auto) { throw BadVariableArgException("Expected two listObjs"); }
+    }, vars.at(0)->getRef(), vars.at(1)->getRef());
+    return ret;
}
```

