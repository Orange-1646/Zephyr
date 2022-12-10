#pragma once
#include "pch.h"

namespace Zephyr
{
    class Entity;
    class Query final
    {
    public:
        Query() = default;
        Query(const std::vector<uint32_t>& all, const std::vector<uint32_t>& some, const std::vector<uint32_t>& exclude);
        ~Query() = default;

        bool Qualify(const Entity* const entity);

        private:
        bool CheckAll(const Entity* const entity);
        bool CheckSome(const Entity* const entity);
        bool CheckExclude(const Entity* const entity);
    private:
        std::vector<uint32_t> m_All;
        std::vector<uint32_t> m_Some;
        std::vector<uint32_t> m_Exclude;

        uint32_t m_AllMask = 0;
        uint32_t m_SomeMask = 0;
        uint32_t m_ExcludeMask = 0;
    };
}