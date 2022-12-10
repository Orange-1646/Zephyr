#include "BoxMesh.h"

namespace Zephyr
{
    BoxMesh::BoxMesh(const BoxMeshDescription& desc) : m_Description(desc)
    {
    
    
        // 12 faces 24 vertex
        float halfWidth  = desc.width / 2;  // x
        float halfHeight = desc.height / 2; // y
        float halfDepth  = desc.depth / 2;  // z

        m_Vertices.reserve(24);
        m_Indices.reserve(36);

                // left
        {
            auto& v1 = m_Vertices.emplace_back();
            v1.px    = -halfWidth;
            v1.py    = -halfHeight;
            v1.pz    = halfDepth;
            v1.tu    = 0;
            v1.tv    = 0;
            v1.nx    = -1;
            v1.ny    = 0;
            v1.nz    = 0;

            auto& v2 = m_Vertices.emplace_back();
            v2.px    = -halfWidth;
            v2.py    = halfHeight;
            v2.pz    = halfDepth;
            v2.tu    = 1;
            v2.tv    = 0;
            v2.nx    = -1;
            v2.ny    = 0;
            v2.nz    = 0;

            auto& v3 = m_Vertices.emplace_back();
            v3.px    = -halfWidth;
            v3.py    = halfHeight;
            v3.pz    = -halfDepth;
            v3.tu    = 1;
            v3.tv    = 1;
            v3.nx    = -1;
            v3.ny    = 0;
            v3.nz    = 0;

            auto& v4 = m_Vertices.emplace_back();
            v4.px    = -halfWidth;
            v4.py    = -halfHeight;
            v4.pz    = -halfDepth;
            v4.tu    = 0;
            v4.tv    = 1;
            v4.nx    = -1;
            v4.ny    = 0;
            v4.nz    = 0;

            m_Indices.insert(m_Indices.end(), {0, 1, 2});
            m_Indices.insert(m_Indices.end(), {0, 2, 3});
        }
        // right
        {
            auto& v1 = m_Vertices.emplace_back();
            v1.px    = halfWidth;
            v1.py    = -halfHeight;
            v1.pz    = halfDepth;
            v1.tu    = 0;
            v1.tv    = 0;
            v1.nx    = 1;
            v1.ny    = 0;
            v1.nz    = 0;

            auto& v2 = m_Vertices.emplace_back();
            v2.px    = halfWidth;
            v2.py    = -halfHeight;
            v2.pz    = -halfDepth;
            v2.tu    = 0;
            v2.tv    = 1;
            v2.nx    = 1;
            v2.ny    = 0;
            v2.nz    = 0;

            auto& v3 = m_Vertices.emplace_back();
            v3.px    = halfWidth;
            v3.py    = halfHeight;
            v3.pz    = -halfDepth;
            v3.tu    = 1;
            v3.tv    = 1;
            v3.nx    = 1;
            v3.ny    = 0;
            v3.nz    = 0;

            auto& v4 = m_Vertices.emplace_back();
            v4.px    = halfWidth;
            v4.py    = halfHeight;
            v4.pz    = halfDepth;
            v4.tu    = 1;
            v4.tv    = 0;
            v4.nx    = 1;
            v4.ny    = 0;
            v4.nz    = 0;

            m_Indices.insert(m_Indices.end(), {4, 5, 6});
            m_Indices.insert(m_Indices.end(), {4, 6, 7});
        }

        // top
        {
            auto& v1 = m_Vertices.emplace_back();
            v1.px    = -halfWidth;
            v1.py    = halfHeight;
            v1.pz    = halfDepth;
            v1.tu    = 0;
            v1.tv    = 0;
            v1.nx    = 0;
            v1.ny    = 1;
            v1.nz    = 0;

            auto& v4 = m_Vertices.emplace_back();
            v4.px    = halfWidth;
            v4.py    = halfHeight;
            v4.pz    = halfDepth;
            v4.tu    = 0;
            v4.tv    = 1;
            v4.nx    = 0;
            v4.ny    = 1;
            v4.nz    = 0;

            auto& v3 = m_Vertices.emplace_back();
            v3.px    = halfWidth;
            v3.py    = halfHeight;
            v3.pz    = -halfDepth;
            v3.tu    = 1;
            v3.tv    = 1;
            v3.nx    = 0;
            v3.ny    = 1;
            v3.nz    = 0;

            auto& v2 = m_Vertices.emplace_back();
            v2.px    = -halfWidth;
            v2.py    = halfHeight;
            v2.pz    = -halfDepth;
            v2.tu    = 1;
            v2.tv    = 0;
            v2.nx    = 0;
            v2.ny    = 1;
            v2.nz    = 0;

            m_Indices.insert(m_Indices.end(), {8, 9, 10});
            m_Indices.insert(m_Indices.end(), {8, 10, 11});
        }
        // bottom
        {
            auto& v1 = m_Vertices.emplace_back();
            v1.px    = -halfWidth;
            v1.py    = -halfHeight;
            v1.pz    = halfDepth;
            v1.tu    = 0;
            v1.tv    = 0;
            v1.nx    = 0;
            v1.ny    = -1;
            v1.nz    = 0;

            auto& v4 = m_Vertices.emplace_back();
            v4.px    = halfWidth;
            v4.py    = -halfHeight;
            v4.pz    = -halfDepth;
            v4.tu    = 1;
            v4.tv    = 0;
            v4.nx    = 0;
            v4.ny    = -1;
            v4.nz    = 0;

            auto& v3 = m_Vertices.emplace_back();
            v3.px    = halfWidth;
            v3.py    = -halfHeight;
            v3.pz    = halfDepth;
            v3.tu    = 1;
            v3.tv    = 1;
            v3.nx    = 0;
            v3.ny    = -1;
            v3.nz    = 0;

            auto& v2 = m_Vertices.emplace_back();
            v2.px    = -halfWidth;
            v2.py    = -halfHeight;
            v2.pz    = -halfDepth;
            v2.tu    = 0;
            v2.tv    = 1;
            v2.nx    = 0;
            v2.ny    = -1;
            v2.nz    = 0;

            m_Indices.insert(m_Indices.end(), {12, 13, 14});
            m_Indices.insert(m_Indices.end(), {12, 15, 13});
        }
        // back
        {
            auto& v1 = m_Vertices.emplace_back();
            v1.px    = -halfWidth;
            v1.py    = -halfHeight;
            v1.pz    = -halfDepth;
            v1.tu    = 0;
            v1.tv    = 0;
            v1.nx    = 0;
            v1.ny    = 0;
            v1.nz    = -1;

            auto& v2 = m_Vertices.emplace_back();
            v2.px    = halfWidth;
            v2.py    = -halfHeight;
            v2.pz    = -halfDepth;
            v2.tu    = 1;
            v2.tv    = 0;
            v2.nx    = 0;
            v2.ny    = 0;
            v2.nz    = -1;

            auto& v3 = m_Vertices.emplace_back();
            v3.px    = halfWidth;
            v3.py    = halfHeight;
            v3.pz    = -halfDepth;
            v3.tu    = 1;
            v3.tv    = 1;
            v3.nx    = 0;
            v3.ny    = 0;
            v3.nz    = -1;

            auto& v4 = m_Vertices.emplace_back();
            v4.px    = -halfWidth;
            v4.py    = halfHeight;
            v4.pz    = -halfDepth;
            v4.tu    = 0;
            v4.tv    = 1;
            v4.nx    = 0;
            v4.ny    = 0;
            v4.nz    = -1;

            m_Indices.insert(m_Indices.end(), {16, 18, 17});
            m_Indices.insert(m_Indices.end(), {16, 19, 18});
        }
        // front
        {
            auto& v1 = m_Vertices.emplace_back();
            v1.px    = -halfWidth;
            v1.py    = -halfHeight;
            v1.pz    = halfDepth;
            v1.tu    = 0;
            v1.tv    = 0;
            v1.nx    = 0;
            v1.ny    = 0;
            v1.nz    = 1;

            auto& v2 = m_Vertices.emplace_back();
            v2.px    = halfWidth;
            v2.py    = -halfHeight;
            v2.pz    = halfDepth;
            v2.tu    = 0;
            v2.tv    = 1;
            v2.nx    = 0;
            v2.ny    = 0;
            v2.nz    = 1;

            auto& v3 = m_Vertices.emplace_back();
            v3.px    = halfWidth;
            v3.py    = halfHeight;
            v3.pz    = halfDepth;
            v3.tu    = 1;
            v3.tv    = 1;
            v3.nx    = 0;
            v3.ny    = 0;
            v3.nz    = 1;

            auto& v4 = m_Vertices.emplace_back();
            v4.px    = -halfWidth;
            v4.py    = halfHeight;
            v4.pz    = halfDepth;
            v4.tu    = 1;
            v4.tv    = 0;
            v4.nx    = 0;
            v4.ny    = 0;
            v4.nz    = 1;

            m_Indices.insert(m_Indices.end(), {20, 21, 22});
            m_Indices.insert(m_Indices.end(), {20, 22, 23});
        }

        auto& submesh         = m_Submeshes.emplace_back();
        submesh.baseIndex     = 0;
        submesh.baseVertex    = 0;
        submesh.indexCount    = m_Indices.size();
        submesh.vertexCount   = m_Vertices.size();
        submesh.materialIndex = 0;
    }

    BoxMesh::~BoxMesh() {}
} // namespace Zephyr