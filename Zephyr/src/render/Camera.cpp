#include "Camera.h"

namespace Zephyr
{
    void Camera::SetPerspective(float fovY, float aspectRatio, float zn, float zf)
    {
        fovy   = fovY;
        aspect = aspectRatio;
        zNear  = zn;
        zFar   = zf;

        projection = glm::perspective(glm::radians(fovy), aspect, zNear, zFar);
    }
    void Camera::LookAt(const glm::vec3& p, const glm::vec3& d, const glm::vec3& u)
    {
        position               = p;
        direction              = d;
        up                     = u;
        postTransformDirection = direction;

        view = glm::lookAt(p, p + d, u);

        // float row;
        // glm::extractEulerAngleYXZ(view, yaw, pitch, row);
    }
    void Camera::Rotate(double horizontal, double vertical)
    {
        pitch -= vertical;
        yaw -= horizontal;
        if (pitch > glm::radians(89.))
        {
            pitch = glm::radians(89.);
        }
        if (pitch < glm::radians(-89.))
        {
            pitch = glm::radians(-89.);
        }
        auto rotation = glm::mat3(glm::eulerAngleYXZ(yaw, pitch, 0.f));

        postTransformDirection = rotation * direction;

        view = glm::lookAt(position, position + postTransformDirection, up);
    }
    CameraCorners Camera::Corners()
    {
        CameraCorners corners;
        // use inverse vp matrix to get world space frustum corner coords

        // 3    2
        //
        // 0    1
        // vulkan has upper-left corner as -1 -1 and depth from 0 to 1
        corners.nearCorners[0] = {-1., 1., 0.};
        corners.nearCorners[1] = {1., 1., 0.};
        corners.nearCorners[2] = {1., -1., 0.};
        corners.nearCorners[3] = {-1., -1., 0.};

        corners.farCorners[0] = {-1., 1., 1.};
        corners.farCorners[1] = {1., 1., 1.};
        corners.farCorners[2] = {1., -1., 1.};
        corners.farCorners[3] = {-1., -1., 1.};

        glm::mat4 inverseVP = glm::inverse(projection);

        for (auto& corner : corners.nearCorners)
        {
            auto m4 = inverseVP * glm::vec4(corner, 1.);
            corner  = m4 / m4.w;
        }

        for (auto& corner : corners.farCorners)
        {
            auto m4 = inverseVP * glm::vec4(corner, 1.);
            corner  = m4 / m4.w;
        }

        return corners;
    }

    Sphere Camera::GetWorldSpaceOuterTangentSphere(Camera& camera, float zNearPct, float zFarPct)
    {

        glm::vec3 cameraDirection = glm::normalize(camera.postTransformDirection);
        auto&     corners         = camera.Corners();

        CameraCorners slicedCorners;

        slicedCorners.nearCorners[0] = (1.f - zNearPct) * corners.nearCorners[0] + zNearPct * corners.farCorners[0];
        slicedCorners.nearCorners[1] = (1.f - zNearPct) * corners.nearCorners[1] + zNearPct * corners.farCorners[1];
        slicedCorners.nearCorners[2] = (1.f - zNearPct) * corners.nearCorners[2] + zNearPct * corners.farCorners[2];
        slicedCorners.nearCorners[3] = (1.f - zNearPct) * corners.nearCorners[3] + zNearPct * corners.farCorners[3];

        slicedCorners.farCorners[0] = (1.f - zFarPct) * corners.nearCorners[0] + zFarPct * corners.farCorners[0];
        slicedCorners.farCorners[1] = (1.f - zFarPct) * corners.nearCorners[1] + zFarPct * corners.farCorners[1];
        slicedCorners.farCorners[2] = (1.f - zFarPct) * corners.nearCorners[2] + zFarPct * corners.farCorners[2];
        slicedCorners.farCorners[3] = (1.f - zFarPct) * corners.nearCorners[3] + zFarPct * corners.farCorners[3];

        float a2 = glm::length(slicedCorners.nearCorners[2] - slicedCorners.nearCorners[0]) *
                   glm::length(slicedCorners.nearCorners[2] - slicedCorners.nearCorners[0]);
        float b2 = glm::length(slicedCorners.farCorners[2] - slicedCorners.farCorners[0]) *
                   glm::length(slicedCorners.farCorners[2] - slicedCorners.farCorners[0]);

        float zNear = (1.f - zNearPct) * camera.zNear + zNearPct * camera.zFar;
        float zFar  = (1.f - zFarPct) * camera.zNear + zFarPct * camera.zFar;

        float len = zFar - zNear;
        float x   = len * .5f + (a2 + b2) / (8.f * len);

        glm::vec3 center = camera.position + cameraDirection * (x + camera.zNear);
        float     r      = sqrt(x * x + a2 * .25f);

        return {center, r};
    }
} // namespace Zephyr