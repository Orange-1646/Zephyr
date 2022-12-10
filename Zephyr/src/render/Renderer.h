#pragma once
#include "RenderResourceManager.h"
#include "pch.h"
#include "render/Camera.h"
#include "render/Light.h"

namespace Zephyr
{

    class Engine;
    class Driver;
    class Mesh;
    class Buffer;
    class MaterialInstance;
    class FrameGraph;

    struct SceneRenderData
    {
        std::vector<Mesh*>     meshes;
        std::vector<glm::mat4> transforms;
        DirectionalLight       light;
        Camera                 camera;
    };

    struct SceneRenderUnit
    {
        Handle<RHIBuffer> vertex;
        Handle<RHIBuffer> index;
        uint32_t          vertexOffset;
        uint32_t          indexOffset;
        uint32_t          indexCount;
        glm::mat4         transform;
        MaterialInstance* material;
    };

    struct GlobalRenderShaderData
    {
        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 vp;
        glm::vec3 directionalLightDirection;
        float     _padding1;
        glm::vec3 directionalLightRadiance;
        float     _padding2;
        glm::vec3 eye;
        float     _padding3;
        glm::mat4 lightVPCascade0;
        glm::mat4 lightVPCascade1;
        glm::mat4 lightVPCascade2;
        glm::mat4 lightVPCascade3;
        glm::vec2 cascadeSplits[4];
        glm::vec4 cascadeSphereInfo[4];
    };

    class Renderer
    {
    public:
        Renderer(Engine* engine);
        void Shutdown();
        ~Renderer();

        void Render(const SceneRenderData& scene);

    private:
        void PrepareScene();
        void SetupGlobalRenderData();
        void PrepareCascadedShadowData();

        void DrawShadowMap(FrameGraph& fg);
        void DrawForward(FrameGraph& fg);
        void DrawResolve(FrameGraph& fg);

    private:
        Engine*         m_Engine;
        Driver*         m_Driver;
        SceneRenderData m_Scene;

        std::vector<SceneRenderUnit> m_SceneRenderUnit;

        Buffer*                m_GlobalRingBuffer;
        GlobalRenderShaderData m_GlobalShaderData = {};

        RenderResourceManager m_Manager;

        float    m_CascadeTransitionScale = .3;
        uint32_t m_ShadowMapResolution    = 2048;
    };
} // namespace Zephyr