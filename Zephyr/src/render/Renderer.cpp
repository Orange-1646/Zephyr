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
        //DrawForward(fg);
        DrawDeferred(fg);
        DispatchBloomCompute(fg);
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
                fg->GetBlackboard().Set("shadow", passData->shadow);

                passNode->SideEffect();
            },
            [](FrameGraph* fg, PrepareShadowPassData* m_Data, PassRenderTarget rt) {});

        // 4x cascade
        for (uint32_t i = 0; i < 4; i++)
        {
            struct ShadowPassData
            {
                FrameGraphResourceHandle<FrameGraphTexture> output;
                uint32_t                                    cascadeIndex;
            };

            auto shadowPass = fg.AddPass<ShadowPassData>(
                "shadow cascade " + std::to_string(i),
                [engine = m_Engine, shadowTexture = prepareShadowPass->GetData()->shadow, i = i](
                    FrameGraph* fg, PassNode* passNode, ShadowPassData* passData) {
                    FrameGraphTexture::SubresourceDescriptor sub {};
                    sub.baseLayer          = i;
                    sub.layerCount         = 1;
                    sub.baseLevel          = 0;
                    sub.levelCount         = 1;
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

                auto shadowResource = static_cast<VirtualResource*>(fg->GetResource(m_Data->shadow));

                auto shadowMap = shadowResource->GetRHITexture();

                m_Driver->BindTexture(shadowMap, 0, 2, TextureUsageBits::SampledDepthStencil);
                m_Driver->BindTexture(m_Engine->GetDefaultSkybox()->GetHandle(), 0, 1, TextureUsageBits::Sampled);
                m_Driver->BindShaderSet(m_Engine->GetShaderSet("lit")->GetHandle());
                for (auto& unit : m_SceneRenderUnit)
                {
                    unit.material->Bind(m_Driver);
                    m_Driver->BindConstantBuffer(0, sizeof(glm::mat4), ShaderStageBits::Vertex, &unit.transform);
                    m_Driver->BindVertexBuffer(unit.vertex);
                    m_Driver->BindIndexBuffer(unit.index);

                    m_Driver->DrawIndexed(unit.vertexOffset, unit.indexOffset, unit.indexCount);
                }

                // skybox
                if (true)
                {
                    m_Driver->BindShaderSet(m_Engine->GetShaderSet("skybox")->GetHandle());
                    m_Driver->BindTexture(m_Engine->GetDefaultSkybox()->GetHandle(), 0, 1, TextureUsageBits::Sampled);
                    auto skyboxMesh = m_Engine->GetSkyboxMesh();
                    auto meshlet    = skyboxMesh->GetSubmeshes()[0];
                    m_Driver->SetRasterState({Culling::FrontFace, FrontFace::CounterClockwise, true, true});
                    m_Driver->BindVertexBuffer(skyboxMesh->GetVertexBuffer());
                    m_Driver->BindIndexBuffer(skyboxMesh->GetIndexBuffer());
                    m_Driver->DrawIndexed(meshlet.baseVertex, meshlet.baseIndex, meshlet.indexCount);
                }
                m_Driver->EndRenderPass(rt.rt);
            });
    }

    void Renderer::DrawDeferred(FrameGraph& fg)
    {
        struct GeometryData
        {
            // albedo metalness
            FrameGraphResourceHandle<FrameGraphTexture> color0;
            // normal roughness
            FrameGraphResourceHandle<FrameGraphTexture> color1;
            // position occlusion
            FrameGraphResourceHandle<FrameGraphTexture> color2;
            // emission
            FrameGraphResourceHandle<FrameGraphTexture> color3;
            FrameGraphResourceHandle<FrameGraphTexture> depthStencil;
        };
        auto geometryPass = fg.AddPass<GeometryData>(
            "geometry",
            [engine = m_Engine](FrameGraph* fg, PassNode* node, GeometryData* data) {
                std::pair<uint32_t, uint32_t> windowDimension = engine->GetWindowDimension();

                // we need 16-bits color attachments for hdr color output && emission && position
                TextureDescription colorDesc {};
                colorDesc.width     = windowDimension.first;
                colorDesc.height    = windowDimension.second;
                colorDesc.depth     = 1;
                colorDesc.levels    = 1;
                colorDesc.samples   = 1;
                colorDesc.format    = TextureFormat::RGBA16_SRGB;
                colorDesc.usage     = TextureUsageBits::ColorAttachment | TextureUsageBits::Sampled;
                colorDesc.sampler   = SamplerType::Sampler2D;
                colorDesc.pipelines = PipelineTypeBits::Graphics;
                data->color0        = fg->CreateTexture(colorDesc, false);
                data->color3        = fg->CreateTexture(colorDesc, false);
                data->color2        = fg->CreateTexture(colorDesc, false);

                // for rest of the attachments, rba8 will do
                colorDesc.format = TextureFormat::RGBA8_UNORM;
                data->color1     = fg->CreateTexture(colorDesc);

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

                data->depthStencil = fg->CreateTexture(dsDesc);

                fg->Write(node, data->color0, TextureUsageBits::ColorAttachment);
                fg->Write(node, data->color1, TextureUsageBits::ColorAttachment);
                fg->Write(node, data->color2, TextureUsageBits::ColorAttachment);
                fg->Write(node, data->color3, TextureUsageBits::ColorAttachment);

                fg->Write(node, data->depthStencil, TextureUsageBits::DepthStencilAttachment);

                auto& blackboard = fg->GetBlackboard();
                blackboard.Set("albedoMetalness", data->color0);
                blackboard.Set("normalRoughness", data->color1);
                blackboard.Set("positionOcclusion", data->color2);
                blackboard.Set("emission", data->color3);
                blackboard.Set("geometryDepthStencil", data->depthStencil);

                FrameGraphRenderTargetDescriptor rtDesc {};
                rtDesc.color.push_back({data->color0, true, true});
                rtDesc.color.push_back({data->color1, true, true});
                rtDesc.color.push_back({data->color2, true, true});
                rtDesc.color.push_back({data->color3, true, true});
                rtDesc.depthStencil = {data->depthStencil, true, true};
                rtDesc.useDepth     = true;
                rtDesc.present      = false;

                fg->SetRenderTarget(node, rtDesc);

                node->SideEffect();
            },
            [engine = m_Engine, driver = m_Driver, self = this](
                FrameGraph* fg, GeometryData* data, PassRenderTarget rt) {
                auto c0 = static_cast<VirtualResource*>(fg->GetResource(data->color0))->GetRHITexture();
                auto c1 = static_cast<VirtualResource*>(fg->GetResource(data->color1))->GetRHITexture();
                auto c2 = static_cast<VirtualResource*>(fg->GetResource(data->color2))->GetRHITexture();
                auto c3 = static_cast<VirtualResource*>(fg->GetResource(data->color3))->GetRHITexture();

                auto dimension = engine->GetWindowDimension();
                driver->SetViewportScissor({0, 0, (int)dimension.first, (int)dimension.second},
                                           {0, 0, dimension.first, dimension.second});
                // bind render target
                driver->BeginRenderPass(rt.rt);
                driver->SetRasterState({Culling::FrontFace, FrontFace::CounterClockwise, true, true});
                auto handle = self->m_GlobalRingBuffer->GetHandle();
                driver->BindBuffer(self->m_GlobalRingBuffer->GetHandle(), 0, 0, BufferUsageBits::StorageDynamic);

                driver->BindShaderSet(engine->GetShaderSet("deferredGeometry")->GetHandle());
                for (auto& unit : self->m_SceneRenderUnit)
                {
                    unit.material->Bind(driver);
                    driver->BindConstantBuffer(0, sizeof(glm::mat4), ShaderStageBits::Vertex, &unit.transform);
                    driver->BindVertexBuffer(unit.vertex);
                    driver->BindIndexBuffer(unit.index);

                    driver->DrawIndexed(unit.vertexOffset, unit.indexOffset, unit.indexCount);
                }
                driver->EndRenderPass(rt.rt);
            });

        struct LightingData
        {
            // albedo roughness
            FrameGraphResourceHandle<FrameGraphTexture> color0;
            // normal metalness
            FrameGraphResourceHandle<FrameGraphTexture> color1;
            // position occlusion
            FrameGraphResourceHandle<FrameGraphTexture> color2;
            // emission
            FrameGraphResourceHandle<FrameGraphTexture> color3;
            // shadowmap
            FrameGraphResourceHandle<FrameGraphTexture> shadow;
            // color output
            FrameGraphResourceHandle<FrameGraphTexture> color;
            // disable writes and reuse depthstencil from geometry pass(mainly for skybox rendering)
            FrameGraphResourceHandle<FrameGraphTexture> depthStencil;
        };

        auto lightingPass = fg.AddPass<LightingData>(
            "lighting",
            [engine = m_Engine](FrameGraph* fg, PassNode* node, LightingData* data) {
                std::pair<uint32_t, uint32_t> windowDimension = engine->GetWindowDimension();
                auto&                         blackboard      = fg->GetBlackboard();
                data->color0                                  = blackboard.Get("albedoMetalness");
                data->color1                                  = blackboard.Get("normalRoughness");
                data->color2                                  = blackboard.Get("positionOcclusion");
                data->color3                                  = blackboard.Get("emission");
                data->shadow                                  = blackboard.Get("shadow");
                data->depthStencil                            = blackboard.Get("geometryDepthStencil");

                // create color output. we dont need depth attachment for the lighting pass
                TextureDescription desc {};
                desc.width     = windowDimension.first;
                desc.height    = windowDimension.second;
                desc.depth     = 1;
                desc.levels    = 1;
                desc.samples   = 1;
                desc.format    = TextureFormat::RGBA16_SRGB;
                desc.usage     = TextureUsageBits::ColorAttachment | TextureUsageBits::Sampled;
                desc.sampler   = SamplerType::Sampler2D;
                desc.pipelines = PipelineTypeBits::Graphics;

                data->color = fg->CreateTexture(desc);

                fg->Read(node, data->color0, TextureUsageBits::Sampled);
                fg->Read(node, data->color1, TextureUsageBits::Sampled);
                fg->Read(node, data->color2, TextureUsageBits::Sampled);
                fg->Read(node, data->color3, TextureUsageBits::Sampled);
                fg->Read(node, data->shadow, TextureUsageBits::SampledDepthStencil);
                fg->Read(node, data->depthStencil, TextureUsageBits::DepthStencilAttachment);
                fg->Write(node, data->color, TextureUsageBits::ColorAttachment);

                FrameGraphRenderTargetDescriptor rtDesc {};
                rtDesc.color.push_back({data->color, true, true});
                rtDesc.depthStencil = {data->depthStencil, false, false};
                rtDesc.useDepth = true;
                rtDesc.present  = false;

                fg->SetRenderTarget(node, rtDesc);

                fg->GetBlackboard().Set("colorOutput", data->color);

                node->SideEffect();
            },
            [engine = m_Engine, driver = m_Driver, self = this](
                FrameGraph* fg, LightingData* data, PassRenderTarget rt) {
                auto& blackboard = fg->GetBlackboard();
                auto dimension = engine->GetWindowDimension();

                
                driver->BindShaderSet(engine->GetShaderSet("deferredLighting")->GetHandle());
                driver->SetViewportScissor({0, 0, (int)dimension.first, (int)dimension.second},
                                           {0, 0, dimension.first, dimension.second});
                // bind render target
                driver->BeginRenderPass(rt.rt);
                driver->SetRasterState({Culling::None, FrontFace::CounterClockwise, false, false});
                auto handle = self->m_GlobalRingBuffer->GetHandle();
                driver->BindBuffer(self->m_GlobalRingBuffer->GetHandle(), 0, 0, BufferUsageBits::StorageDynamic);
                // bind g-buffers

                auto shadowMap = static_cast<VirtualResource*>(fg->GetResource(data->shadow))->GetRHITexture();
                auto color0 = static_cast<VirtualResource*>(fg->GetResource(data->color0))->GetRHITexture();
                auto color1 = static_cast<VirtualResource*>(fg->GetResource(data->color1))->GetRHITexture();
                auto color2 = static_cast<VirtualResource*>(fg->GetResource(data->color2))->GetRHITexture();
                auto color3 = static_cast<VirtualResource*>(fg->GetResource(data->color3))->GetRHITexture();

                driver->BindTexture(shadowMap, 0, 1, TextureUsageBits::SampledDepthStencil);
                driver->BindTexture(color0, 1, 0, TextureUsageBits::Sampled);
                driver->BindTexture(color1, 1, 1, TextureUsageBits::Sampled);
                driver->BindTexture(color2, 1, 2, TextureUsageBits::Sampled);
                driver->BindTexture(color3, 1, 3, TextureUsageBits::Sampled);

                driver->Draw(3, 0);
                if (true)
                {
                    driver->SetRasterState({Culling::BackFace, FrontFace::CounterClockwise, true, false});
                    driver->BindShaderSet(engine->GetShaderSet("skybox")->GetHandle());
                    driver->BindTexture(engine->GetDefaultSkybox()->GetHandle(), 0, 1, TextureUsageBits::Sampled);
                    auto skyboxMesh = engine->GetSkyboxMesh();
                    auto meshlet    = skyboxMesh->GetSubmeshes()[0];
                    driver->BindVertexBuffer(skyboxMesh->GetVertexBuffer());
                    driver->BindIndexBuffer(skyboxMesh->GetIndexBuffer());
                    driver->DrawIndexed(meshlet.baseVertex, meshlet.baseIndex, meshlet.indexCount);
                }

                driver->EndRenderPass(rt.rt);
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

                driver->SetViewportScissor({0, dimension.second, (int)dimension.first, -(int)dimension.second},
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
                // data->input = fg->GetBlackboard().Get("bloomDownsampleResult7");
                data->input = fg->GetBlackboard().Get("bloomComposite");
                //  data->input  = fg->GetBlackboard().Get("colorOutput");

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
                auto dimension  = engine->GetWindowDimension();
                auto input      = static_cast<VirtualResource*>(fg->GetResource(m_Data->input))->GetRHITexture();
                auto inputRange = static_cast<VirtualResource*>(fg->GetResource(m_Data->input))->GetViewRange();
                auto storage    = static_cast<VirtualResource*>(fg->GetResource(m_Data->storage))->GetRHITexture();

                driver->BindTexture(storage, 0, 0, TextureUsageBits::Storage);
                driver->BindTexture(input, inputRange, 0, 1, TextureUsageBits::Sampled);
                // bind shader
                driver->BindShaderSet(engine->GetShaderSet("postCompute")->GetHandle());

                uint32_t groupCountX = dimension.first % 16 == 0 ? dimension.first / 16 : dimension.first / 16 + 1;
                uint32_t groupCountY = dimension.second % 16 == 0 ? dimension.second / 16 : dimension.second / 16 + 1;
                driver->Dispatch(groupCountX, groupCountY, 1);
            });
    }

    void Renderer::DispatchBloomCompute(FrameGraph& fg)
    {
        struct BloomComputeData
        {
            FrameGraphResourceHandle<FrameGraphTexture> downsample;
            FrameGraphResourceHandle<FrameGraphTexture> upsample;
            FrameGraphResourceHandle<FrameGraphTexture> composite;
            float                                       threshold;
            float                                       downsampleCount;
        };
        // prepare resource
        auto prepareBloomPass = fg.AddPass<BloomComputeData>(
            "prepare bloom compute",
            [count  = m_BloomDownsampleCount,
             engine = m_Engine](FrameGraph* fg, PassNode* node, BloomComputeData* data) {
                std::pair<uint32_t, uint32_t> windowDimension = engine->GetWindowDimension();

                TextureDescription desc {};
                desc.width     = windowDimension.first;
                desc.height    = windowDimension.second;
                desc.depth     = 1;
                desc.levels    = count + 1;
                desc.format    = TextureFormat::RGBA16_SRGB;
                desc.pipelines = PipelineTypeBits::Compute;
                desc.sampler   = SamplerType::Sampler2D;
                desc.usage     = TextureUsageBits::Storage | TextureUsageBits::Sampled;
                desc.samples   = 1;

                // we use image mipchains to store downsample and upsample result
                data->downsample = fg->CreateTexture(desc);

                desc.levels--;
                data->upsample = fg->CreateTexture(desc);
                // we dont need mipmaps for the composite image
                desc.levels = 1;

                data->composite = fg->CreateTexture(desc);

                node->SideEffect();

                auto& blackboard = fg->GetBlackboard();
                blackboard.Set("bloomDownsample", data->downsample);
                blackboard.Set("bloomUpsample", data->upsample);
                blackboard.Set("bloomComposite", data->composite);
            },
            [](FrameGraph* fg, BloomComputeData* data, PassRenderTarget) {});

        // prefilter
        struct PrefilterData
        {
            FrameGraphResourceHandle<FrameGraphTexture> hdrInput;
            FrameGraphResourceHandle<FrameGraphTexture> prefilterTarget;
        };

        auto prefilterPass = fg.AddPass<PrefilterData>(
            "bloom compute prefilter",
            [](FrameGraph* fg, PassNode* node, PrefilterData* data) {
                auto downsampleImage = fg->GetBlackboard().Get("bloomDownsample");

                // filter hdr color input and store in downsample mip 0
                data->hdrInput = fg->GetBlackboard().Get("colorOutput");

                FrameGraphTexture::SubresourceDescriptor sub {};
                sub.baseLayer  = 0;
                sub.layerCount = 1;
                sub.baseLevel  = 0;
                sub.levelCount = 1;

                data->prefilterTarget = fg->CreateSubresource(downsampleImage, sub);

                fg->Read(node, data->hdrInput, TextureUsageBits::Sampled);
                fg->Write(node, data->prefilterTarget, TextureUsageBits::Storage);

                auto  a          = static_cast<VirtualResource*>(fg->GetResource(data->prefilterTarget));
                auto& blackboard = fg->GetBlackboard();
                blackboard.Set("bloomPrefilterResult", data->prefilterTarget);
            },
            [engine = m_Engine, driver = m_Driver](FrameGraph* fg, PrefilterData* data, PassRenderTarget) {
                struct BloomPrefilterConstant
                {
                    glm::vec4 data;
                };

                BloomPrefilterConstant pc = {{1., .0, 0., 0.}};

                auto dimension = engine->GetWindowDimension();
                driver->BindShaderSet(engine->GetShaderSet("bloomPrefilter")->GetHandle());

                auto input   = static_cast<VirtualResource*>(fg->GetResource(data->hdrInput))->GetRHITexture();
                auto storage = static_cast<VirtualResource*>(fg->GetResource(data->prefilterTarget))->GetRHITexture();
                auto storageRange =
                    static_cast<VirtualResource*>(fg->GetResource(data->prefilterTarget))->GetViewRange();
                driver->BindTexture(storage, storageRange, 0, 0, TextureUsageBits::Storage);
                driver->BindTexture(input, 0, 1, TextureUsageBits::Sampled);
                driver->BindConstantBuffer(0, sizeof(glm::vec4), ShaderStageBits::Compute, &pc);

                uint32_t groupCountX = dimension.first % 16 == 0 ? dimension.first / 16 : dimension.first / 16 + 1;
                uint32_t groupCountY = dimension.second % 16 == 0 ? dimension.second / 16 : dimension.second / 16 + 1;
                driver->Dispatch(groupCountX, groupCountY, 1);
            });
        //// downsample
        for (uint32_t i = 0; i < m_BloomDownsampleCount; i++)
        {
            struct DownSampleData
            {
                FrameGraphResourceHandle<FrameGraphTexture> input;
                FrameGraphResourceHandle<FrameGraphTexture> output;
            };

            auto downsamplePass = fg.AddPass<DownSampleData>(
                "downsample " + std::to_string(i),
                [i = i](FrameGraph* fg, PassNode* node, DownSampleData* data) {
                    auto& blackboard      = fg->GetBlackboard();
                    auto  downsampleImage = blackboard.Get("bloomDownsample");

                    if (i == 0)
                    {
                        data->input = blackboard.Get("bloomPrefilterResult");
                    }
                    else
                    {
                        data->input = blackboard.Get("bloomDownsampleResult" + std::to_string(i));
                    }

                    FrameGraphTexture::SubresourceDescriptor desc {};
                    desc.baseLayer  = 0;
                    desc.layerCount = 1;
                    desc.baseLevel  = i + 1;
                    desc.levelCount = 1;
                    data->output    = fg->CreateSubresource(downsampleImage, desc);

                    auto name = "bloomDownsampleResult" + std::to_string(i + 1);

                    blackboard.Set("bloomDownsampleResult" + std::to_string(i + 1), data->output);

                    fg->Read(node, data->input, TextureUsageBits::Sampled);
                    fg->Write(node, data->output, TextureUsageBits::Storage);

                    node->SideEffect();
                },
                [i, engine = m_Engine, driver = m_Driver](FrameGraph* fg, DownSampleData* data, PassRenderTarget) {
                    struct BloomDownsampleConst
                    {
                        float mip;
                    };

                    BloomDownsampleConst c {i};

                    auto dimension = engine->GetWindowDimension();
                    driver->BindShaderSet(engine->GetShaderSet("bloomDownsample")->GetHandle());

                    auto input      = static_cast<VirtualResource*>(fg->GetResource(data->input))->GetRHITexture();
                    auto inputRange = static_cast<VirtualResource*>(fg->GetResource(data->input))->GetViewRange();

                    auto output      = static_cast<VirtualResource*>(fg->GetResource(data->output))->GetRHITexture();
                    auto outputRange = static_cast<VirtualResource*>(fg->GetResource(data->output))->GetViewRange();

                    driver->BindTexture(output, outputRange, 0, 0, TextureUsageBits::Storage);
                    driver->BindTexture(input, inputRange, 0, 1, TextureUsageBits::Sampled);
                    driver->BindConstantBuffer(0, sizeof(float), ShaderStageBits::Compute, &c);

                    uint32_t groupCountX = dimension.first % 16 == 0 ? dimension.first / 16 : dimension.first / 16 + 1;
                    uint32_t groupCountY =
                        dimension.second % 16 == 0 ? dimension.second / 16 : dimension.second / 16 + 1;

                    auto x = groupCountX / pow(2, i + 1);
                    auto y = groupCountY / pow(2, i + 1);
                    driver->Dispatch(ceil(x), ceil(y), 1);
                    // driver->Dispatch(100, 100, 1);
                });
        }
        //// upsample
        for (uint32_t i = 0; i < m_BloomDownsampleCount; i++)
        {
            struct UpSampleData
            {
                FrameGraphResourceHandle<FrameGraphTexture> input0;
                FrameGraphResourceHandle<FrameGraphTexture> input1;
                FrameGraphResourceHandle<FrameGraphTexture> output;
            };

            // upsample needs one less mip level than downsample because for the first upsample we read directly from
            // downsample-last and downsample-last-1
            auto upsamplePass = fg.AddPass<UpSampleData>(
                "upsample " + std::to_string(i),
                [i = i, count = m_BloomDownsampleCount - 1](FrameGraph* fg, PassNode* node, UpSampleData* data) {
                    auto& blackboard    = fg->GetBlackboard();
                    auto  upsampleImage = blackboard.Get("bloomUpsample");

                    if (i == 0)
                    {
                        data->input0 = blackboard.Get("bloomDownsampleResult" + std::to_string(count + 1));
                    }
                    else
                    {
                        data->input0 = blackboard.Get("bloomUpsampleResult" + std::to_string(count - i + 1));
                    }
                    if (i == count)
                    {
                        data->input1 = blackboard.Get("bloomPrefilterResult");
                    }
                    else
                    {
                        data->input1 = blackboard.Get("bloomDownsampleResult" + std::to_string(count - i));
                    }

                    FrameGraphTexture::SubresourceDescriptor desc {};
                    desc.baseLayer  = 0;
                    desc.layerCount = 1;
                    desc.baseLevel  = count - i;
                    desc.levelCount = 1;

                    data->output = fg->CreateSubresource(upsampleImage, desc);

                    blackboard.Set("bloomUpsampleResult" + std::to_string(count - i), data->output);

                    fg->Read(node, data->input0, TextureUsageBits::Sampled);
                    fg->Read(node, data->input1, TextureUsageBits::Sampled);

                    fg->Write(node, data->output, TextureUsageBits::Storage);

                    node->SideEffect();
                },
                [count = m_BloomDownsampleCount - 1, i = i, engine = m_Engine, driver = m_Driver](
                    FrameGraph* fg, UpSampleData* data, PassRenderTarget) {
                    auto dimension = engine->GetWindowDimension();
                    driver->BindShaderSet(engine->GetShaderSet("bloomUpsample")->GetHandle());

                    auto input0      = static_cast<VirtualResource*>(fg->GetResource(data->input0))->GetRHITexture();
                    auto inputRange0 = static_cast<VirtualResource*>(fg->GetResource(data->input0))->GetViewRange();

                    auto input1      = static_cast<VirtualResource*>(fg->GetResource(data->input1))->GetRHITexture();
                    auto inputRange1 = static_cast<VirtualResource*>(fg->GetResource(data->input1))->GetViewRange();

                    auto output      = static_cast<VirtualResource*>(fg->GetResource(data->output))->GetRHITexture();
                    auto outputRange = static_cast<VirtualResource*>(fg->GetResource(data->output))->GetViewRange();

                    driver->BindTexture(output, outputRange, 0, 0, TextureUsageBits::Storage);
                    driver->BindTexture(input1, inputRange1, 0, 1, TextureUsageBits::Sampled);
                    driver->BindTexture(input0, inputRange0, 0, 2, TextureUsageBits::Sampled);

                    uint32_t groupCountX = dimension.first % 16 == 0 ? dimension.first / 16 : dimension.first / 16 + 1;
                    uint32_t groupCountY =
                        dimension.second % 16 == 0 ? dimension.second / 16 : dimension.second / 16 + 1;

                    auto x = groupCountX / pow(2, count - i);
                    auto y = groupCountY / pow(2, count - i);
                    driver->Dispatch(ceil(x), ceil(y), 1);
                });
        }
        //// final composite

        struct FinalCompositeData
        {
            FrameGraphResourceHandle<FrameGraphTexture> upsampleResult;
            FrameGraphResourceHandle<FrameGraphTexture> original;
            FrameGraphResourceHandle<FrameGraphTexture> output;
        };

        auto compositePass = fg.AddPass<FinalCompositeData>(
            "bloom composite",
            [engine = m_Engine](FrameGraph* fg, PassNode* node, FinalCompositeData* data) {
                std::pair<uint32_t, uint32_t> windowDimension = engine->GetWindowDimension();
                auto&                         blackboard      = fg->GetBlackboard();

                data->upsampleResult = blackboard.Get("bloomUpsampleResult0");
                data->original       = blackboard.Get("colorOutput");

                TextureDescription desc {};
                desc.width     = windowDimension.first;
                desc.height    = windowDimension.second;
                desc.depth     = 1;
                desc.levels    = 1;
                desc.format    = TextureFormat::RGBA16_SRGB;
                desc.pipelines = PipelineTypeBits::Compute | PipelineTypeBits::Graphics;
                desc.sampler   = SamplerType::Sampler2D;
                desc.usage     = TextureUsageBits::Storage | TextureUsageBits::Sampled;
                desc.samples   = 1;

                data->output = blackboard.Get("bloomComposite");
                // data->output = fg->CreateTexture(desc);

                fg->Read(node, data->upsampleResult, TextureUsageBits::Sampled);
                fg->Read(node, data->original, TextureUsageBits::Sampled);

                fg->Write(node, data->output, TextureUsageBits::Storage);

                node->SideEffect();
            },
            [engine = m_Engine, driver = m_Driver](FrameGraph* fg, FinalCompositeData* data, PassRenderTarget) {
                auto dimension = engine->GetWindowDimension();
                driver->BindShaderSet(engine->GetShaderSet("bloomComposite")->GetHandle());

                auto input0 = static_cast<VirtualResource*>(fg->GetResource(data->upsampleResult))->GetRHITexture();
                auto inputRange0 = static_cast<VirtualResource*>(fg->GetResource(data->upsampleResult))->GetViewRange();

                auto input1      = static_cast<VirtualResource*>(fg->GetResource(data->original))->GetRHITexture();
                auto inputRange1 = static_cast<VirtualResource*>(fg->GetResource(data->original))->GetViewRange();

                auto output      = static_cast<VirtualResource*>(fg->GetResource(data->output))->GetRHITexture();
                auto outputRange = static_cast<VirtualResource*>(fg->GetResource(data->output))->GetViewRange();

                driver->BindTexture(output, outputRange, 0, 0, TextureUsageBits::Storage);
                driver->BindTexture(input0, inputRange0, 0, 1, TextureUsageBits::Sampled);
                driver->BindTexture(input1, inputRange1, 0, 2, TextureUsageBits::Sampled);

                uint32_t groupCountX = dimension.first % 16 == 0 ? dimension.first / 16 : dimension.first / 16 + 1;
                uint32_t groupCountY = dimension.second % 16 == 0 ? dimension.second / 16 : dimension.second / 16 + 1;

                driver->Dispatch(groupCountX, groupCountY, 1);
            });
    }
} // namespace Zephyr