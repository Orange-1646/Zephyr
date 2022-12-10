#include "System.h"

namespace Zephyr
{
    void SystemBase::Tick(float delta, const std::vector<Entity*>& entities) {
        if (!m_Enabled)
        {
            return;
        }
        std::vector<Entity*> list;

        for (auto& entity : entities)
        {
            if (m_Query.Qualify(entity))
            {
                list.push_back(entity);
            }
        }
        Execute(delta, list);
    }
} // namespace Zephyr