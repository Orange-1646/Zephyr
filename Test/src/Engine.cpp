#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#include "engine/Engine.h"
#include "resource/Mesh.h"
#include "resource/preset/geometry/BoxMesh.h"
#include "resource/MaterialInstance.h"
#include "scene/Scene.h"
#include "scene/component/DirectionalLightComponent.h"
#include "scene/component/PointLightComponent.h"
#include "scene/component/MainCameraComponent.h"
#include "scene/component/MeshComponent.h"
#include "scene/system/preset/RenderSystem.h"
#include "scene/system/preset/CameraControlSystem.h"
#include "scene/component/TransformComponent.h"
#include <glm/gtc/matrix_transform.hpp>

struct A
{};

int main()
{
    using namespace Zephyr;

    auto               engine = Engine::Create({1920, 1080, DriverType::Vulkan, "ZephyrEngineTest", false, false, true});

    engine->Run([&](Engine* engine) {
        //auto mask  = engine->CreateMesh("asset/model/venice_mask/scene.gltf");
        auto mask  = engine->CreateMesh("asset/model/pbr_kabuto_samurai_helmet/scene.gltf");
        //auto mask  = engine->CreateMesh("asset/model/game_ready_scifi_helmet/scene.gltf");
        //auto mask =
        //    engine->CreateMesh("asset/model/sponza-gltf-pbr/sponza-gltf-pbr/sponza.glb");
        auto scene = engine->CreateScene("test");

        auto e2                          = scene->CreateEntity();
        auto lightComponent2             = scene->AddComponent<DirectionalLightComponent>(e2);
        lightComponent2->light.direction = {-1, -1., -1.};
        lightComponent2->light.radiance  = {1., 1., 1.};

        for (float i = -3.f; i < 3.f; i++)
        {
            for (float j = -3.f; j < 3.f; j++)
            {
                if (!(i == 0 && j == 0))
                {
                    continue;
                }
                //break;
                //if (!(i == 2 && j == 2))
                //{
                //    continue;
                //}
                auto pl                         = scene->CreateEntity();
                auto lightComponent3            = scene->AddComponent<PointLightComponent>(pl);
                lightComponent3->light.position = {i * 10. , 3., j * 10.};
                lightComponent3->light.radiance = {1. + 2.f / 6.f * (i + 3.f) / 5., 1., 1. + 2.f / 6.f * (3. - i) / 5.};
                //lightComponent3->light.radiance = {.3, .4, .5};
                lightComponent3->light.radius   = 20.;
                lightComponent3->light.falloff  = 0.001;

                auto               lm3 = scene->AddComponent<MeshComponent>(pl);
                BoxMeshDescription d2 {.2, .2, .2};
                lm3->mesh = engine->CreateMesh("asset/model/Default/Cube.fbx");
                auto lmm  = engine->CreateMaterial(ShadingModel::Lit);
                lmm->CastShadow(false);
                lmm->ReceiveShadow(false);
                lmm->SetConstantBlock<glm::vec3>("albedo", {0.0, 0.0, 0.0});
                lmm->SetConstantBlock<float>("metalness", 0.);
                lmm->SetConstantBlock<float>("roughness", 1.);
                lmm->SetConstantBlock<glm::vec3>("emission",
                                                 {1. + 2.f / 6.f * (i + 3.f), 2., 1. + 2.f / 6.f * (3. - i)});
                //lmm->SetConstantBlock<glm::vec3>("emission",
                //                                 {.3, .4, .5});
                lm3->mesh->SetMaterials({lmm});

                auto transform  = glm::mat4(1.);

                auto lm3t = scene->AddComponent<TransformComponent>(pl);
                lm3t->transform =
                    glm::scale(glm::translate(transform, {10 * i, 10 * j + 10, 0}), {10, 10, 10});
                //lm3t->transform = glm::translate(transform, {i * 10., 3., j * 10.});
            }
        }


        //auto meshCmp = 

        auto e3               = scene->CreateEntity();
        auto cameraComponent3 = scene->AddComponent<MainCameraComponent>(e3);
        cameraComponent3->camera.SetPerspective(45., 1920. / 1080., .1, 1000.);
        cameraComponent3->camera.LookAt({0, 7, 20}, {0, 0, -1}, {0.f, 1.f, 0.f});

        auto transform = glm::mat4(1.);
        transform      = glm::translate(transform, {0, 3, 0});

        //auto fm = engine->CreateMesh(BoxMeshDescription{300., 1., 300.});
        //auto floor = scene->CreateEntity();
        //auto floorMesh = scene->AddComponent<MeshComponent>(floor);
        //floorMesh->mesh    = fm;

        // mask
        auto msk = scene->CreateEntity();
        auto mskMesh = scene->AddComponent<MeshComponent>(msk);
        mskMesh->mesh = mask;
        auto mskTrans = scene->AddComponent<TransformComponent>(msk);
        mskTrans->transform = glm::translate(transform, {-3, 0, -5});


        // boxes
        //auto e1             = scene->CreateEntity();
        //auto meshComponent1 = scene->AddComponent<MeshComponent>(e1);
        //auto meshTransform  = scene->AddComponent<TransformComponent>(e1);

        //meshComponent1->mesh     = box;
        //meshTransform->transform = glm::translate(transform, {0, 0, -0});
        //for (int i = -3; i < 3; i++)
        //{
        //    for (int j = 0; j < 6; j++)
        //    {
        //        //if (!(i == 0 && j == 0))
        //        //{
        //        //    continue;
        //        //}
        //        auto e1             = scene->CreateEntity();
        //        auto meshComponent1 = scene->AddComponent<MeshComponent>(e1);
        //        auto meshTransform  = scene->AddComponent<TransformComponent>(e1);

        //        meshComponent1->mesh = box;
        //        meshTransform->transform = glm::translate(transform, {20 * i, 0, -20 * j});
        //    }
        //}
        //auto e1             = scene->CreateEntity();
        //auto meshComponent1 = scene->AddComponent<MeshComponent>(e1);
        //auto meshTransform  = scene->AddComponent<TransformComponent>(e1);

        //meshComponent1->mesh     = box2;
        //meshTransform->transform = glm::translate(transform, {10, 0, 0});

        scene->AddSystem<RenderSystem>();
        scene->AddSystem<CameraControlSystem>();
    });

    Engine::Destroy(engine);
}