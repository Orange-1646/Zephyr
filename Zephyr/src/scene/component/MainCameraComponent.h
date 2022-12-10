#pragma once
#include "Component.h"
#include "render/Camera.h"

namespace Zephyr
{
    COMPONENT(MainCameraComponent)
    { 
        Camera camera;
    };
}
