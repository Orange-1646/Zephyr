#include "VulkanDriver.h"
#include "VulkanBuffer.h"
#include "VulkanPipelineCache.h"
#include "VulkanRenderUnit.h"
#include "VulkanShaderSet.h"
#include "VulkanSwapchain.h"
#include "VulkanTexture.h"
#include "VulkanUtil.h"

namespace Zephyr
{
    VulkanDriver::VulkanDriver(Window* window, bool headless) :
        m_PipelineCache(this), m_SamplerCache(this), m_Window(window), m_Headless(headless)
    {
        if (!headless)
        {
            m_Swapchain = new VulkanSwapchain(window, this);
        }

        m_CommandPoolGraphics.resize(MAX_FRAME_IN_FLIGHT);
        m_CommandPoolCompute.resize(MAX_FRAME_IN_FLIGHT);
        m_CommandBufferAvailableGraphics.resize(MAX_FRAME_IN_FLIGHT);
        m_CommandBufferInUseGraphics.resize(MAX_FRAME_IN_FLIGHT);
        m_CommandBufferAvailableCompute.resize(MAX_FRAME_IN_FLIGHT);
        m_CommandBufferInUseCompute.resize(MAX_FRAME_IN_FLIGHT);
        m_SemaphoreAvailable.resize(MAX_FRAME_IN_FLIGHT);
        m_SemaphoreInUse.resize(MAX_FRAME_IN_FLIGHT);

        for (uint32_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
        {
            // create command pools and allocate command buffers
            VkCommandPoolCreateInfo createInfo {};
            createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            createInfo.queueFamilyIndex = m_Context.QueueIndices().graphics;
            VK_CHECK(vkCreateCommandPool(m_Context.Device(), &createInfo, nullptr, &m_CommandPoolGraphics[i]),
                     "Graphics Command Pool Creation");

            createInfo.queueFamilyIndex = m_Context.QueueIndices().compute;
            VK_CHECK(vkCreateCommandPool(m_Context.Device(), &createInfo, nullptr, &m_CommandPoolCompute[i]),
                     "Compute Command Pool Creation");
        }
    }

    VulkanDriver::~VulkanDriver()
    {
        vkDeviceWaitIdle(m_Context.Device());
        if (!m_Headless)
        {
            m_Swapchain->Shutdown();
        }
        m_PipelineCache.Shutdown();
        m_SamplerCache.Shutdown();
        auto device = m_Context.Device();
        // clear resource
        assert(m_ResourceCache.size() == 0);
        for (auto& resource : m_ResourceCache)
        {
            delete resource.second;
        }

        for (uint32_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
        {
            for (auto& cb : m_CommandBufferInUseGraphics[i])
            {
                vkFreeCommandBuffers(device, m_CommandPoolGraphics[i], 1, &cb);
            }
            for (auto& cb : m_CommandBufferAvailableGraphics[i])
            {
                vkFreeCommandBuffers(device, m_CommandPoolGraphics[i], 1, &cb);
            }
            for (auto& cb : m_CommandBufferInUseCompute[i])
            {
                vkFreeCommandBuffers(device, m_CommandPoolCompute[i], 1, &cb);
            }
            for (auto& cb : m_CommandBufferAvailableCompute[i])
            {
                vkFreeCommandBuffers(device, m_CommandPoolCompute[i], 1, &cb);
            }
            vkDestroyCommandPool(device, m_CommandPoolGraphics[i], nullptr);
            vkDestroyCommandPool(device, m_CommandPoolCompute[i], nullptr);

            for (auto& sem : m_SemaphoreAvailable[i])
            {
                vkDestroySemaphore(device, sem, nullptr);
            }
            for (auto& sem : m_SemaphoreInUse[i])
            {
                vkDestroySemaphore(device, sem, nullptr);
            }
        }

        vkDestroySampler(device, m_DefaultSampler, nullptr);

        delete m_Swapchain;
    }

    Handle<RHIBuffer> VulkanDriver::CreateBuffer(const BufferDescription& desc)
    {
        auto handle = GetHandle<RHIBuffer>();
        auto buffer = new VulkanBuffer(this, desc);
        m_ResourceCache.insert({handle.id, buffer});

        return handle;
    }

    Handle<RHITexture> VulkanDriver::CreateTexture(const TextureDescription& desc)
    {
        auto handle  = GetHandle<RHITexture>();
        auto texture = new VulkanTexture(this, desc);
        m_ResourceCache.insert({handle.id, texture});

        return handle;
    }

    Handle<RHIShaderSet> VulkanDriver::CreateShaderSet(const ShaderSetDescription& desc)
    {
        auto handle = GetHandle<RHIShaderSet>();
        auto shader = new VulkanShaderSet(this, desc);
        m_ResourceCache.insert({handle.id, shader});

        return handle;
    }

    Handle<RHIRenderUnit> VulkanDriver::CreateRenderUnit(const RenderUnitDescriptor& desc)
    {
        auto handle = GetHandle<RHIRenderUnit>();
        auto ru     = new VulkanRenderUnit(this, desc);
        m_ResourceCache.insert({handle.id, ru});

        return handle;
    }

    Handle<RHIRenderTarget> VulkanDriver::CreateRenderTarget(const RenderTargetDescription&         desc,
                                                             const std::vector<Handle<RHITexture>>& attachments)
    {
        auto handle = GetHandle<RHIRenderTarget>();
        auto ru     = new VulkanRenderTarget(this, desc, attachments);
        m_ResourceCache.insert({handle.id, ru});

        return handle;
    }

    Handle<RHITexture> VulkanDriver::CreateTexture(const TextureDescription& desc, VkImage image, VkFormat format)
    {
        auto handle  = GetHandle<RHITexture>();
        auto texture = new VulkanTexture(this, desc, image, format);
        m_ResourceCache.insert({handle.id, texture});

        return handle;
    }

    Handle<RHITexture> VulkanDriver::GetSwapchainImage()
    {
        auto r = m_Swapchain->GetTexture();
        auto a = GetResource<VulkanTexture>(r);
        return m_Swapchain->GetTexture();
    }

    void VulkanDriver::UpdateBuffer(const BufferUpdateDescriptor& desc, Handle<RHIBuffer> handle)
    {
        auto buffer = GetResource<VulkanBuffer>(handle);
        if (!buffer)
        {
            return;
        }
        buffer->Update(this, desc);
    }

    void VulkanDriver::UpdateTexture(const TextureUpdateDescriptor& desc, Handle<RHITexture> handle)
    {
        auto texture = GetResource<VulkanTexture>(handle);
        if (!texture)
        {
            return;
        }
        texture->Update(this, desc);
    }

    void VulkanDriver::GenerateMips(Handle<RHITexture> handle)
    {
        auto texture = GetResource<VulkanTexture>(handle);
        if (!texture)
        {
            return;
        }
        texture->GenerateMips(this);
    }

    void VulkanDriver::DestroyBuffer(Handle<RHIBuffer> handle)
    {
        auto buffer = GetResource<VulkanBuffer>(handle);
        if (!buffer)
        {
            return;
        }
        buffer->Destroy(this);
        delete buffer;
        m_ResourceCache.erase(handle.id);
    }

    void VulkanDriver::DestroyTexture(Handle<RHITexture> handle)
    {
        auto texture = GetResource<VulkanTexture>(handle);
        if (!texture)
        {
            return;
        }
        texture->Destroy(this);
        delete texture;
        m_ResourceCache.erase(handle.id);
    }

    void VulkanDriver::DestroyShaderSet(Handle<RHIShaderSet> handle)
    {
        auto shader = GetResource<VulkanShaderSet>(handle);
        if (!shader)
        {
            return;
        }
        shader->Destroy(this);
        delete shader;
        m_ResourceCache.erase(handle.id);
    }

    void VulkanDriver::DestroyRenderUnit(Handle<RHIRenderUnit> handle)
    {

        auto ru = GetResource<VulkanRenderUnit>(handle);
        if (!ru)
        {
            return;
        }
        ru->Destroy(this);
        delete ru;
        m_ResourceCache.erase(handle.id);
    }

    void VulkanDriver::DestroyRenderTarget(Handle<RHIRenderTarget> handle)
    {

        auto rt = GetResource<VulkanRenderTarget>(handle);
        if (!rt)
        {
            return;
        }
        rt->Destroy(this);
        delete rt;
        m_ResourceCache.erase(handle.id);
    }

    bool VulkanDriver::BeginFrame(uint64_t frame)
    {
        m_CurrentFrameIndex = frame % MAX_CONCURRENT_FRAME;

        m_Swapchain->PrepareFrame(m_CurrentFrameIndex);

        // if the current swapchain is stale, we skip this frame
        if (!m_Swapchain->AcquireImage())
        {
            return false;
        }
        // reset command buffers
        vkResetCommandPool(m_Context.Device(),
                           m_CommandPoolGraphics[m_CurrentFrameIndex],
                           VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
        vkResetCommandPool(
            m_Context.Device(), m_CommandPoolCompute[m_CurrentFrameIndex], VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

        // at this point all graphics and comput command buffer for this frame should be done.
        // move in-use command buffer to available command buffer
        {
            auto& cbGraphics = m_CommandBufferInUseGraphics[m_CurrentFrameIndex];

            for (auto& cb : cbGraphics)
            {
                m_CommandBufferAvailableGraphics[m_CurrentFrameIndex].push_back(cb);
            }
            cbGraphics.clear();

            auto& cbCompute = m_CommandBufferInUseCompute[m_CurrentFrameIndex];

            for (auto& cb : cbCompute)
            {
                m_CommandBufferAvailableCompute[m_CurrentFrameIndex].push_back(cb);
            }
            cbCompute.clear();
        }

        // same for semaphores
        {
            for (auto& sem : m_SemaphoreInUse[m_CurrentFrameIndex])
            {
                m_SemaphoreAvailable[m_CurrentFrameIndex].push_back(sem);
            }
            m_SemaphoreInUse[m_CurrentFrameIndex].clear();
        }

        return true;
    }

    void VulkanDriver::EndFrame()
    {
        // explicitly transition the swapchain image layout

        m_BoundVertexBuffer = VK_NULL_HANDLE;
        m_BoundIndexBuffer  = VK_NULL_HANDLE;

        SubmitJobGraphics(false, true);
        assert(m_CurrentCommandBufferCompute == VK_NULL_HANDLE);
        // end command buffer and submit
        // vkEndCommandBuffer(m_CommandBufferGraphics[m_CurrentFrameIndex]);
        // vkEndCommandBuffer(m_CommandBufferCompute[m_CurrentFrameIndex]);

        // auto                 sem1         = m_Swapchain->GetImageAcquireSemaphore();
        // auto                 sem2         = m_Swapchain->GetPresentReadySemaphore();
        // VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        // VkSubmitInfo         submitGraphics {};
        // submitGraphics.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        // submitGraphics.waitSemaphoreCount   = 1;
        // submitGraphics.pWaitSemaphores      = &sem1;
        // submitGraphics.signalSemaphoreCount = 1;
        // submitGraphics.pSignalSemaphores    = &sem2;
        // submitGraphics.commandBufferCount   = 1;
        // submitGraphics.pCommandBuffers      = &m_CommandBufferGraphics[m_CurrentFrameIndex];
        // submitGraphics.pWaitDstStageMask    = waitStages;

        // vkQueueSubmit(m_Context.m_GraphicsQueue, 1, &submitGraphics, m_Swapchain->GetFence());
        m_PipelineCache.Reset();
    }

    void VulkanDriver::WaitAndPresent()
    {
        vkDeviceWaitIdle(m_Context.Device());
        m_Swapchain->WaitAndPresent();
    }

    void VulkanDriver::DrawIndexed(uint32_t vertexOffset, uint32_t indexOffset, uint32_t indexCount)
    {
        // if there are uncommited compute command, we assume we'll use it's result in
        // our graphics task, so we have to commit the compute task and use a semaphore to link these tasks

        if (m_CurrentCommandBufferCompute != VK_NULL_HANDLE)
        {
            SubmitJobCompute(true);
        }

        auto cb = PrepareCommandBufferGraphics();

        // this will create and bind pipeline
        m_PipelineCache.Begin(cb);
        // this will bind all descriptors
        m_PipelineCache.BindDescriptor(cb);

        // auto t1 = std::chrono::steady_clock::now();
        vkCmdDrawIndexed(cb, indexCount, 1, indexOffset, vertexOffset, 0);
        // auto t2     = std::chrono::steady_clock::now();
        // auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count() / 10000;
        // std::cout << "single draw time: " << delta << std::endl;
    }

    void VulkanDriver::Draw(uint32_t vertexCount, uint32_t vertexOffset)
    {
        if (m_CurrentCommandBufferCompute != VK_NULL_HANDLE)
        {
            SubmitJobCompute(true);
        }

        auto cb = PrepareCommandBufferGraphics();

        // this will create and bind pipeline
        m_PipelineCache.Begin(cb);
        // this will bind all descriptors
        m_PipelineCache.BindDescriptor(cb);

        vkCmdDraw(cb, vertexCount, 1, vertexOffset, 0);
    }

    void VulkanDriver::Dispatch(uint32_t x, uint32_t y, uint32_t z)
    {

        if (m_CurrentCommandBufferGraphics != VK_NULL_HANDLE)
        {
            SubmitJobGraphics(true, false);
        }
        auto cb = PrepareCommandBufferCompute();

        m_PipelineCache.Begin(cb);
        m_PipelineCache.BindDescriptor(cb);

        vkCmdDispatch(cb, x, y, z);
    }

    void VulkanDriver::BeginRenderPass(Handle<RHIRenderTarget> rt)
    {
        auto vrt = GetResource<VulkanRenderTarget>(rt);
        m_PipelineCache.BindRenderPass(vrt);

        auto cb = PrepareCommandBufferGraphics();

        vrt->Begin(cb);
    }

    void VulkanDriver::EndRenderPass(Handle<RHIRenderTarget> rt)
    {
        auto vrt = GetResource<VulkanRenderTarget>(rt);
        // m_PipelineCache.Unbind();

        auto cb = PrepareCommandBufferGraphics();

        vrt->End(cb);
    }

    void VulkanDriver::BindShaderSet(Handle<RHIShaderSet> handle)
    {
        auto shader = GetResource<VulkanShaderSet>(handle);
        m_PipelineCache.BindShaderSet(shader);
    }

    void VulkanDriver::BindBuffer(Handle<RHIBuffer> buffer, uint32_t set, uint32_t binding, BufferUsage usage)
    {

        switch (usage)
        {
            case BufferUsageBits::StorageDynamic:
                m_PipelineCache.BindStorageBufferDynamic(
                    buffer, set, binding, GetResource<VulkanBuffer>(buffer)->GetOffset(m_CurrentFrameIndex));
                break;
            case BufferUsageBits::Storage:
                m_PipelineCache.BindStorageBuffer(buffer, set, binding);
                break;
            case BufferUsageBits::Uniform:
                m_PipelineCache.BindUniformBuffer(buffer, set, binding);
                break;
            case BufferUsageBits::UniformDynamic:
                m_PipelineCache.BindUniformBufferDynamic(
                    buffer, set, binding, GetResource<VulkanBuffer>(buffer)->GetOffset(m_CurrentFrameIndex));
                break;
            default:
                assert(false);
        }
    }

    void VulkanDriver::BindTexture(Handle<RHITexture> texture,
                                   const ViewRange&   range,
                                   uint32_t           set,
                                   uint32_t           binding,
                                   TextureUsage       usage,
                                   SamplerWrap        addressMode)
    {
        VkCommandBuffer cb;
        switch (usage)
        {
            case TextureUsageBits::SampledDepthStencil:
            case TextureUsageBits::Sampled:
                m_PipelineCache.BindSampler2D(texture, range, set, binding, addressMode);
                break;
            case TextureUsageBits::Storage:
                m_PipelineCache.BindStorageImage(texture, range, set, binding, addressMode);
                break;
            default:
                assert(false);
        }
    }

    void VulkanDriver::BindTexture(Handle<RHITexture> texture,
                                   uint32_t           set,
                                   uint32_t           binding,
                                   TextureUsage       usage,
                                   SamplerWrap        addressMode)
    {
        ViewRange defaultRange = {0, ALL_LAYERS, 0, ALL_LEVELS};
        BindTexture(texture, defaultRange, set, binding, usage, addressMode);
    }

    void VulkanDriver::BindConstantBuffer(uint32_t offset, uint32_t size, ShaderStage stage, void* data)
    {
        // we cannot bind push constant to both graphics and compute command buffer

        VkCommandBuffer cb = VK_NULL_HANDLE;

        if (stage & ShaderStageBits::Compute)
        {
            assert(!(stage & ShaderStageBits::Vertex || stage & ShaderStageBits::Fragment));
            cb = PrepareCommandBufferCompute();
        }
        if (stage & ShaderStageBits::Vertex || stage & ShaderStageBits::Fragment)
        {
            assert(!(stage & ShaderStageBits::Compute));
            cb = PrepareCommandBufferGraphics();
        }

        assert(cb != VK_NULL_HANDLE);
        m_PipelineCache.BindPushConstant(cb, offset, size, stage, data);
    }

    // TODO: this should be delayed too
    void VulkanDriver::BindVertexBuffer(Handle<RHIBuffer> buffer)
    {
        auto vkvb = GetResource<VulkanBuffer>(buffer)->GetBuffer();
        if (vkvb == m_BoundVertexBuffer)
        {
            return;
        }
        VkDeviceSize offset = 0;
        auto         cb     = PrepareCommandBufferGraphics();
        vkCmdBindVertexBuffers(cb, 0, 1, &vkvb, &offset);
        m_BoundVertexBuffer = vkvb;
    }

    // TODO: this should be delayed too
    void VulkanDriver::BindIndexBuffer(Handle<RHIBuffer> buffer)
    {
        auto vkib = GetResource<VulkanBuffer>(buffer)->GetBuffer();
        if (vkib == m_BoundIndexBuffer)
        {
            return;
        }
        VkDeviceSize offset = 0;
        auto         cb     = PrepareCommandBufferGraphics();
        vkCmdBindIndexBuffer(cb, vkib, 0, VK_INDEX_TYPE_UINT32);
        m_BoundIndexBuffer = vkib;
    }

    void VulkanDriver::SetRasterState(const RasterState& raster) { m_PipelineCache.SetRaster(raster); }

    void VulkanDriver::SetViewportScissor(const Viewport& viewport, const Scissor& scissor)
    {
        auto cb = PrepareCommandBufferGraphics();
        m_PipelineCache.SetViewportScissor(cb, viewport, scissor);
    }

    VkCommandBuffer VulkanDriver::BeginSingleTimeCommandBuffer() { return m_Context.BeginSingleTimeCommandBuffer(); }

    void VulkanDriver::EndSingleTimeCommandBuffer(VkCommandBuffer cb) { m_Context.EndSingleTimeCommandBuffer(cb); }

    VulkanTexture* VulkanDriver::GetTexture(HandleID id)
    {
        auto iter = m_ResourceCache.find(id);
        if (iter == m_ResourceCache.end())
        {
            return nullptr;
        }

        return reinterpret_cast<VulkanTexture*>(iter->second);
    }

    VulkanBuffer* VulkanDriver::GetBuffer(HandleID id)
    {
        auto iter = m_ResourceCache.find(id);
        if (iter == m_ResourceCache.end())
        {
            return nullptr;
        }

        return reinterpret_cast<VulkanBuffer*>(iter->second);
    }

    VkSampler VulkanDriver::GetSampler()
    {
        if (m_DefaultSampler != VK_NULL_HANDLE)
        {
            return m_DefaultSampler;
        }
        VkSamplerCreateInfo createInfo {};
        createInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        createInfo.minFilter    = VK_FILTER_LINEAR;
        createInfo.magFilter    = VK_FILTER_LINEAR;
        createInfo.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        // TODO: add mipmap&& anisotropy support

        VK_CHECK(vkCreateSampler(m_Context.Device(), &createInfo, nullptr, &m_DefaultSampler), "Sampler Creation");

        return m_DefaultSampler;
    }

    VkSampler VulkanDriver::GetSampler(SamplerWrap addressMode) { return m_SamplerCache.GetSampler(addressMode); }

    void VulkanDriver::SubmitJobCompute(bool synchronize)
    {
        VkCommandBuffer cb = m_CurrentCommandBufferCompute;
        assert(cb != VK_NULL_HANDLE);
        assert(m_CurrentSemaphoreCompute == VK_NULL_HANDLE);

        vkEndCommandBuffer(cb);

        if (synchronize)
        {
            SetupSemaphoreCompute();
        }
        VkPipelineStageFlags flags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

        // this submit consumes the current graphics semaphore
        VkSubmitInfo submit {};
        submit.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.waitSemaphoreCount = m_CurrentSemaphoreGraphics != VK_NULL_HANDLE ? 1 : 0;
        submit.pWaitSemaphores   = m_CurrentSemaphoreGraphics != VK_NULL_HANDLE ? &m_CurrentSemaphoreGraphics : nullptr;
        submit.pWaitDstStageMask = &flags;
        submit.signalSemaphoreCount = synchronize ? 1 : 0;
        submit.pSignalSemaphores    = &m_CurrentSemaphoreCompute;
        submit.commandBufferCount   = 1;
        submit.pCommandBuffers      = &m_CurrentCommandBufferCompute;

        VK_CHECK(vkQueueSubmit(m_Context.GetQueueCompute(), 1, &submit, VK_NULL_HANDLE), "Compute Queue Submit");

        m_CurrentCommandBufferCompute = VK_NULL_HANDLE;
        m_CurrentSemaphoreGraphics    = VK_NULL_HANDLE;
    }

    void VulkanDriver::SubmitJobGraphics(bool synchronize, bool present)
    {
        assert(!(synchronize & present));
        VkCommandBuffer cb = m_CurrentCommandBufferGraphics;
        assert(cb != VK_NULL_HANDLE);
        assert(m_CurrentSemaphoreGraphics == VK_NULL_HANDLE);

        vkEndCommandBuffer(cb);

        if (synchronize)
        {
            SetupSemaphoreGraphics();
        }

        // we might use result from previous compute job on either vertex or fragment
        std::vector<VkPipelineStageFlags> flags;
        auto                              sem1 = m_Swapchain->GetImageAcquireSemaphore();
        // this submit consumes the current compute semaphore
        VkSubmitInfo submit {};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        std::vector<VkSemaphore> semsWait;
        std::vector<VkSemaphore> semsSignal;

        if (present)
        {
            semsWait.push_back(m_Swapchain->GetImageAcquireSemaphore());
            flags.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

            semsSignal.push_back(m_Swapchain->GetPresentReadySemaphore());
        }
        if (m_CurrentSemaphoreCompute != VK_NULL_HANDLE)
        {
            semsWait.push_back(m_CurrentSemaphoreCompute);
            flags.push_back(VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        }

        if (synchronize)
        {
            semsSignal.push_back(m_CurrentSemaphoreGraphics);
        }

        assert(flags.size() == semsWait.size());

        submit.waitSemaphoreCount   = semsWait.size();
        submit.pWaitSemaphores      = semsWait.data();
        submit.pWaitDstStageMask    = flags.data();
        submit.signalSemaphoreCount = semsSignal.size();
        submit.pSignalSemaphores    = semsSignal.data();
        submit.commandBufferCount   = 1;
        submit.pCommandBuffers      = &m_CurrentCommandBufferGraphics;

        VK_CHECK(
            vkQueueSubmit(m_Context.GetQueueGraphics(), 1, &submit, present ? m_Swapchain->GetFence() : VK_NULL_HANDLE),
            "Graphics Queue Submit");

        m_CurrentCommandBufferGraphics = VK_NULL_HANDLE;
        m_CurrentSemaphoreCompute      = VK_NULL_HANDLE;
    }

    void VulkanDriver::SetupSemaphoreCompute()
    {
        assert(m_CurrentSemaphoreCompute == VK_NULL_HANDLE);

        // create a semaphore if there's no available ones
        if (m_SemaphoreAvailable[m_CurrentFrameIndex].size() == 0)
        {
            auto sem                  = VulkanUtil::CreateSemaphore(m_Context.Device());
            m_CurrentSemaphoreCompute = sem;
            m_SemaphoreInUse[m_CurrentFrameIndex].push_back(sem);
        }
        else
        {
            auto sem = m_SemaphoreAvailable[m_CurrentFrameIndex].back();
            m_SemaphoreAvailable[m_CurrentFrameIndex].pop_back();
            m_CurrentSemaphoreCompute = sem;
            m_SemaphoreInUse[m_CurrentFrameIndex].push_back(sem);
        }
    }

    void VulkanDriver::SetupSemaphoreGraphics()
    {
        assert(m_CurrentSemaphoreGraphics == VK_NULL_HANDLE);

        // create a semaphore if there's no available ones
        if (m_SemaphoreAvailable[m_CurrentFrameIndex].size() == 0)
        {
            auto sem                   = VulkanUtil::CreateSemaphore(m_Context.Device());
            m_CurrentSemaphoreGraphics = sem;
            m_SemaphoreInUse[m_CurrentFrameIndex].push_back(sem);
        }
        else
        {
            auto sem = m_SemaphoreAvailable[m_CurrentFrameIndex].back();
            m_SemaphoreAvailable[m_CurrentFrameIndex].pop_back();
            m_CurrentSemaphoreGraphics = sem;
            m_SemaphoreInUse[m_CurrentFrameIndex].push_back(sem);
        }
    }

    VkCommandBuffer VulkanDriver::PrepareCommandBufferGraphics()
    {
        // if we already have one, just return it
        if (m_CurrentCommandBufferGraphics != VK_NULL_HANDLE)
        {
            return m_CurrentCommandBufferGraphics;
        }

        // if there are available command buffer, use it
        if (m_CommandBufferAvailableGraphics[m_CurrentFrameIndex].size() != 0)
        {
            m_CurrentCommandBufferGraphics = m_CommandBufferAvailableGraphics[m_CurrentFrameIndex].back();
            m_CommandBufferAvailableGraphics[m_CurrentFrameIndex].pop_back();
            m_CommandBufferInUseGraphics[m_CurrentFrameIndex].push_back(m_CurrentCommandBufferGraphics);

            VkCommandBufferBeginInfo begin {};
            begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            VK_CHECK(vkBeginCommandBuffer(m_CurrentCommandBufferGraphics, &begin), "Begin Graphics Command Buffer");

            return m_CurrentCommandBufferGraphics;
        }

        // else we create a new command buffer
        VkCommandBuffer cb;

        VkCommandBufferAllocateInfo allocInfo {};
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool        = m_CommandPoolGraphics[m_CurrentFrameIndex];
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VK_CHECK(vkAllocateCommandBuffers(m_Context.Device(), &allocInfo, &cb), "Graphics Command Buffer Allocation");

        m_CurrentCommandBufferGraphics = cb;

        m_CommandBufferInUseGraphics[m_CurrentFrameIndex].push_back(cb);

        VkCommandBufferBeginInfo begin {};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(m_CurrentCommandBufferGraphics, &begin), "Begin Graphics Command Buffer");

        return m_CurrentCommandBufferGraphics;
    }

    VkCommandBuffer VulkanDriver::PrepareCommandBufferCompute()
    {
        // if we already have one, just return it
        if (m_CurrentCommandBufferCompute != VK_NULL_HANDLE)
        {
            return m_CurrentCommandBufferCompute;
        }

        // if there are available command buffer, use it
        if (m_CommandBufferAvailableCompute[m_CurrentFrameIndex].size() != 0)
        {
            m_CurrentCommandBufferCompute = m_CommandBufferAvailableCompute[m_CurrentFrameIndex].back();
            m_CommandBufferAvailableCompute[m_CurrentFrameIndex].pop_back();
            m_CommandBufferInUseCompute[m_CurrentFrameIndex].push_back(m_CurrentCommandBufferCompute);

            VkCommandBufferBeginInfo begin {};
            begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            VK_CHECK(vkBeginCommandBuffer(m_CurrentCommandBufferCompute, &begin), "Begin Graphics Command Buffer");

            return m_CurrentCommandBufferCompute;
        }

        // else we create a new command buffer
        VkCommandBuffer cb;

        VkCommandBufferAllocateInfo allocInfo {};
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool        = m_CommandPoolCompute[m_CurrentFrameIndex];
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VK_CHECK(vkAllocateCommandBuffers(m_Context.Device(), &allocInfo, &cb), "Graphics Command Buffer Allocation");

        m_CurrentCommandBufferCompute = cb;

        m_CommandBufferInUseCompute[m_CurrentFrameIndex].push_back(cb);

        VkCommandBufferBeginInfo begin {};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(m_CurrentCommandBufferCompute, &begin), "Begin Graphics Command Buffer");

        return m_CurrentCommandBufferCompute;
    }

    void VulkanDriver::SetupBarrier(Handle<RHITexture> texture,
                                    const ViewRange&   range,
                                    TextureUsage       nextUsage,
                                    PipelineType       pipeline)
    {
        VkCommandBuffer cb = VK_NULL_HANDLE;

        if (pipeline == PipelineTypeBits::Graphics)
        {
            cb = PrepareCommandBufferGraphics();
        }
        if (pipeline == PipelineTypeBits::Compute)
        {
            cb = PrepareCommandBufferCompute();
        }
        assert(cb != VK_NULL_HANDLE);

        auto vkTexture = GetResource<VulkanTexture>(texture);

        // FIXME: this should not set all layers and all levels. the function should accept layer and level as parameter
        vkTexture->TransitionLayout(cb,
                                    0,
                                    range.baseLayer,
                                    range.layerCount,
                                    range.baseLevel,
                                    range.levelCount,
                                    VulkanUtil::GetImageLayoutFromUsage(nextUsage),
                                    pipeline);
    }

} // namespace Zephyr