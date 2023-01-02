#pragma once
#include "pch.h"
#include "rhi/vulkan/VulkanCommon.h"
#include "rhi/vulkan/VulkanContext.h"
#include "rhi/vulkan/VulkanTexture.h"
#include "rhi/vulkan/VulkanDriver.h"

namespace Zephyr
{
    struct Texture
    {
        VkImage        image;
        VkDeviceMemory memory;
        VkImageView    view;
    };

    class SMExecutor
    {
    public:
        SMExecutor();
        ~SMExecutor();

        bool Run();

        void LoadCubemap();
        void CreateComputeTarget();
        void CreatePipeline();
        void DispatchCompute();
        void Save();

    private:
        VulkanDriver* m_Driver;
        Handle<RHITexture> m_Cubemap;
        Handle<RHITexture> m_Target;
        Handle<RHIShaderSet> m_Shader;
    };
} // namespace Zephyr