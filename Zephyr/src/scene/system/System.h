#pragma once
#include "pch.h"
#include "Query.h"

namespace Zephyr
{
    class Engine;
    class Entity;
    class SystemBase
    {
    public:
        static uint32_t NextID() { 
            static uint32_t id = 0;
            return id++;
        }

        SystemBase() = default;
        virtual ~SystemBase() = default;

        SystemBase(Engine* engine, const std::vector<uint32_t>& all,
                   const std::vector<uint32_t>& some,
                   const std::vector<uint32_t>& exclude) :
            m_Engine(engine), m_Query(all, some, exclude)
        {}

        void Disable() { m_Enabled = false; }

        void Enable() { m_Enabled = true; }

        void Tick(float delta, const std::vector<Entity*>& entities);

        virtual void Execute(float delta, const std::vector<Entity*>& entities) = 0;
        virtual void Shutdown()                                                 = 0;

    protected:
        Query m_Query;
        Engine* m_Engine;
        bool  m_Enabled = true;
    };

    template<typename T>
    class System : public SystemBase
    {
    public:
        static const uint32_t ID;
        System(Engine* engine, const std::vector<uint32_t>& all,
                   const std::vector<uint32_t>& some,
                   const std::vector<uint32_t>& exclude) :
            SystemBase(engine, all, some, exclude)
        {}
    };

    template<typename T>
    const uint32_t System<T>::ID = SystemBase::NextID();

    #define SYSTEM(s) class s : public System<s>
}