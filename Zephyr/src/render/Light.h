#pragma once
#include <glm/glm.hpp>

namespace Zephyr
{
    struct DirectionalLight
    {
        glm::vec3 direction;
        glm::vec3 radiance;
        bool      castShadow;
    };
}