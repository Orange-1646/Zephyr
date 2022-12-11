#include "Renderer.h"
#include "engine/Engine.h"
#include "framegraph/FrameGraph.h"
#include "resource/Buffer.h"
#include "resource/MaterialInstance.h"
#include "rhi/Driver.h"
#include "rhi/RHIEnums.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Zephyr
{
    Renderer::Renderer(Engine* engine) : m_Engine(engine), m_Driver(engine->GetDriver()), m_Manager(m_Driver)
    {
        // setup global render ringbuffer
        BufferDescription desc {};
        desc.memoryType   = BufferMemoryType::DynamicRing;
        desc.size         = sizeof(GlobalRenderShaderData);
        desc.pipelines    = PipelineTypeBits::Graphics;
        desc.shaderStages = ShaderStageBits::Vertex | ShaderStageBits::Fragment;
        desc.usage        = BufferUsageBits::StorageDynamic;

        m_GlobalRingBuffer = engine->CreateBuffer(desc);
    }
    void Renderer::Shutdown() { m_Manager.Shutdown(); }

    Renderer::~Renderer() {}

    void Renderer::Render(const SceneRenderData& scene)
    {
        m_Scene = scene;
        PrepareScene();
        SetupGlobalRenderData();
        //  build frame graph

        if (!m_Driver->BeginFrame(m_Engine->GetFrame()))
        {
            return;
        }

        FrameGraph fg(m_Engine, &m_Manager);

        DrawShadowMap(fg);
        DrawForward(fg);
        DispatchPostProcessingCompute(fg);
        DrawResolve(fg);

        fg.Compile();
        fg.Execute();

        m_Driver->EndFrame();
        m_Driver->WaitAndPresent();
    }
    // create mesh render unit if not found in cache
    void Renderer::PrepareScene()
    {
        m_SceneRenderUnit.clear();
        uint32_t i = 0;
        for (auto& mesh : m_Scene.meshes)
        {
            for (auto& submesh : mesh->GetSubmeshes())
            {
                auto& ru        = m_SceneRenderUnit.emplace_back();
                ru.vertex       = mesh->GetVertexBuffer();
                ru.index        = mesh->GetIndexBuffer();
                ru.vertexOffset = submesh.baseVertex;
                ru.indexOffset  = submesh.baseIndex;
                ru.indexCount   = submesh.indexCount;
                ru.material     = mesh->GetMaterials()[submesh.materialIndex];
                ru.transform    = m_Scene.transforms[i] * submesh.transform;
            }
            i++;
        }
    }

    void Renderer::SetupGlobalRenderData()
    {
        auto& camera = m_Scene.camera;
        auto& light  = m_Scene.light;

        m_GlobalShaderData.view                      = camera.view;
        m_GlobalShaderData.projection                = camera.projection;
        m_GlobalShaderData.vp                        = camera.projection * camera.view;
        m_GlobalShaderData.directionalLightDirection = light.direction;
        m_GlobalShaderData.directionalLightRadiance  = light.radiance;
        m_GlobalShaderData.eye                       = camera.position;

        PrepareCascadedShadowData();

        BufferUpdateDescriptor update {};
        update.data      = &m_GlobalShaderData;
        update.size      = sizeof(m_GlobalShaderData);
        update.srcOffset = 0;
        update.dstOffset = 0;

        m_Driver->UpdateBuffer(update, m_GlobalRingBuffer->GetHandle());
    }

    void Renderer::PrepareCascadedShadowData()
    {
        auto& camera = m_Scene.camera;
        auto& light  = m_Scene.light;

        uint32_t cascadeCount         = 4;
        float    cascadeExponentScale = 2.5;

        // 基于缩放系数（m_cascadeExponentScale）计算等比数列
        float expScale[8] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}; // 我的框架最多允许 8 级级联
        float expNormalizeFactor = 1.0f;

        for (uint32_t i = 1; i < cascadeCount; i++)
        {
            expScale[i] = expScale[i - 1] * cascadeExponentScale;
            expNormalizeFactor += expScale[i];
        }
        expNormalizeFactor = 1.0f / expNormalizeFactor;

        float percentage = 0.0f;
        float lastZPct   = 0.f;

        glm::mat4 cascadeShadowVP[4];

        for (uint32_t i = 0; i < cascadeCount; i++)
        {
            // 计算每一级 cascade 的百分比例值
            float percentageOffset = expScale[i] * expNormalizeFactor;

            // 将比例值映射到 Frustum，从而求出各级 SubFrustum 的 zNear 和 zFar
            float zNearPct = percentage;
            percentage += percentageOffset;
            float zFarPct = percentage;

            float zPct = zFarPct - zNearPct;

            float lastTransitionPct = m_CascadeTransitionScale * lastZPct;

            zNearPct -= lastTransitionPct;
            lastZPct = zPct;

            auto cascadeBoundingSphere = Camera::GetWorldSpaceOuterTangentSphere(camera, zNearPct, zFarPct);

            // 生成各级AABB....

            // shadowmap camera construction
            auto& center = cascadeBoundingSphere.m_Center;
            float radius = cascadeBoundingSphere.m_Radius;

            glm::vec3 se = {0.f, 0.f, 0.f};
            glm::vec3 sd = -light.direction;
            glm::vec3 su = {0.f, 1.f, 0.f};

            glm::mat4 w2l = glm::lookAt(se, se + sd, su);

            center = w2l * glm::vec4(center, 1.);

            // hou much of the shadow map is mapped to one unit of the shadow frustum
            float shadowMapPixelSize = 2.f * radius / m_ShadowMapResolution;

            center.x -= std::fmodf(center.x, shadowMapPixelSize);
            center.y -= std::fmodf(center.y, shadowMapPixelSize);

            center = glm::inverse(w2l) * glm::vec4(center, 1.f);

            glm::vec3 shadowMapEye       = center - (200.f + radius) * glm::normalize(light.direction);
            glm::vec3 shadowMapUp        = {0.f, 1.f, 0.f};
            glm::vec3 shadowMapDirection = light.direction;

            glm::mat4 shadowMapView       = glm::lookAt(shadowMapEye, shadowMapEye + shadowMapDirection, shadowMapUp);
            glm::mat4 shadowMapProjection = glm::ortho(-radius, radius, -radius, radius, 0.0f, 2 * radius + 200.f);
            cascadeShadowVP[i]            = shadowMapProjection * shadowMapView;
            m_GlobalShaderData.cascadeSplits[i] = {-camera.zNear - zFarPct * (camera.zFar - camera.zNear),
                                                   lastTransitionPct * (camera.zFar - camera.zNear)};
            m_GlobalShaderData.cascadeSphereInfo[i] =
                glm::vec4(cascadeBoundingSphere.m_Center, cascadeBoundingSphere.m_Radius);
        }

        m_GlobalShaderData.lightVPCascade0 = cascadeShadowVP[0];
        m_GlobalShaderData.lightVPCascade1 = cascadeShadowVP[1];
        m_GlobalShaderData.lightVPCascade2 = cascadeShadowVP[2];
        m_GlobalShaderData.lightVPCascade3 = cascadeShadowVP[3];
    }

    void Renderer::DrawShadowMap(FrameGraph& fg)
    {
        struct PrepareShadowPassData
        {
            FrameGraphResourceHandle<FrameGraphTexture> shadow;
        };
        // prepare shadow render target
        auto prepareShadowPass = fg.AddPass<PrepareShadowPassData>(
            "prepareShadow",
            [engine = m_Engine](FrameGraph* fg, PassNode* passNode, PrepareShadowPassData* passData) {
                std::pair<uint32_t, uint32_t> windowDimension = engine->GetWindowDimension();

                TextureDescription desc {};
                desc.width     = 2048;
                desc.height    = 2048;
                desc.depth     = 4;
                desc.levels    = 1;
                desc.samples   = 1;
                desc.format    = TextureFormat::DEPTH24_STENCIL8;
                desc.usage     = TextureUsageBits::DepthStencilAttachment | TextureUsageBits::Sampled;
                desc.sampler   = SamplerType::Sampler2DArray;
                desc.pipelines = PipelineTypeBits::Graphics;

                passData->shadow = fg->CreateTexture(desc);

                passNode->SideEffect();
            },
            [](FrameGraph* fg, PrepareShadowPassData* m_Data, PassRenderTarget rt) {});
        fg.GetBlackboard().Set("shadow", prepareShadowPass->GetData()->shadow);

        // 4x cascade
        for (uint32_t i = 0; i < 4; i++)
        {
            struct ShadowPassData
            {
                FrameGraphResourceHandle<FrameGraphTexture> output;
                uint32_t                                    cascadeIndex;
            };

            auto shadowPass = fg.AddPass<ShadowPassData>(
                "shadow cascade-" + i,
                [engine = m_Engine, shadowTexture = prepareShadowPass->GetData()->shadow, i = i](
                    FrameGraph* fg, PassNode* passNode, ShadowPassData* passData) {
                    FrameGraphTexture::SubresourceDescriptor sub {};
                    sub.layer              = i;
                    sub.level              = 0;
                    passData->output       = fg->CreateSubresource(shadowTexture, sub);
                    passData->cascadeIndex = i;

                    FrameGraphRenderTargetDescriptor rtDesc {};
                    rtDesc.useDepth            = true;
                    rtDesc.depthStencil.handle = passData->output;

                    fg->Write(passNode, passData->output, TextureUsageBits::DepthStencilAttachment);
                    fg->SetRenderTarget(passNode, rtDesc);

                    // passNode->SideEffect();
                },
                [&](FrameGraph* fg, ShadowPassData* m_Data, PassRenderTarget rt) {
                    // set viewport scissor
                    m_Driver->SetViewportScissor({0, 0, (int)m_ShadowMapResolution, (int)m_ShadowMapResolution},
                                                 {0, 0, m_ShadowMapResolution, m_ShadowMapResolution});
                    // bind render target
                    m_Driver->BeginRenderPass(rt.rt);
                    m_Driver->BindShaderSet(m_Engine->GetShaderSet("shadow")->GetHandle());
                    m_Driver->SetRasterState({Culling::FrontFace, FrontFace::CounterClockwise, true, true});

                    auto handle = m_GlobalRingBuffer->GetHandle();
                    m_Driver->BindBuffer(m_GlobalRingBuffer->GetHandle(), 0, 0, BufferUsageBits::StorageDynamic);
                    for (auto& unit : m_SceneRenderUnit)
                    {

                        struct ShadowMapConstant
                        {
                            glm::mat4 world;
                            uint32_t  cascadeIndex;
                        };

                        ShadowMapConstant sc {unit.transform, m_Data->cascadeIndex};
                        // bind shader. this will automatically create a new render pipeline if there's no
                        // pipeline with same config exists. this would also bind the pipeline
                        m_Driver->BindVertexBuffer(unit.vertex);
                        m_Driver->BindIndexBuffer(unit.index);
                        m_Driver->BindConstantBuffer(0, sizeof(ShadowMapConstant), ShaderStageBits::Vertex, &sc);

                        m_Driver->DrawIndexed(unit.vertexOffset, unit.indexOffset, unit.indexCount);
                    }
                    m_Driver->EndRenderPass(rt.rt);
                });
        }
    }

    void Renderer::DrawForward(FrameGraph& fg)
    {

        struct ColorPassData
        {
            FrameGraphResourceHandle<FrameGraphTexture> shadow;
            FrameGraphResourceHandle<FrameGraphTexture> color;
            FrameGraphResourceHandle<FrameGraphTexture> depthStencil;
        };

        auto colorPass = fg.AddPass<ColorPassData>(
            "color pass",
            [engine = m_Engine](FrameGraph* fg, PassNode* passNode, ColorPassData* passData) {
                passData->shadow = fg->GetBlackboard().Get("shadow");

                std::pair<uint32_t, uint32_t> windowDimension = engine->GetWindowDimension();

                TextureDescription colorDesc {};
                colorDesc.width     = windowDimension.first;
                colorDesc.height    = windowDimension.second;
                colorDesc.depth     = 1;
                colorDesc.levels    = 1;
                colorDesc.samples   = 1;
                colorDesc.format    = TextureFormat::RGBA16_SRGB;
                colorDesc.usage     = TextureUsageBits::ColorAttachment | TextureUsageBits::Sampled;
                colorDesc.sampler   = SamplerType::Sampler2D;
                colorDesc.pipelines = PipelineTypeBits::Graphics | PipelineTypeBits::Compute;

                TextureDescription dsDesc {};
                dsDesc.width     = windowDimension.first;
                dsDesc.height    = windowDimension.second;
                dsDesc.depth     = 1;
                dsDesc.levels    = 1;
                dsDesc.samples   = 1;
                dsDesc.format    = TextureFormat::DEPTH24_STENCIL8;
                dsDesc.usage     = TextureUsageBits::DepthStencilAttachment;
                dsDesc.sampler   = SamplerType::Sampler2D;
                dsDesc.pipelines = PipelineTypeBits::Graphics;

                passData->color        = fg->CreateTexture(colorDesc, false);
                passData->depthStencil = fg->CreateTexture(dsDesc);

                fg->Write(passNode, passData->color, TextureUsageBits::ColorAttachment);
                fg->Write(passNode, passData->depthStencil, TextureUsageBits::DepthStencilAttachment);
                fg->Read(passNode, passData->shadow, TextureUsageBits::SampledDepthStencil);

                FrameGraphRenderTargetDescriptor rtDesc {};
                rtDesc.color.push_back({passData->color, true, true});
                rtDesc.depthStencil = {passData->depthStencil, true, false};
                rtDesc.useDepth     = true;
                rtDesc.present      = false;

                fg->SetRenderTarget(passNode, rtDesc);

                // passNode->SideEffect();

                fg->GetBlackboard().Set("colorOutput", passData->color);
            },
            [&](FrameGraph* fg, ColorPassData* m_Data, PassRenderTarget rt) {
                auto dimension = m_Engine->GetWindowDimension();
                m_Driver->SetViewportScissor({0, dimension.second, (int)dimension.first, -(int)dimension.second},
                                             {0, 0, dimension.first, dimension.second});
                // bind render target
                m_Driver->BeginRenderPass(rt.rt);
                m_Driver->SetRasterState({Culling::BackFace, FrontFace::CounterClockwise, true, true});
                auto handle = m_GlobalRingBuffer->GetHandle();
                m_Driver->BindBuffer(m_GlobalRingBuffer->GetHandle(), 0, 0, BufferUsageBits::StorageDynamic);
                auto shadowMap = static_cast<VirtualResource*>(fg->GetResource(m_Data->shadow))->GetRHITexture();
                m_Driver->BindTexture(shadowMap, 0, 2, TextureUsageBits::Sampled);
                m_Driver->BindTexture(m_Engine->GetDefaultSkybox()->GetHandle(), 0, 1, TextureUsageBits::Sampled);
                for (auto& unit : m_SceneRenderUnit)
                {
                    unit.material->Bind(m_Driver);
                    m_Driver->BindConstantBuffer(0, sizeof(glm::mat4), ShaderStageBits::Vertex, &unit.transform);
                    m_Driver->BindVertexBuffer(unit.vertex);
                    m_Driver->BindIndexBuffer(unit.index);

                    m_Driver->DrawIndexed(unit.vertexOffset, unit.indexOffset, unit.indexCount);
                }

                // skybox

                m_Driver->BindShaderSet(m_Engine->GetShaderSet("skybox")->GetHandle());
                m_Driver->BindTexture(m_Engine->GetDefaultSkybox()->GetHandle(), 0, 1, TextureUsageBits::Sampled);
                auto skyboxMesh = m_Engine->GetSkyboxMesh();
                auto meshlet    = skyboxMesh->GetSubmeshes()[0];
                m_Driver->SetRasterState({Culling::FrontFace, FrontFace::CounterClockwise, true, true});
                m_Driver->BindVertexBuffer(skyboxMesh->GetVertexBuffer());
                m_Driver->BindIndexBuffer(skyboxMesh->GetIndexBuffer());
                m_Driver->DrawIndexed(meshlet.baseVertex, meshlet.baseIndex, meshlet.indexCount);
                m_Driver->EndRenderPass(rt.rt);
            });
    }

    void Renderer::DrawResolve(FrameGraph& fg)
    {
        struct ResolvePassData
        {
            FrameGraphResourceHandle<FrameGraphTexture> input;
            FrameGraphResourceHandle<FrameGraphTexture> color;
        };

        auto pass = fg.AddPass<ResolvePassData>(
            "resolve",
            [engine = m_Engine](FrameGraph* fg, PassNode* passNode, ResolvePassData* passData) {
                std::pair<uint32_t, uint32_t> windowDimension = engine->GetWindowDimension();

                TextureDescription colorDesc {};
                colorDesc.width     = windowDimension.first;
                colorDesc.height    = windowDimension.second;
                colorDesc.depth     = 1;
                colorDesc.levels    = 1;
                colorDesc.samples   = 1;
                colorDesc.format    = TextureFormat::RGBA8_UNORM;
                colorDesc.usage     = TextureUsageBits::ColorAttachment | TextureUsageBits::Sampled;
                colorDesc.sampler   = SamplerType::Sampler2D;
                colorDesc.pipelines = PipelineTypeBits::Graphics | PipelineTypeBits::Compute;

                passData->color = fg->CreateTexture(colorDesc, true);
                passData->input = fg->GetBlackboard().Get("postCompute");

                fg->Write(passNode, passData->color, TextureUsageBits::ColorAttachment);
                fg->Read(passNode, passData->input, TextureUsageBits::Sampled);

                FrameGraphRenderTargetDescriptor rtDesc {};
                rtDesc.color.push_back({passData->color, true, true});
                rtDesc.useDepth = false;
                rtDesc.present  = true;

                passNode->SideEffect();

                fg->SetRenderTarget(passNode, rtDesc);
            },
            [engine = m_Engine, driver = m_Driver](FrameGraph* fg, ResolvePassData* m_Data, PassRenderTarget rt) {
                auto dimension  = engine->GetWindowDimension();
                auto colorInput = static_cast<VirtualResource*>(fg->GetResource(m_Data->input))->GetRHITexture();

                driver->SetViewportScissor({0, 0, (int)dimension.first, (int)dimension.second},
                                           {0, 0, dimension.first, dimension.second});
                driver->SetRasterState({Culling::None, FrontFace::CounterClockwise, false, false});
                driver->BindTexture(colorInput, 0, 0, TextureUsageBits::Sampled);
                // bind shader
                driver->BindShaderSet(engine->GetShaderSet("resolve")->GetHandle());
                driver->BeginRenderPass(rt.rt);
                driver->Draw(3, 0);
                driver->EndRenderPass(rt.rt);
            });
    }

    void Renderer::DispatchPostProcessingCompute(FrameGraph& fg)
    {
        struct PostProcessingComputePassData
        {
            FrameGraphResourceHandle<FrameGraphTexture> input;
            FrameGraphResourceHandle<FrameGraphTexture> storage;
        };
        auto computePass = fg.AddPass<PostProcessingComputePassData>(
            "post compute",
            [engine = m_Engine](FrameGraph* fg, PassNode* passNode, PostProcessingComputePassData* data) {
                data->input = fg->GetBlackboard().Get("colorOutput");

                std::pair<uint32_t, uint32_t> windowDimension = engine->GetWindowDimension();

                TextureDescription desc {};
                desc.width     = windowDimension.first;
                desc.height    = windowDimension.second;
                desc.depth     = 1;
                desc.levels    = 1;
                desc.samples   = 1;
                desc.format    = TextureFormat::RGBA16_SRGB;
                desc.usage     = TextureUsageBits::Storage | TextureUsageBits::Sampled;
                desc.sampler   = SamplerType::Sampler2D;
                desc.pipelines = PipelineTypeBits::Graphics | PipelineTypeBits::Compute;

                data->storage = fg->CreateTexture(desc);

                fg->Write(passNode, data->storage, TextureUsageBits::Storage);
                fg->Read(passNode, data->input, TextureUsageBits::Sampled);
                fg->GetBlackboard().Set("postCompute", data->storage);
            },
            [engine = m_Engine,
             driver = m_Driver](FrameGraph* fg, PostProcessingComputePassData* m_Data, PassRenderTarget rt) {
                auto dimension = engine->GetWindowDimension();
                auto input     = static_cast<VirtualResource*>(fg->GetResource(m_Data->input))->GetRHITexture();
                auto storage   = static_cast<VirtualResource*>(fg->GetResource(m_Data->storage))->GetRHITexture();

                driver->BindTexture(storage, 0, 0, TextureUsageBits::Storage);
                driver->BindTexture(input, 0, 1, TextureUsageBits::Sampled);
                // bind shader
                driver->BindShaderSet(engine->GetShaderSet("postCompute")->GetHandle());

                uint32_t groupCountX = dimension.first % 16 == 0 ? dimension.first / 16 : dimension.first / 16 + 1;
                uint32_t groupCountY = dimension.second % 16 == 0 ? dimension.second / 16 : dimension.second / 16 + 1;
                driver->Dispatch(groupCountX, groupCountY, 1);
            });
    }
} // namespace Zephyr