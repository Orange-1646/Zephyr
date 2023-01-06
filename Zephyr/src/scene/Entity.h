#pragma once
#include "pch.h"
#include "scene/component/Component.h"

namespace Zephyr
{
    class Entity final
    {
    public:
        Entity() = default;
        ~Entity()
        {
            for (auto& component : m_Components)
            {
                delete component.second;
            }
        }

        template<typename T, typename = std::enable_if_t<std::is_base_of_v<ComponentBase, T>>>
        T* AddComponent()
        {
            if (HasComponent<T>())
            {
                return static_cast<T*>(m_Components.at(T::ID));
            }

            T* component = new T();

            m_Components.insert({T::ID, component});

            m_ComponentMask |= T::ID;

            return component;
        }

        template<typename T>
        void RemoveComponent()
        {
            if (!HasComponent<T>())
            {
                return;
            }

            T* component = m_Components.at(T::ID);
            delete component;
            m_Components.erase(T::ID);
            m_ComponentMask &= ~T::ID;
        }

        template<typename T>
        bool HasComponent()
        {
            auto a      = T::ID;
            auto result = m_ComponentMask & a;
            return (m_ComponentMask & T::ID) != 0;
        }

        template<typename T>
        T* GetComponent()
        {
            if (!HasComponent<T>())
            {
                return nullptr;
            }
            return static_cast<T*>(m_Components.at(T::ID));
        }

        inline const std::unordered_map<uint32_t, ComponentBase*>& GetComponents() const { return m_Components; }

    public:
        std::unordered_map<uint32_t, ComponentBase*> m_Components;
        // note that for small component size, iterating through vector is actually faster than hashmap lookup
        // need more tests after whole engine is finished
        // std::vector<std::pair<uint32_t, ComponentBase*>> m_Components;
        uint32_t m_ComponentMask = 0;
    };
} // namespace Zephyr