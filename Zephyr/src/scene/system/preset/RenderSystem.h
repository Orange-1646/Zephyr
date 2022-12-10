#pragma once
#include "scene/system/System.h"
#include "render/Renderer.h"

namespace Zephyr
{
    class Engine;
    class Entity;

    SYSTEM(RenderSystem)
    {
    public:
        RenderSystem(Engine * engine);
        ~RenderSystem() override = default;

        void Execute(float delta, const std::vector<Entity*>& entities) override;
        void Shutdown() override;

    private:
        Renderer m_Renderer;
    };
}