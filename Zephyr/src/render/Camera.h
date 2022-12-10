#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "core/Math/Sphere.h"
#include "glm/gtx/euler_angles.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Zephyr
{

    struct CameraCorners
    {
        glm::vec3 nearCorners[4];
        glm::vec3 farCorners[4];
    };

    struct Camera
    {
        float fovy; // in degree
        float aspect;
        float zNear;
        float zFar;

        glm::vec3 position;
        glm::vec3 direction;
        glm::vec3 up;

        glm::vec3 postTransformDirection;

        glm::mat4 view;
        glm::mat4 projection;

        float pitch = 0;
        float yaw   = 0;

        void SetPerspective(float fovY, float aspectRatio, float zn, float zf);

        void LookAt(const glm::vec3& p, const glm::vec3& d, const glm::vec3& u);

        void Camera::Rotate(double horizontal, double vertical);

        CameraCorners Corners();

        static Sphere GetWorldSpaceOuterTangentSphere(Camera& camera, float zNearPct, float zFarPct);
    };
} // namespace Zephyr