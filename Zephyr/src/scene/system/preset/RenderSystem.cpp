#include "RenderSystem.h"
#include "engine/Engine.h"
#include "scene/component/DirectionalLightComponent.h"
#include "scene/component/MainCameraComponent.h"
#include "scene/component/MeshComponent.h"
#include "scene/component/TransformComponent.h"
#include "scene/Entity.h"
#include "render/Renderer.h"

namespace Zephyr
{
    RenderSystem::RenderSystem(Engine* engine):
        System(engine, {}, { DirectionalLightComponent::ID, MainCameraComponent::ID, MeshComponent::ID }, {}),
        m_Renderer(engine)
    {}

    void RenderSystem::Execute(float delta, const std::vector<Entity*>& entities)
    {
        SceneRenderData scene;
        for (auto& entity : entities)
        {
            auto b = entity->HasComponent<DirectionalLightComponent>();
            if (entity->HasComponent<DirectionalLightComponent>())
            {
                scene.light = entity->GetComponent<DirectionalLightComponent>()->light;
            }
            if (entity->HasComponent<MainCameraComponent>())
            {
                scene.camera = entity->GetComponent<MainCameraComponent>()->camera;
            }
            if (entity->HasComponent<MeshComponent>())
            {
                scene.meshes.push_back(entity->GetComponent<MeshComponent>()->mesh);
                if (entity->HasComponent<TransformComponent>())
                {
                    scene.transforms.push_back(entity->GetComponent<TransformComponent>()->transform);
                }
                else
                {
                    scene.transforms.push_back(glm::mat4(1.));
                }
            }
        }

        m_Renderer.Render(scene);
    }
    void RenderSystem::Shutdown() { m_Renderer.Shutdown();
    }
} // namespace Zephyr