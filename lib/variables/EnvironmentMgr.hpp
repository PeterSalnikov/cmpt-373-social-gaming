#ifndef ENV_MGR_H
#define ENV_MGR_H
#include "Variables.hpp"
#include "RuleInterpreter.h"
#include <deque>
#include <chrono>
#include <time.h>
#include <string>


using namespace var;


namespace env_mgr
{
    class GameOverException : public std::exception
    {
        public:
        char * what () { return (char*) "End of Game Reached"; }
    };

    struct Timer
    {
        public:
            enum Type { PAD, STOP, REGUALR };

            Timer();
            Timer(const Timer &other);
            Timer(int anId, const std::shared_ptr<RuleNode> &next, int aduration,
                  Type aType=Type::REGUALR, std::string aflagName="");
            ~Timer() = default;

            bool hasFlag() const;
            bool isExpired() const;

            void start();

            const int id = -1;
            const std::shared_ptr<RuleNode> nextRuleNode;
            int scopeid = 0;
            const Type type = Type::REGUALR;
            std::string flagName = "";

        private:
            unsigned int duration = 0;
            time_t expireTime;
    };

    class EnvironmentManager;
    class ControlFlow
    {
        public:
            ControlFlow(const std::shared_ptr<RuleNode> &anode, EnvironmentManager* amgr);
            bool operator==(const ControlFlow &other) { return other.getNode() == node; }
            ~ControlFlow() = default;
            ControlFlow(const ControlFlow& other) = delete;

            std::shared_ptr<RuleNode> getNext();
            std::shared_ptr<RuleNode> getNode() const;
            std::shared_ptr<RuleNode> updateLoop(); // update loop variable. returns next node to goto

            bool exiting = true;

        private:
            const std::shared_ptr<RuleNode> node;
            std::string loopVarName;
            std::shared_ptr<varType> range;
            EnvironmentManager* mgr;
    };
    class Scope
    {
        public:
            Scope(EnvironmentManager* mgr) { }
            Scope(const std::shared_ptr<RuleNode> &node, EnvironmentManager* mgr):
                isParallel(node? node->getType() == NodeType::PARALLEL : false),
                ctrlFlow(std::make_unique<ControlFlow>(node, mgr)) { }
            Scope(const Scope& other) = delete;
            ~Scope() = default;

            template <typename T>
            void setVariable(std::string_view name, const T &value);
            void setVariable(std::string_view name, const std::shared_ptr<Variable> &value);

            std::shared_ptr<Variable> getVariable(std::string_view name) const;
            ControlFlow* getCtrlFlow();

            std::map<std::string, std::shared_ptr<Variable>>::iterator begin() { return variables.begin(); }
            std::map<std::string, std::shared_ptr<Variable>>::iterator end() { return variables.end(); }

            bool hasVar(std::string_view name) const;
            bool waitingInputs() const { return false; } // [TODO]

            std::map<std::string, std::shared_ptr<Variable>> variables;
            const bool isParallel = false;
            int timerId = -1;
        private:
            const std::unique_ptr<ControlFlow> ctrlFlow = nullptr;
    };

    class EnvironmentManager
    {
        public:
            EnvironmentManager();
            EnvironmentManager(const EnvironmentManager&) = delete;
            ~EnvironmentManager() = default;

            template <typename T>
            void setVariable(std::string_view name, const T &value);

            std::shared_ptr<Variable> getVariable(std::string_view name) const;

            Scope* hasVar(std::string_view name) const;

            void enterScope(const std::shared_ptr<RuleNode> &node, const Timer &timer);

            // enterScope and exitScope return next node to goto
            std::shared_ptr<RuleNode> enterScope(const std::shared_ptr<RuleNode> &node);
            // force used to force exit scope (used for timers)
            // shouldBlock set to true if should block
            std::shared_ptr<RuleNode> exitScope(bool &shouldBlock, bool force=false);

            int depth() const; // number of scopes
            bool inParallel() const;

            // [TODO]
            std::shared_ptr<Variable> evaluateExpression(std::string_view expr) const { return makeVarPtr("true"); }

            std::shared_ptr<RuleNode> updateTimers(); // returns next node if need to jump, else nullptr
            Timer* getTimer(int timerId) const; // should never be used, for testing only

            enum builtinTypes{
                SIZETYPE,
                CONTAINS,
                COLLECT,
                UPFROM
            };

            enum operatorTypes{
                EQUALS,
                NEGATION,
                ADDITION,
                OR
            };
            
            varType evaluationExpression(std::vector<std::string>::iterator& expression, std::vector<std::string>::iterator& end, env_mgr::Scope* scope);
            varType evaluateOperator(std::vector<std::string>::iterator& expression, std::vector<std::string>::iterator& end, Scope* scope, const std::map<std::string, operatorTypes>::iterator operatorIt);
            varType evaluateBuildIn(std::vector<std::string>::iterator& expression, std::vector<std::string>::iterator& end, Scope* scope, const std::map<std::string, builtinTypes>::iterator it);
            varType parseValue(std::string_view expression, Scope* scope);
        
        protected:
            std::deque<std::unique_ptr<Scope>> scopes;
            std::map<int, std::unique_ptr<Timer>> timers;
        private:
            std::map<std::string, enum builtinTypes> buildtinMap {
                {"size", SIZETYPE},
                {"contains", CONTAINS},
                {"collect", COLLECT},
                {"upfrom", UPFROM}
            };

            std::map<std::string, enum operatorTypes> operatorMap {
                {"=", EQUALS},
                {"!", NEGATION},
                {"+", ADDITION},
                {"|", OR}
            };
    };


    // template function definitions
    template <typename T>
    void Scope::setVariable(std::string_view name, const T &value)
    { setVariable(name, std::make_shared<Variable>(value)); }

    template <typename T>
    void EnvironmentManager::setVariable(std::string_view name, const T &value)
    {
        if(scopes.empty()) { return; }

        // check if variable already in a scope -> reset if so, else create new
        auto scope = hasVar(name);
        auto addToScope = (scope == nullptr)? scopes.front().get() : scope;
        addToScope->setVariable(name, value);
    }

};

#endif
