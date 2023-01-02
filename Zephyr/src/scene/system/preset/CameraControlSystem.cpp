#include "CameraControlSystem.h"
#include "core/event/EventSystem.h"
#include "core/event/KeyboardEvent.h"
#include "core/event/MouseEvent.h"
#include "scene/Entity.h"
#include "scene/component/CameraControlComponent.h"
#include "scene/component/MainCameraComponent.h"

namespace Zephyr
{
    CameraControlSystem::CameraControlSystem(Engine* engine) :
        System(engine, {}, {CameraControlComponent::ID, MainCameraComponent::ID}, {})
    {
        EventCenter::Get()->Register<KeyPressedEvent>([this](KeyPressedEvent& event) {
            auto keycode = event.GetKeyCode();
            switch (keycode)
            {
                case KeyCode::W:
                    this->m_ActionKey |= Forward;
                    break;
                case KeyCode::S:
                    this->m_ActionKey |= Back;
                    break;
                case KeyCode::A:
                    this->m_ActionKey |= Left;
                    break;
                case KeyCode::D:
                    this->m_ActionKey |= Right;
                    break;
                default:
                    break;
            }
            return true;
        });

        EventCenter::Get()->Register<KeyReleasedEvent>([this](KeyReleasedEvent& event) {
            auto keycode = event.GetKeyCode();
            switch (keycode)
            {
                case KeyCode::W:
                    this->m_ActionKey &= (completeAction ^ Forward);
                    break;
                case KeyCode::S:
                    this->m_ActionKey &= (completeAction ^ Back);
                    break;
                case KeyCode::A:
                    this->m_ActionKey &= (completeAction ^ Left);
                    break;
                case KeyCode::D:
                    this->m_ActionKey &= (completeAction ^ Right);
                    break;
                default:
                    break;
            }
            return true;
        });

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
            if (m_ActionKey & Forward)
            {
                camera->camera.Forward(.03);
            }
            if (m_ActionKey & Back)
            {
                camera->camera.Back(.03);
            }
            if (m_ActionKey & Left)
            {
                camera->camera.Left(.03);
            }
            if (m_ActionKey & Right)
            {
                camera->camera.Right(.03);
            }
        }

        m_CameraAngularMovementX = 0.;
        m_CameraAngularMovementY = 0.;
    }
} // namespace Zephyr