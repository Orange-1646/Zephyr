#pragma once
#include "Component.h"
#include "render/Light.h"

namespace Zephyr
{
    COMPONENT(DirectionalLightComponent)
    { 
        DirectionalLight light;
    };
}
