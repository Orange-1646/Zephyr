#pragma once
#include "pch.h"
#include "FrameGraphResource.h";

namespace Zephyr
{
    class Blackboard
    {
    public:
        Blackboard() = default;
        ~Blackboard() = default;

        void Set(const std::string& name, FrameGraphResourceHandle<FrameGraphTexture> handle);

        FrameGraphResourceHandle<FrameGraphTexture> Get(const std::string& name)
        {
            auto iter = m_HandleMap.find(name);
            if (iter == m_HandleMap.end())
            {
                return {};
            }

            return iter->second;
        }

    private:
        std::unordered_map<std::string, FrameGraphResourceHandle<FrameGraphTexture>> m_HandleMap;
    };
} // namespace Zephyr