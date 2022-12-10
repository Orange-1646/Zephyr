#pragma once

#include "resource/Mesh.h"

namespace Zephyr
{
    struct BoxMeshDescription
    {
        float width;
        float height;
        float depth;
    };

    class BoxMesh final : public Mesh
    {
    private:
        BoxMesh(const BoxMeshDescription& descp);
        ~BoxMesh() override;

    private:
        BoxMeshDescription m_Description;

        friend class ResourceManager;
    };
}