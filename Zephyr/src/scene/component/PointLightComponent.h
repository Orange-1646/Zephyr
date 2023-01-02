#pragma once
#include "Component.h"
#include "render/Light.h"
#include <glm/glm.hpp>

namespace Zephyr
{
    COMPONENT(PointLightComponent) { PointLight light; };
} // namespace Zephyr
