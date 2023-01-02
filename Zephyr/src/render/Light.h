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
    struct PointLight
    {
        glm::vec3  position;
        float radius;
        glm::vec3  radiance;
        float falloff;
    };
}