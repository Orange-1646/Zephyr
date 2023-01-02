#pragma once
#include "pch.h"
#include "scene/system/System.h"

namespace Zephyr
{
    class Engine;
    class Entity;

    enum CameraAction: uint32_t
    {
        Forward = 1,
        Back = Forward << 1,
        Left = Back << 1,
        Right = Left << 1,
    };

    using CameraActionKey = uint32_t;

    static constexpr CameraActionKey completeAction = 0xFFFFFFFF;

    SYSTEM(CameraControlSystem)
    {
    public:
        CameraControlSystem(Engine * engine);
        ~CameraControlSystem() override = default;

        void Execute(float delta, const std::vector<Entity*>& entities) override;
        void Shutdown() override {}

    private:
        CameraActionKey m_ActionKey = 0;

        double m_CameraAngularMovementX = 0.;
        double m_CameraAngularMovementY = 0.;

        double m_CameraAngluarSensitivity = 0.001;

        double m_CameraMovementX = 0.;
        double m_CameraMovementY = 0.;
    };
} // namespace Zephyr