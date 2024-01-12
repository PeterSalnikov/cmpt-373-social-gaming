#ifndef TASK_FACTORY_H
#define TASK_FACTORY_H
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <vector>
#include "Variables.hpp"
#include <unordered_map>

using namespace var;
using mutableVarPointerVector = std::vector<std::shared_ptr<Variable>>;

namespace taskFactory
{
    class RunnableTask
    {
        public:
            RunnableTask() = default;
            RunnableTask(const RunnableTask&) = delete;
            virtual ~RunnableTask() = default;

            RunnableTask& operator=(const RunnableTask&) = delete;
            RunnableTask& operator=(RunnableTask&&) = delete;

            virtual void run() = 0;
            virtual int getType() const { return type; }
        protected:
            int type;
    };

    class TaskFactory
    {
        public:
            TaskFactory() = default;
            virtual std::shared_ptr<RunnableTask> create(mutableVarPointerVector &vars) const = 0;
        private:
    };

    template <typename convertType, typename enumType, typename argSource>
    class Converter
    {
        public:
            virtual std::shared_ptr<RunnableTask> convert(std::shared_ptr<convertType> task) = 0;
            virtual void addFactory(enumType e, std::shared_ptr<TaskFactory> factory) = 0;
            std::shared_ptr<argSource> src;
    };
}

#endif