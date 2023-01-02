#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "engine/Engine.h"
#include "resource/MaterialInstance.h"
#include "resource/Mesh.h"
#include "resource/preset/geometry/BoxMesh.h"
#include "scene/Scene.h"
#include "scene/component/DirectionalLightComponent.h"
#include "scene/component/MainCameraComponent.h"
#include "scene/component/MeshComponent.h"
#include "scene/component/PointLightComponent.h"
#include "scene/component/TransformComponent.h"
#include "scene/system/preset/CameraControlSystem.h"
#include "scene/system/preset/RenderSystem.h"
#include <glm/gtc/matrix_transform.hpp>

struct A
{};

int main()
{
    using namespace Zephyr;

    auto engine = Engine::Create({1920, 1080, DriverType::Vulkan, "ZephyrEngineTest", false, false, true});

    engine->Run([&](Engine* engine) {
        // auto mask  = engine->CreateMesh("asset/model/venice_mask/scene.gltf");
        auto scene = engine->CreateScene("test");

        auto e2                          = scene->CreateEntity();
        auto lightComponent2             = scene->AddComponent<DirectionalLightComponent>(e2);
        lightComponent2->light.direction = {0, -1., -1};
        lightComponent2->light.radiance  = {2., 2., 2.};

        for (float i = -1.f; i < 1.f; i++)
        {
            for (float j = -1.f; j < 1.f; j++)
            {
                auto pl = scene->CreateEntity();

                auto cube = engine->CreateMesh("asset/model/Default/Cube.fbx");
                auto lm3  = scene->AddComponent<MeshComponent>(pl);
                lm3->mesh = cube;
                auto lmm  = engine->CreateMaterial(ShadingModel::Lit);
                lmm->CastShadow(false);
                lmm->ReceiveShadow(false);
                lmm->SetConstantBlock<glm::vec3>("albedo", {0.0, 0.0, 0.0});
                lmm->SetConstantBlock<float>("metalness", 0.);
                lmm->SetConstantBlock<float>("roughness", 1.);
                lmm->SetConstantBlock<glm::vec3>("emission", {2. + i, 2. + j, 3.});
                lm3->mesh->SetMaterials({lmm});

                auto transform = glm::mat4(1.);

                auto lm3t       = scene->AddComponent<TransformComponent>(pl);
                lm3t->transform = glm::scale(glm::translate(transform, {i * 10., j * 15., 0}), {20, 500, 20});
            }
        }

        // auto meshCmp =

        auto e3               = scene->CreateEntity();
        auto cameraComponent3 = scene->AddComponent<MainCameraComponent>(e3);
        cameraComponent3->camera.SetPerspective(45., 1920. / 1080., .1, 1000.);
        cameraComponent3->camera.LookAt({0, 7, 20}, {0, -1, -1}, {0.f, 1.f, 0.f});

        scene->AddSystem<RenderSystem>();
        scene->AddSystem<CameraControlSystem>();
    });

    Engine::Destroy(engine);
}