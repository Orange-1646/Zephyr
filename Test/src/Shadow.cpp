#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "engine/Engine.h"
#include "resource/MaterialInstance.h"
#include "resource/Mesh.h"
#include "resource/preset/geometry/BoxMesh.h"
#include "scene/Scene.h"
#include "scene/component/DirectionalLightComponent.h"
#include "scene/component/MainCameraComponent.h"
#include "scene/component/MeshComponent.h"
#include "scene/component/TransformComponent.h"
#include "scene/system/preset/CameraControlSystem.h"
#include "scene/system/preset/RenderSystem.h"
#include <glm/gtc/matrix_transform.hpp>

struct A
{};

int main()
{
    using namespace Zephyr;

    auto               engine = Engine::Create({1080, 640, DriverType::Vulkan, "ZephyrEngineTest", false, false, true});
    BoxMeshDescription d {5, 5, 5};
    auto               box = engine->CreateMesh(d);

    // auto mask = engine->CreateMesh("asset/model/venice_mask/scene.gltf");

    auto material = engine->CreateMaterial(ShadingModel::Lit);
    material->SetConstantBlock<glm::vec3>("albedo", {1., 1., 1.});
    box->SetMaterials({material});

    engine->Run([box](Engine* engine) {
        auto scene = engine->CreateScene("test");

        auto e2                          = scene->CreateEntity();
        auto lightComponent2             = scene->AddComponent<DirectionalLightComponent>(e2);
        lightComponent2->light.direction = {glm::sin(-glm::radians(27.)), -.5f, -glm::cos(glm::radians(27.))};
        lightComponent2->light.radiance  = {2., 2., 2.};

        auto e3               = scene->CreateEntity();
        auto cameraComponent3 = scene->AddComponent<MainCameraComponent>(e3);
        cameraComponent3->camera.SetPerspective(45., 1080. / 640., .1, 1000.);
        cameraComponent3->camera.LookAt({0, 20, 20}, {0, -1, -1}, {0.f, 1.f, 0.f});

        auto transform = glm::mat4(1.);
        transform      = glm::translate(transform, {0, 3, 0});

        // floor
        auto fm         = engine->CreateMesh(BoxMeshDescription {800., 1., 800.});
        auto floor      = scene->CreateEntity();
        auto floorMesh  = scene->AddComponent<MeshComponent>(floor);
        floorMesh->mesh = fm;

        // mask
        // auto msk = scene->CreateEntity();
        // auto mskMesh = scene->AddComponent<MeshComponent>(msk);
        // mskMesh->mesh = mask;
        // auto mskTrans = scene->AddComponent<TransformComponent>(msk);
        // mskTrans->transform = glm::scale(transform, {300., 300., 300.});

        // boxes
        for (int i = -10; i < 10; i++)
        {
            for (int j = 0; j < 20; j++)
            {
                auto e1             = scene->CreateEntity();
                auto meshComponent1 = scene->AddComponent<MeshComponent>(e1);
                auto meshTransform  = scene->AddComponent<TransformComponent>(e1);

                meshComponent1->mesh     = box;
                meshTransform->transform = glm::translate(transform, {20 * i, 0, -20 * j});
            }
        }

        scene->AddSystem<RenderSystem>();
        scene->AddSystem<CameraControlSystem>();
    });

    Engine::Destroy(engine);
}