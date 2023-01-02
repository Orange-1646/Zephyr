#pragma once
#include "pch.h"

namespace Zephyr
{
    using ComponentId = uint64_t;
    constexpr uint32_t InvalidComponentId = 0;

    struct ComponentBase
    {
        static uint32_t NextComponentId() {
            static uint32_t nextId = 1;

            uint32_t id = nextId;

            nextId <<= 1;

            return id;
        }
    };

    template<typename T>
    struct Component : ComponentBase
    {
        static const uint32_t ID;

        inline const uint32_t GetID() { return Component<T>::ID; }
    };

    template<typename T>
    const uint32_t Component<T>::ID(ComponentBase::NextComponentId());


    #define COMPONENT(c) struct c: Component<c>
}