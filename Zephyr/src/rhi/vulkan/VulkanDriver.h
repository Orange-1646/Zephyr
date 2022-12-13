#pragma once
#include "VulkanBuffer.h"
#include "VulkanCommon.h"
#include "VulkanContext.h"
#include "VulkanPipelineCache.h"
#include "VulkanRenderTarget.h"
#include "VulkanSwapchain.h"
#include "pch.h"
#include "rhi/Driver.h"
#include "rhi/Handle.h"

namespace Zephyr
{
    class VulkanSwapchain;
    class VulkanTexture;
    class VulkanDriver final : public Driver
    {
    public:
        VulkanDriver(Window* window);
        ~VulkanDriver() override;

        inline Window*        GetWindow() { return m_Window; }
        inline VulkanContext* GetContext() { return &m_Context; }
        inline uint32_t       GetCurrentFrameIndex() { return m_CurrentFrameIndex; }

        // resource allocation
        Handle<RHIBuffer>       CreateBuffer(const BufferDescription& desc) override;
        Handle<RHITexture>      CreateTexture(const TextureDescription& desc) override;
        Handle<RHIShaderSet>    CreateShaderSet(const ShaderSetDescription& desc) override;
        Handle<RHIRenderUnit>   CreateRenderUnit(const RenderUnitDescriptor& desc) override;
        Handle<RHIRenderTarget> CreateRenderTarget(const RenderTargetDescription&         desc,
                                                   const std::vector<Handle<RHITexture>>& attachments) override;
        Handle<RHITexture>      CreateTexture(const TextureDescription& desc, VkImage image, VkFormat format);
        Handle<RHITexture>      GetSwapchainImage() override;
        // resource update
        void UpdateBuffer(const BufferUpdateDescriptor& desc, Handle<RHIBuffer> handle);
        void UpdateTexture(const TextureUpdateDescriptor& desc, Handle<RHITexture> handle);

        // resource destruction
        void DestroyBuffer(Handle<RHIBuffer> handle) override;
        void DestroyTexture(Handle<RHITexture> handle) override;
        void DestroyShaderSet(Handle<RHIShaderSet> handle) override;
        void DestroyRenderUnit(Handle<RHIRenderUnit> ru) override;
        void DestroyRenderTarget(Handle<RHIRenderTarget> rt) override;

        // render methods
        bool BeginFrame(uint64_t frame) override;
        void EndFrame() override;
        void WaitAndPresent() override;
        void DrawIndexed(uint32_t vertexOffset, uint32_t indexOffset, uint32_t indexCount) override;
        void Draw(uint32_t vertexCount, uint32_t vertexOffset) override;
        void Dispatch(uint32_t x, uint32_t y, uint32_t z) override;

        void BeginRenderPass(Handle<RHIRenderTarget> rt) override;
        void EndRenderPass(Handle<RHIRenderTarget> rt) override;
        void BindShaderSet(Handle<RHIShaderSet> shader) override;
        void BindBuffer(Handle<RHIBuffer> buffer, uint32_t set, uint32_t binding, BufferUsage usage) override;
        void BindTexture(Handle<RHITexture> texture,
                         const ViewRange&   range,
                         uint32_t           set,
                         uint32_t           binding,
                         TextureUsage       usage) override;
        void BindTexture(Handle<RHITexture> texture, uint32_t set, uint32_t binding, TextureUsage usage) override;
        void BindConstantBuffer(uint32_t offset, uint32_t size, ShaderStage stage, void* data) override;
        void BindVertexBuffer(Handle<RHIBuffer> buffer) override;
        void BindIndexBuffer(Handle<RHIBuffer> buffer) override;
        void SetRasterState(const RasterState& raster) override;
        void SetViewportScissor(const Viewport& viewport, const Scissor& scissor);

        void SetupBarrier(Handle<RHITexture> texture, const ViewRange& range, TextureUsage nextUsage, PipelineType pipeline) override;

        // for internal uses
        VkCommandBuffer BeginSingleTimeCommandBuffer();
        void            EndSingleTimeCommandBuffer(VkCommandBuffer cb);
        VulkanTexture*  GetTexture(HandleID id);
        VulkanBuffer*   GetBuffer(HandleID id);
        VkSampler       GetSampler();

        void WaitIdle() { vkDeviceWaitIdle(m_Context.Device()); }

        template<typename T>
        Handle<T> GetHandle()
        {
            return Handle<T>(m_HandleIdNext++);
        }

        template<typename T, typename U, typename = std::enable_if_t<std::is_base_of_v<U, T>>>
        T* GetResource(Handle<U> handle)
        {
            auto iter = m_ResourceCache.find(handle.id);
            if (iter == m_ResourceCache.end())
            {
                return nullptr;
            }

            return reinterpret_cast<T*>(iter->second);
        }

        inline VkFormat GetSurfaceFormat() { return m_Swapchain->GetSurfaceFormat(); }

    private:
        // synchronize indicates if there are graphics job after us and we need to provide a semaphore to sync with it
        void SubmitJobCompute(bool synchronize);
        // synchronize indicates if there are compute job after us and we need to provide a semaphore to sync with it
        void SubmitJobGraphics(bool synchronize, bool present);

        VkCommandBuffer PrepareCommandBufferGraphics();
        VkCommandBuffer PrepareCommandBufferCompute();

        void SetupSemaphoreCompute();
        void SetupSemaphoreGraphics();

    private:
        Window*          m_Window;
        VulkanSwapchain* m_Swapchain;
        // Do NOT change the order of declaration of the member below.
        VulkanContext       m_Context;
        VulkanPipelineCache m_PipelineCache;

        uint32_t m_CurrentFrameIndex = 0;

        uint32_t                            m_HandleIdNext = 0;
        std::unordered_map<HandleID, void*> m_ResourceCache;

        VkCommandBuffer                           m_CurrentGraphicsCB = VK_NULL_HANDLE;
        VkCommandBuffer                           m_CurrentComputeCB  = VK_NULL_HANDLE;
        std::vector<VkCommandPool>                m_CommandPoolGraphics;
        std::vector<std::vector<VkCommandBuffer>> m_CommandBufferAvailableGraphics;
        std::vector<std::vector<VkCommandBuffer>> m_CommandBufferInUseGraphics;
        VkCommandBuffer                           m_CurrentCommandBufferGraphics = VK_NULL_HANDLE;

        std::vector<VkCommandBuffer> m_CommandBufferGraphics;

        std::vector<VkCommandPool>                m_CommandPoolCompute;
        std::vector<std::vector<VkCommandBuffer>> m_CommandBufferAvailableCompute;
        std::vector<std::vector<VkCommandBuffer>> m_CommandBufferInUseCompute;
        VkCommandBuffer                           m_CurrentCommandBufferCompute = VK_NULL_HANDLE;

        std::vector<std::vector<VkSemaphore>> m_SemaphoreAvailable;
        std::vector<std::vector<VkSemaphore>> m_SemaphoreInUse;

        VkSemaphore m_CurrentSemaphoreGraphics = VK_NULL_HANDLE;
        VkSemaphore m_CurrentSemaphoreCompute  = VK_NULL_HANDLE;

        std::vector<VkCommandBuffer> m_CommandBufferCompute;

        VkSampler m_DefaultSampler = VK_NULL_HANDLE;

        VkBuffer m_BoundVertexBuffer = VK_NULL_HANDLE;
        VkBuffer m_BoundIndexBuffer  = VK_NULL_HANDLE;

        std::string m_DebugMarkerName;
    };
} // namespace Zephyr