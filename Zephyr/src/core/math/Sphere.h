#include <glm/glm.hpp>

namespace Zephyr
{
    class Sphere
    {
    public:
        Sphere(const glm::vec3& center, float radius);
    public:
        glm::vec3 m_Center;
        float     m_Radius;
    };
}