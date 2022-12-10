#pragma once
#include "Component.h"
#include <glm/glm.hpp>

namespace Zephyr
{
    COMPONENT(TransformComponent) { glm::mat4 transform; };
} // namespace Zephyr