#pragma once
#include "Entity.h"
#include "component/Component.h"
#include "pch.h"
#include "system/System.h"

namespace Zephyr
{
    class Engine;

    using EntityHandle                                = intptr_t;
    using SystemHandle                                = intptr_t;
    inline constexpr SystemHandle InvalidSystemHandle = 0;

    /*
        A scene contains a list of entities and systems.
        Entities contains their components(data only)
        Systems use query to look for proper entity and update entity based on their components
    */
    class Scene final
    {
    public:
        Scene(Engine* engine, const std::string& name) : m_Engine(engine), m_DebugName(name) {}
        void Shutdown()
        {
            for (auto& system : m_Systems)
            {
                system.second->Shutdown();
            }
        }
        ~Scene()
        {
            for (auto& entity : m_Entities)
            {
                delete entity;
            }

            for (auto& system : m_Systems)
            {
                delete system.second;
            }
        }

        EntityHandle CreateEntity()
        {
            auto entity = new Entity();
            m_Entities.push_back(entity);

            return (EntityHandle)entity;
        }

        template<typename T, typename = std::enable_if_t<std::is_base_of_v<ComponentBase, T>>>
        T* AddComponent(EntityHandle handle)
        {
            Entity* entity = reinterpret_cast<Entity*>(handle);

            return entity->AddComponent<T>();
        }

        template<typename T, typename = std::enable_if_t<std::is_base_of_v<ComponentBase, T>>>
        T* GetComponent(EntityHandle handle)
        {
            Entity* entity = reinterpret_cast<Entity*>(handle);

            return entity->GetComponent<T>();
        }

        bool RemoveEntity(EntityHandle handle)
        {
            Entity* entity = reinterpret_cast<Entity*>(handle);
            for (auto iter = m_Entities.begin(); iter != m_Entities.end(); iter++)
            {
                if (*iter == entity)
                {
                    delete entity;
                    m_Entities.erase(iter);
                    return true;
                }
            }

            return false;
        }

        template<typename T, typename = std::enable_if_t<std::is_base_of_v<SystemBase, T>>>
        SystemHandle AddSystem()
        {
            auto system = new T(m_Engine);

            m_Systems.insert({T::ID, system});

            return (SystemHandle)system;
        }

        template<typename T, typename = std::enable_if_t<std::is_base_of_v<SystemBase, T>>>
        bool DisableSystem()
        {
            auto system = HaveSystem<T>();

            if (system == InvalidSystemHandle)
            {
                return false;
            }

            reinterpret_cast<SystemBase*>(system)->Disable();

            return true;
        }

        template<typename T, typename = std::enable_if_t<std::is_base_of_v<SystemBase, T>>>
        bool EnableSystem()
        {
            auto system = HaveSystem<T>();

            if (system == InvalidSystemHandle)
            {
                return false;
            }

            reinterpret_cast<SystemBase*>(system)->Enable();

            return true;
        }

        template<typename T, typename = std::enable_if_t<std::is_base_of_v<SystemBase, T>>>
        SystemHandle HaveSystem()
        {
            auto iter = m_Systems.find(T::ID);

            if (iter == m_Systems.end())
            {
                return 0;
            }

            return (uint32_t)iter.second;
        }

        void Tick(float delta)
        {
            for (auto& system : m_Systems)
            {
                system.second->Tick(delta, m_Entities);
            }
        }

    private:
        Engine*                                   m_Engine;
        std::string                               m_DebugName;
        std::vector<Entity*>                      m_Entities;
        std::unordered_map<uint32_t, SystemBase*> m_Systems;
    };
} // namespace Zephyr