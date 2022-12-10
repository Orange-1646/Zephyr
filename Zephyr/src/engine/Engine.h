#pragma once
#include "pch.h"

#include "core/macro.h"
#include "resource/Buffer.h"
#include "resource/Material.h"
#include "resource/Mesh.h"
#include "resource/ResourceManager.h"
#include "resource/ShaderSet.h"
#include "resource/Texture.h"
#include "rhi/RHIEnums.h"

#define ZEPHYR_STR(x) ZEPHYR_STR_I(x)
#define ZEPHYR_STR_I(x) #x

namespace Zephyr
{
    class Window;
    class Driver;
    class Scene;
    class View;
    class Mesh;
    using SceneHandle = uint32_t;
    using ViewHandle  = uint32_t;

    struct EngineDescription
    {
        uint32_t    width;
        uint32_t    height;
        DriverType  driver;
        std::string title;
        bool        fulscreen;
        bool        resize;
        bool        debug;
    };

    /*
        Zephyr engine entry point
        All frontend resources are managed by engine instance through resourceManager
    */
    class Engine final
    {
    public:
        using SetupCallback = std::function<void(Engine*)>;
        static Engine* Create(const EngineDescription& desc);
        static bool    Destroy(Engine* engine);

        DISALE_COPY_AND_MOVE(Engine);

        inline Driver* GetDriver() { return m_Driver; }

        void            Run(SetupCallback&& setup);
        inline uint64_t GetFrame() { return m_FrameCount; }

        Scene* CreateScene(const std::string& debugName);

        // resource creation
        Texture*                      CreateTexture(const TextureDescription& desc);
        Buffer*                       CreateBuffer(const BufferDescription& desc);
        Mesh*                         CreateMesh(const BoxMeshDescription& desc);
        Mesh*                         CreateMesh(const std::string& path);
        std::pair<uint32_t, uint32_t> GetWindowDimension();
        ShaderSet*                    GetShaderSet(const std::string& name);
        Texture*                      GetDefaultSkybox();
        Mesh*                         GetSkyboxMesh();
        MaterialInstance*             CreateMaterial(ShadingModel shading);

    private:
        Engine(const EngineDescription& desc);
        ~Engine();

        float GetDeltaTime();

    private:
        EngineDescription m_Description;
        uint64_t          m_FrameCount = 0;

        Window* m_Window;
        Driver* m_Driver;

        ResourceManager m_ResourceManager;

        std::unordered_map<std::string, Scene*> m_Scenes;

        bool m_ShouldClose = false;

        std::chrono::steady_clock::time_point m_LastTimePoint = std::chrono::steady_clock::now();
    };
} // namespace Zephyr