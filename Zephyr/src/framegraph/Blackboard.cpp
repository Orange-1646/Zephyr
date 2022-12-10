#include "Blackboard.h"

namespace Zephyr
{
    void Blackboard::Set(const std::string& name, FrameGraphResourceHandle<FrameGraphTexture> handle)
    {
        auto iter = m_HandleMap.find(name);

        if (iter == m_HandleMap.end())
        {
            m_HandleMap.insert({name, handle});
        }
        else
        {
            iter->second = handle;
        }
    }
} // namespace Zephyr