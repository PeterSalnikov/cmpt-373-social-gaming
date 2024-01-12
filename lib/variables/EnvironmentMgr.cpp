#include "EnvironmentMgr.hpp"


using namespace env_mgr;


// ===========================================Timer================================================
time_t getTime(unsigned int seconds)
{
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point end = now + std::chrono::seconds(seconds);
    return std::chrono::system_clock::to_time_t(end);
}

Timer::Timer() { }
Timer::Timer(const Timer &other):
    id(other.id),
    nextRuleNode(other.nextRuleNode),
    type(other.type),
    flagName(other.flagName),
    duration(other.duration)
    { }

Timer::Timer(int anId, const std::shared_ptr<RuleNode> &next, int aduration, Type aType, std::string aflagName):
    id(anId),
    nextRuleNode(next),
    type(aType),
    flagName(aflagName),
    duration(aduration)
    { }
bool Timer::hasFlag() const { return flagName != ""; }
void Timer::start() { expireTime = getTime(duration); }

bool Timer::isExpired() const
{
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    time_t now_t = std::chrono::system_clock::to_time_t(now);
    return difftime(expireTime, now_t) < 0;
}


// ================================================Scope===========================================
void Scope::setVariable(std::string_view name, const std::shared_ptr<Variable> &value)
{ variables[std::string(name)] = value; }

std::shared_ptr<Variable> Scope::getVariable(std::string_view name) const
{
    auto it = variables.find(std::string(name));
    return it == variables.end()? nullptr : it->second;
}

bool Scope::hasVar(std::string_view name) const { return variables.find(std::string(name)) != variables.end(); }
ControlFlow* Scope::getCtrlFlow() { return ctrlFlow.get(); }


// =========================================EnvironmentManager=====================================
EnvironmentManager::EnvironmentManager()
{ scopes.push_front(std::make_unique<Scope>(this)); }

void EnvironmentManager::enterScope(const std::shared_ptr<RuleNode> &node, const Timer &timer)
{
    auto newScope = std::make_unique<Scope>(node, this);
    newScope->timerId = timer.id;
    timers[timer.id] = std::make_unique<Timer>(timer);
    timers.at(timer.id)->scopeid = scopes.size();

    if(timer.hasFlag())
    {
        setVariable(timer.flagName, makeVar(false));
    }
    timers.at(timer.id)->start();

    scopes.emplace_front(std::move(newScope));
}

std::shared_ptr<RuleNode> EnvironmentManager::enterScope(const std::shared_ptr<RuleNode> &node)
{
    ControlFlow* currScopeCtrlFlow;
    if(!scopes.empty() &&
       (currScopeCtrlFlow = scopes.front()->getCtrlFlow()) &&
       currScopeCtrlFlow->getNode() == node)
    {
        return currScopeCtrlFlow->updateLoop(); // will return either loop child or loop sibling
    }

    auto newScope = std::make_unique<Scope>(node, this);
    auto next = (currScopeCtrlFlow = newScope->getCtrlFlow())? currScopeCtrlFlow->getNext() : nullptr;
    scopes.emplace_front(std::move(newScope)); // make sure is before updateLoop so loop variable is put in right scope

    if(!node)
    { return next; }

    auto nodeType = node->getType();
    return (nodeType == NodeType::FOR || nodeType == NodeType::PARALLEL)? currScopeCtrlFlow->updateLoop() : next;
}

std::shared_ptr<RuleNode> EnvironmentManager::exitScope(bool &shouldBlock, bool force)
{
    shouldBlock = false;
    if(scopes.empty()) // end of game
    { throw GameOverException(); return nullptr; }

    Scope* toDelete = scopes.front().get();
    if(!toDelete) // error: should never happen
    { return nullptr; }

    // check for unexpired pad timers
    if(timers.find(toDelete->timerId) != timers.end())
    {
        Timer* timer = timers.at(toDelete->timerId).get();
        if(timer->type == Timer::Type::PAD && !timer->isExpired())
        {
            shouldBlock = true;
        }
        else if(!timer->hasFlag())
        {
            timers.erase(toDelete->timerId);
        }
    }
    // check for waiting inputs
    else if(toDelete->waitingInputs())
    {
        shouldBlock = true;
    }

    // check if should exit loop
    auto deleteCtrlFlow = toDelete->getCtrlFlow();
    if(!deleteCtrlFlow)
    { return nullptr; }

    if(!force && !deleteCtrlFlow->exiting)
    {
        toDelete->variables.clear();
        return deleteCtrlFlow->getNode(); // goto for statement
    }

    auto next = deleteCtrlFlow->getNode()->getNextNode();
    scopes.pop_front(); // needs to be last so toDelete isnt deallocated
    return next; // goto statement after for
}

std::shared_ptr<RuleNode> EnvironmentManager::updateTimers()
{
    // move all expired timers to vector
    std::vector<std::unique_ptr<Timer>> expiredTimers;
    std::for_each(timers.begin(), timers.end(),
        [&expiredTimers](auto &timer) { expiredTimers.emplace_back(std::move(timer.second)); });
    std::erase_if(timers, [](auto &timer) { return timer.second == nullptr; });

    unsigned int lastExitScopeId = scopes.size();
    std::shared_ptr<RuleNode> nextRuleNode = nullptr;
    std::for_each(expiredTimers.begin(), expiredTimers.end(),
        [this, &lastExitScopeId, &nextRuleNode](const auto &timer)
        {
            if(timer->hasFlag())
            { setVariable(timer->flagName, true); }

            if(timer->type != Timer::Type::STOP)
            { return; }

            // exit scopes up to one with timer
            int scopeId = timer->scopeid; // need to be signed since -1 is valid
            while(scopeId < (int) scopes.size())
            {
                bool shouldBlock; // can ignore
                exitScope(shouldBlock, true);
            }

            if((int) lastExitScopeId > scopeId)
            {
                nextRuleNode = timer->nextRuleNode;
                lastExitScopeId = scopeId;
            }
        });
    return nextRuleNode;
}

Timer* EnvironmentManager::getTimer(int timerId) const
{
    auto it = timers.find(timerId);
    return it == timers.end()? nullptr : (it->second).get();
}

std::shared_ptr<Variable> EnvironmentManager::getVariable(std::string_view name) const
{
    auto scope = hasVar(name);
    return (scope != nullptr)? scope->getVariable(name) : nullptr;
}

Scope* EnvironmentManager::hasVar(std::string_view name) const
{
    auto scopeIt = std::find_if(scopes.begin(), scopes.end(),
        [&name](const auto &scope) { return scope->hasVar(name); });

    return (scopeIt == scopes.end())? nullptr : (*scopeIt).get();
}

int EnvironmentManager::depth() const { return scopes.size(); }
bool EnvironmentManager::inParallel() const { return scopes.front()->isParallel; }


// ==========================================ControlFlow===========================================
ControlFlow::ControlFlow(const std::shared_ptr<RuleNode> &anode, EnvironmentManager* amgr):
    node(std::shared_ptr<RuleNode>(anode)), mgr(amgr)
{
    if(!node)
    { return; }

    std::vector<std::vector<std::string>> data = node->getData();
    if(data.size() < 2) // loops need minimum of variable name + range
    { return; }

    std::vector<std::string> element = data.at(1);
    std::vector<std::string> list = data.at(2);

    loopVarName = element.at(0);
    auto var = mgr->getVariable(list.at(1));
    if(std::get_if<listObj>(&var->getRef()))
    {
        range = var->getPtr();
    }
}

std::shared_ptr<RuleNode> ControlFlow::getNext()
{
    if(!node)
    { return nullptr; }

    auto nodeType = node->getType();
    if(nodeType == NodeType::FOR || nodeType == NodeType::PARALLEL)
    {
        auto childNode = node->getChildWithKey({"true"});
        return childNode.child;
    }
    else // [TODO] everything thats not a for or parallel
    {
        return nullptr;
    }
}

std::shared_ptr<RuleNode> ControlFlow::getNode() const { return node; }

std::shared_ptr<RuleNode> ControlFlow::updateLoop()
{
    if(VariableUtils::size(*range) != 0)
    {
        std::shared_ptr<Variable> newVar = ListObjUtils::get_at(*range, varType(0));
        mgr->setVariable(loopVarName, newVar->getRef());
        ListObjUtils::remove_at(*range, varType(0));

        exiting = VariableUtils::size(*range) == 0;
    }
    return getNext();
}

varType EnvironmentManager::parseValue(std::string_view expression, Scope* scope){
    if (expression == "true")
    {
        return true;
    } else if (expression == "false")
    {
        return false;
    } else if(expression[0] == '\"'){
        
        
        auto first = expression.find("{");
        auto second = expression.find("}");
        auto variable = expression.substr(first, second - first);
       
        if(variable.size() != 0){
            auto stringVariable = scope->getVariable(variable).get()->get();

            std::string stringVar = std::get<std::string>(stringVariable);

            std::string firstPart = (std::string)expression.substr(0, expression.find("{"));
            std::string secondPart = (std::string)expression.substr(second, expression.length() - second);
            return firstPart + stringVar + secondPart;
        }

        return (std::string)expression;
    } else if (expression[0] >= 48 && expression[0] <= 57) { // ascii values for integers
        try
        {
            return std::stoi((std::string)expression);
        }
        catch(const std::exception& e)
        {
            return (std::string)expression;
        } 
    }
    return (std::string)expression;
}

varType EnvironmentManager::evaluateOperator(
    std::vector<std::string>::iterator& expression, std::vector<std::string>::iterator& end, 
    Scope* scope, const std::map<std::string, operatorTypes>::iterator operatorIt){
    switch(operatorIt->second)
    {
        case EQUALS:
            expression += 1;
            return evaluationExpression(expression, end, scope) == evaluationExpression(expression, end, scope);
            break;
        case NEGATION:
            expression += 1;
            return !std::get<bool>(evaluationExpression(expression, end, scope));
            break;
        case ADDITION:
            expression += 1;
            return std::get<int>(evaluationExpression(expression, end, scope)) + std::get<int>(evaluationExpression(expression, end, scope));
            break;
        case OR:
            expression += 1;
            return std::get<bool>(evaluationExpression(expression, end, scope)) || std::get<bool>(evaluationExpression(expression, end, scope));
            break;
    }
    return 0; // should never get here
}

varType EnvironmentManager::evaluateBuildIn(
    std::vector<std::string>::iterator& expression, std::vector<std::string>::iterator& end, 
    Scope* scope, const std::map<std::string, builtinTypes>::iterator it){
    switch (it->second)
    {
        case SIZETYPE:
        {
            varType size((int)scope->getVariable(*expression)->size());
            expression += 2;
            return size;
            break;
        }
        case CONTAINS:
        {
            //varType contains(var::ListObjUtils::contains(scope->getVariable(*expression), scope->getVariable(*(expression + 2))));
            expression += 3;
            return true;//contains;
            break;
        }
        case COLLECT:
        {
            //auto collect = var::ListObjUtils::collect(scope->getVariable(*expression), scope->getVariable(*(expression + 2)), scope->getVariable(*(expression + 3)) == scope->getVariable(*(expression + 4)));
            expression += 5;
            return false;
            break;
        }
        case UPFROM:
        {
            //auto upfrom = var::ListObjUtils::upfrom(scope->getVariable(*expression), scope->getVariable(*(expression + 2)));
            expression += 3;
            return false;
            break;
        }
    }
    return false; // should never get here
}

varType EnvironmentManager::evaluationExpression(std::vector<std::string>::iterator& expression, std::vector<std::string>::iterator& end, Scope* scope){
    // expression should never be at end, so only check if next item is end, if so, skip checking for multi part expresion
    if(expression + 1 != end){
        const auto it = buildtinMap.find(*(expression + 1));
        if(it != buildtinMap.end()){
            return evaluateBuildIn(expression, end, scope, it);
        } 
            
        const auto operatorIt = operatorMap.find(*expression);
        if(operatorIt != operatorMap.end()){
            return evaluateOperator(expression, end, scope, operatorIt);
        }
    }
    
    if(hasVar(*expression)->hasVar(*expression)){
        auto var = scope->getVariable(*expression);
        expression++;
        return (*var.get()).get();
    } else{
        auto var = parseValue(*expression, scope);
        expression++;
        return var;
    }
    
}
