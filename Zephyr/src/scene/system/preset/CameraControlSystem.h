#pragma once
#include "pch.h"
#include "scene/system/System.h"

namespace Zephyr
{
    class Engine;
    class Entity;

    SYSTEM(CameraControlSystem)
    {
    public:
        CameraControlSystem(Engine * engine);
        ~CameraControlSystem() override = default;

        void Execute(float delta, const std::vector<Entity*>& entities) override;
        void Shutdown() override {}

    private:

        double m_CameraAngularMovementX = 0.;
        double m_CameraAngularMovementY = 0.;

        double m_CameraAngluarSensitivity = 0.001;

        double m_CameraMovementX = 0.;
        double m_CameraMovementY = 0.;
    };
} // namespace Zephyr