#include "Engine.h"
#include "core/event/EventSystem.h"
#include "core/event/KeyboardEvent.h"
#include "platform/Path.h"
#include "platform/Window.h"
#include "rhi/Driver.h"
#include "scene/Scene.h"

namespace Zephyr
{
    Engine* Engine::Create(const EngineDescription& desc)
    {
        Path::SetRootDir(ZEPHYR_STR(ZEPHYR_ROOT_DIR));
        auto engine = new Engine(desc);

        return engine;
    }

    bool Engine::Destroy(Engine* engine)
    {
        delete engine;
        return true;
    }

    Engine::Engine(const EngineDescription& desc) : m_Description(desc), m_ResourceManager(this)
    {
        EventCenter::Init();
        // initialize rendering context
        m_Window = new Window({desc.width, desc.height, desc.title, desc.resize, desc.fulscreen});
        m_Window->Init();

        EventCenter::Register<KeyPressedEvent>([this](KeyPressedEvent& event) {
            switch (event.GetKeyCode())
            {
                case KeyCode::Escape:
                    this->m_ShouldClose = true;
            }
            return false;
        });

        m_Driver = Driver::Create(desc.driver, m_Window);

        m_ResourceManager.InitResources(m_Driver);
    }

    Engine::~Engine()
    {
        m_ResourceManager.Shutdown();
        delete m_Driver;
        delete m_Window;
        for (auto& scene : m_Scenes)
        {
            delete scene.second;
        }
    }

    float Engine::GetDeltaTime()
    {
        float delta;
        {
            std::chrono::steady_clock::time_point nowTimePoint = std::chrono::steady_clock::now();
            std::chrono::duration<float>          timespan     = nowTimePoint - m_LastTimePoint;

            delta = std::chrono::duration_cast<std::chrono::nanoseconds>(timespan).count();

            m_LastTimePoint = nowTimePoint;
        }
        return delta;
    }
    /*
        The setup function hooks up user-defined scene logic.
        Scene initialization and system hookup is done here
    */
    void Engine::Run(SetupCallback&& setup)
    {
        static double a     = 0;
        static double frame = 0;
        setup(this);
        m_LastTimePoint = std::chrono::steady_clock::now();
        while (!m_ShouldClose)
        {
            m_Window->PollEvents();
            // update scene
            float delta = GetDeltaTime();
            a += delta / 1000000;
            frame++;
            for (auto& scene : m_Scenes)
            {
                scene.second->Tick(delta / 1000000);
            }
            m_FrameCount++;
        }
        m_Driver->WaitIdle();

        for (auto& scene : m_Scenes)
        {
            scene.second->Shutdown();
        }
        std::cout << "--------------------------------------\n";
        std::cout << "Average Time: " << a / frame << std::endl;
        std::cout << "--------------------------------------\n";
    }

    Scene* Engine::CreateScene(const std::string& debugName)
    {
        auto scene = new Scene(this, debugName);

        m_Scenes.insert({debugName, scene});

        return scene;
    }

    Texture* Engine::CreateTexture(const TextureDescription& desc) { return m_ResourceManager.CreateTexture(desc); }
    Buffer*  Engine::CreateBuffer(const BufferDescription& desc) { return m_ResourceManager.CreateBuffer(desc); }
    Mesh*    Engine::CreateMesh(const BoxMeshDescription& desc) { return m_ResourceManager.CreateMesh(desc); }
    Mesh*    Engine::CreateMesh(const std::string& path) { return m_ResourceManager.CreateMesh(path); }

    std::pair<uint32_t, uint32_t> Engine::GetWindowDimension()
    {
        uint32_t width, height;
        m_Window->GetFramebufferSize(&width, &height);

        return {width, height};
    }

    ShaderSet* Engine::GetShaderSet(const std::string& name) { return m_ResourceManager.GetShaderSet(name); }

    Texture* Engine::GetDefaultSkybox() { return m_ResourceManager.GetDefaultSkybox(); }
    Texture* Engine::GetDefaultPrefilteredEnv() { return m_ResourceManager.GetDefaultPrefilteredEnv(); }
    Texture* Engine::GetBRDFLut() { return m_ResourceManager.GetBRDFLut(); }
    Texture* Engine::GetDither() { return m_ResourceManager.GetDither(); }
    Mesh*    Engine::GetSkyboxMesh() { return m_ResourceManager.GetSkyboxMesh(); }

    MaterialInstance* Engine::CreateMaterial(ShadingModel shading)
    {
        return m_ResourceManager.CreateMaterialInstance(shading);
    }
} // namespace Zephyr
