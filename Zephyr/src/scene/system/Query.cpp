#include "Query.h"
#include "scene/Entity.h"

namespace Zephyr
{
    Query::Query(const std::vector<uint32_t>& all, const std::vector<uint32_t>& some, const std::vector<uint32_t>& exclude) :
        m_All(all),
        m_Some(some), m_Exclude(exclude)
    {
        for (auto& id : all)
        {
            m_AllMask |= id;
        }

        for (auto& id : some)
        {
            m_SomeMask |= id;
        }

        for (auto& id : exclude)
        {
            m_ExcludeMask |= id;
        }
    }

    bool Query::Qualify(const Entity* const entity) {
        return CheckExclude(entity) && CheckSome(entity) && CheckAll(entity);
    }
    bool Query::CheckExclude(const Entity* const entity)
    {
        return m_ExcludeMask == 0 || (entity->m_ComponentMask & m_ExcludeMask) == 0;
    }
    bool Query::CheckSome(const Entity* const entity)
    {
        return m_SomeMask == 0 || (entity->m_ComponentMask & m_SomeMask) != 0;
    }
    bool Query::CheckAll(const Entity* const entity) 
    {
        return (entity->m_ComponentMask & m_AllMask) == m_AllMask;
    }
} // namespace Zephyr