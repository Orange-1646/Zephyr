#pragma once
#include "core/math/AABB.h"
#include "glm/glm.hpp"
#include "pch.h"
#include "rhi/Handle.h"
#include "rhi/RHIBuffer.h"

namespace Zephyr
{
    class Engine;
    class MaterialInstance;
    class Driver;

    struct Vertex
    {
        // position
        float px, py, pz;
        // normal
        float nx, ny, nz;
        // tangent
        float tx, ty, tz;
        // bitangent
        float bx, by, bz;
        // uv
        float tu, tv;
    };

    struct Submesh
    {
        uint32_t baseVertex;
        uint32_t baseIndex;
        uint32_t vertexCount;
        uint32_t indexCount;
        uint32_t materialIndex;

        glm::mat4 transform      = glm::mat4(1.f);
        glm::mat4 localTransform = glm::mat4(1.f);

        AABB aabb;
    };

    class Mesh
    {
    public:
    public:
        void SetMaterials(const std::vector<MaterialInstance*>& materials);

        void InitResource(Driver* driver);
        void Destroy(Driver* driver);

        inline const std::vector<Vertex>&            GetVertices() { return m_Vertices; }
        inline const std::vector<uint32_t>&          GetIndices() { return m_Indices; }
        inline const std::vector<Submesh>&           GetSubmeshes() { return m_Submeshes; }
        inline const std::vector<MaterialInstance*>& GetMaterials() { return m_Materials; }
        inline Handle<RHIBuffer>                     GetVertexBuffer() { return m_VertexBuffer; }
        inline Handle<RHIBuffer>                     GetIndexBuffer() { return m_IndexBuffer; }

    protected:
        Mesh();
        virtual ~Mesh() = default;

    protected:
        std::vector<Vertex>            m_Vertices;
        std::vector<uint32_t>          m_Indices;
        std::vector<Submesh>           m_Submeshes;
        std::vector<MaterialInstance*> m_Materials;
        Handle<RHIBuffer>              m_VertexBuffer;
        Handle<RHIBuffer>              m_IndexBuffer;

        AABB m_Aabb;

    private:
        friend class ResourceManager;
        friend class ModelLoader;
    };
} // namespace Zephyr