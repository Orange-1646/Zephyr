#pragma once

#include <glm/glm.hpp>

namespace Zephyr
{

    struct AABB
    {
        glm::vec3 min;
        glm::vec3 max;

        AABB() : min(0.0f), max(0.0f) {}

        AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}
    };

} // namespace Hazel