#include "Mesh.h"
#include "rhi/Driver.h"

namespace Zephyr
{
    Mesh::Mesh() {}

    void Mesh::SetMaterials(const std::vector<MaterialInstance*>& materials) { m_Materials = materials; }

    void Mesh::InitResource(Driver* driver)
    {
        BufferDescription desc {};
        desc.size         = m_Vertices.size() * sizeof(Vertex);
        desc.usage        = BufferUsageBits::Vertex;
        desc.memoryType   = BufferMemoryType::Static;
        desc.pipelines    = PipelineTypeBits::Graphics;
        desc.shaderStages = ShaderStageBits::Vertex;

        auto vb = driver->CreateBuffer(desc);

        desc.size  = m_Indices.size() * sizeof(uint32_t);
        desc.usage = BufferUsageBits::Index;

        auto ib = driver->CreateBuffer(desc);

        // upload data into it
        BufferUpdateDescriptor update {};
        update.data      = (void*)m_Vertices.data();
        update.size      = m_Vertices.size() * sizeof(Vertex);
        update.srcOffset = 0;
        update.dstOffset = 0;

        driver->UpdateBuffer(update, vb);

        update.data      = (void*)m_Indices.data();
        update.size      = m_Indices.size() * sizeof(uint32_t);
        update.srcOffset = 0;
        update.dstOffset = 0;

        driver->UpdateBuffer(update, ib);

        m_VertexBuffer = vb;
        m_IndexBuffer  = ib;
    }

    void Mesh::Destroy(Driver* driver)
    {
        driver->DestroyBuffer(m_VertexBuffer);
        driver->DestroyBuffer(m_IndexBuffer);
    }
} // namespace Zephyr