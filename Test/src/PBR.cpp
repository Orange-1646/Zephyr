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
    BoxMeshDescription d {5, 5, 5};
    auto               box = engine->CreateMesh(d);

    auto material = engine->CreateMaterial(ShadingModel::Lit);
    material->SetConstantBlock<glm::vec3>("albedo", {1.0, 1., 1.});
    material->SetConstantBlock<glm::vec3>("emission", {0.0, 0., 0.});
    material->SetConstantBlock<float>("metalness", 1);
    material->SetConstantBlock<float>("roughness", .7);
    box->SetMaterials({material});

    auto box2      = engine->CreateMesh(d);
    auto material2 = engine->CreateMaterial(ShadingModel::Lit);
    material2->SetConstantBlock<glm::vec3>("albedo", {1.0, .8, 0.7});
    material2->SetConstantBlock<glm::vec3>("emission", {0., 0., 0.});
    box2->SetMaterials({material2});

    engine->Run([&](Engine* engine) {

        // auto mask  = engine->CreateMesh("asset/model/venice_mask/scene.gltf");
        auto scene = engine->CreateScene("test");

        auto e2                          = scene->CreateEntity();
        auto lightComponent2             = scene->AddComponent<DirectionalLightComponent>(e2);
        lightComponent2->light.direction = {glm::sin(-glm::radians(27.)), -.5f, -glm::cos(glm::radians(27.))};
        lightComponent2->light.direction = {0, -0.2, -1};
        lightComponent2->light.radiance  = {.3, .3, .3};

        for (float i = -3.f; i < 3.f; i++)
        {
            for (float j = -3.f; j < 3.f; j++)
            {
                break;
                auto pl                         = scene->CreateEntity();
                auto lightComponent3            = scene->AddComponent<PointLightComponent>(pl);
                lightComponent3->light.position = {i * 10., 3., j * 10.};
                // lightComponent3->light.radiance = {1. + 2.f / 6.f * (i + 3.f) / 5., 1., 1. + 2.f / 6.f * (3. - i)
                // / 5.};
                lightComponent3->light.radiance = {.3, .4, .5};
                lightComponent3->light.radius   = 40.;
                lightComponent3->light.falloff  = 0.001;

                auto               lm3 = scene->AddComponent<MeshComponent>(pl);
                BoxMeshDescription d2 {.2, .2, .2};
                lm3->mesh = engine->CreateMesh(d2);
                auto lmm  = engine->CreateMaterial(ShadingModel::Lit);
                lmm->CastShadow(false);
                lmm->ReceiveShadow(false);
                lmm->SetConstantBlock<glm::vec3>("albedo", {0.0, 0.0, 0.0});
                lmm->SetConstantBlock<float>("metalness", 0.);
                lmm->SetConstantBlock<float>("roughness", 1.);
                // lmm->SetConstantBlock<glm::vec3>("emission",
                //                                  {1. + 2.f / 6.f * (i + 3.f), 2., 1. + 2.f / 6.f * (3. - i)});
                lmm->SetConstantBlock<glm::vec3>("emission", {.3, .4, .5});
                lm3->mesh->SetMaterials({lmm});

                auto transform = glm::mat4(1.);

                auto lm3t       = scene->AddComponent<TransformComponent>(pl);
                lm3t->transform = glm::translate(transform, {i * 10., 3., j * 10.});
            }
        }

        // auto meshCmp =

        auto e3               = scene->CreateEntity();
        auto cameraComponent3 = scene->AddComponent<MainCameraComponent>(e3);
        cameraComponent3->camera.SetPerspective(45., 1920. / 1080., .1, 1000.);
        cameraComponent3->camera.LookAt({0, 7, 20}, {0, -1, -1}, {0.f, 1.f, 0.f});

        auto transform = glm::mat4(1.);
        transform      = glm::translate(transform, {0, 3, 0});

        for (int i = -3; i < 3; i++)
        {
            for (int j = 0; j < 6; j++)
            {
                // if (!(i == 0 && j == 0))
                //{
                //     continue;
                // }
                auto sphere         = engine->CreateMesh("asset/model/Default/Sphere.fbx");
                auto material = engine->CreateMaterial(ShadingModel::Lit);
                material->SetConstantBlock<glm::vec3>("albedo", {1.0, 1.0, 1.0});
                material->SetConstantBlock<glm::vec3>("emission", {0.0, 0., 0.});
                float metalness = 1. - (i + 3.f) / 6.f;
                float roughness = 1. - j / 6.f;

                material->SetConstantBlock<float>("metalness", metalness);
                material->SetConstantBlock<float>("roughness", roughness);
                sphere->SetMaterials({material});
                auto e1             = scene->CreateEntity();
                auto meshComponent1 = scene->AddComponent<MeshComponent>(e1);
                auto meshTransform  = scene->AddComponent<TransformComponent>(e1);

                meshComponent1->mesh = sphere;
                meshTransform->transform =
                    glm::scale(glm::translate(transform, {10 * i, 10 * (j - 3), 0}), {300, 300, 300});
            }
        }
        // auto e1             = scene->CreateEntity();
        // auto meshComponent1 = scene->AddComponent<MeshComponent>(e1);
        // auto meshTransform  = scene->AddComponent<TransformComponent>(e1);

        // meshComponent1->mesh     = box2;
        // meshTransform->transform = glm::translate(transform, {10, 0, 0});

        scene->AddSystem<RenderSystem>();
        scene->AddSystem<CameraControlSystem>();
    });

    Engine::Destroy(engine);
}