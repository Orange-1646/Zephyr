#include "CameraControlSystem.h"
#include "core/event/EventSystem.h"
#include "core/event/KeyboardEvent.h"
#include "core/event/MouseEvent.h"
#include "scene/component/CameraControlComponent.h"
#include "scene/component/MainCameraComponent.h"
#include "scene/Entity.h"

namespace Zephyr
{
    CameraControlSystem::CameraControlSystem(Engine* engine) :
        System(engine, {}, {CameraControlComponent::ID, MainCameraComponent::ID}, {})
    {
        //EventCenter::Get()->Register<KeyPressedEvent>([](KeyPressedEvent& event) {
        //    return true;
        //});

        EventCenter::Get()->Register<MouseMovedEvent>([this](MouseMovedEvent& event) {
            this->m_CameraAngularMovementX += event.GetDiffX() * m_CameraAngluarSensitivity;
            this->m_CameraAngularMovementY += event.GetDiffY() * m_CameraAngluarSensitivity;

            return true;
        });
    }

    void CameraControlSystem::Execute(float delta, const std::vector<Entity*>& entities)
    {
        for (auto& entity : entities)
        {
            auto camera = entity->GetComponent<MainCameraComponent>();
            camera->camera.Rotate(m_CameraAngularMovementX * delta, m_CameraAngularMovementY * delta);
        }

        m_CameraAngularMovementX = 0.;
        m_CameraAngularMovementY = 0.;
    }
} // namespace Zephyr