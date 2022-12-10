#pragma once
#include "Handle.h"
#include "RHIBuffer.h"
#include "RHIEnums.h"
#include "RHIRenderTarget.h"
#include "RHIRenderUnit.h"
#include "RHIShaderSet.h"
#include "RHITexture.h"

namespace Zephyr
{
    class Mesh;
    class Window;
    /*
        Zephyr engine backend
        Driver instance controls all backend resources like render context, swapchain,
        actual gpu texture/buffer, pipeline caches, renderpass, etc.

    */
    class Driver
    {
    public:
        static Driver* Create(DriverType backend, Window* window);
        virtual ~Driver() {}

        // rhi functions
        //  resource allocation
        virtual Handle<RHIBuffer>       CreateBuffer(const BufferDescription& desc)                            = 0;
        virtual Handle<RHITexture>      CreateTexture(const TextureDescription& desc)                          = 0;
        virtual Handle<RHIShaderSet>    CreateShaderSet(const ShaderSetDescription& desc)                      = 0;
        virtual Handle<RHIRenderUnit>   CreateRenderUnit(const RenderUnitDescriptor& desc)                     = 0;
        virtual Handle<RHIRenderTarget> CreateRenderTarget(const RenderTargetDescription&         desc,
                                                           const std::vector<Handle<RHITexture>>& attachments) = 0;
        virtual Handle<RHITexture>      GetSwapchainImage()                                                    = 0;

        // resouce update
        virtual void UpdateBuffer(const BufferUpdateDescriptor& desc, Handle<RHIBuffer>)    = 0;
        virtual void UpdateTexture(const TextureUpdateDescriptor& desc, Handle<RHITexture>) = 0;

        // resource destruction
        virtual void DestroyBuffer(Handle<RHIBuffer> buffer)         = 0;
        virtual void DestroyTexture(Handle<RHITexture> texture)      = 0;
        virtual void DestroyShaderSet(Handle<RHIShaderSet> shader)   = 0;
        virtual void DestroyRenderUnit(Handle<RHIRenderUnit> ru)     = 0;
        virtual void DestroyRenderTarget(Handle<RHIRenderTarget> rt) = 0;

        // render methods
        virtual bool BeginFrame(uint64_t frame)                                                    = 0;
        virtual void EndFrame()                                                                    = 0;
        virtual void WaitAndPresent()                                                              = 0;
        virtual void DrawIndexed(uint32_t vertexOffset, uint32_t indexOffset, uint32_t indexCount) = 0;
        virtual void Draw(uint32_t vertexCount, uint32_t vertexOffset)                             = 0;
        virtual void Dispatch(uint32_t x, uint32_t y, uint32_t z) = 0;

        virtual void BeginRenderPass(Handle<RHIRenderTarget> rt)                                                 = 0;
        virtual void EndRenderPass(Handle<RHIRenderTarget> rt)                                                   = 0;
        virtual void BindShaderSet(Handle<RHIShaderSet> shader)                                                  = 0;
        virtual void BindBuffer(Handle<RHIBuffer> buffer, uint32_t set, uint32_t binding, BufferUsage usage)     = 0;
        virtual void BindTexture(Handle<RHITexture> texture, uint32_t set, uint32_t binding, TextureUsage usage) = 0;
        virtual void BindConstantBuffer(uint32_t offset, uint32_t size, ShaderStage stage, void* data)           = 0;
        virtual void BindVertexBuffer(Handle<RHIBuffer> buffer)                                                  = 0;
        virtual void BindIndexBuffer(Handle<RHIBuffer> buffer)                                                   = 0;
        virtual void SetRasterState(const RasterState& raster)                                                   = 0;
        virtual void SetViewportScissor(const Viewport& viewport, const Scissor& scissor)                        = 0;

        virtual void BeginGraphicsCommand() = 0;
        virtual void EndGraphicsCommand()   = 0;
        virtual void BeginComputeCommand()  = 0;
        virtual void EndComputeCommand()    = 0;

        // this is for the image layout transition between passes
        virtual void SetupBarrier(Handle<RHITexture> texture, TextureUsage nextUsage) = 0;

        virtual void WaitIdle() = 0;
    };
} // namespace Zephyr