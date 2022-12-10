#include "MaterialInstance.h"
#include "resource/Texture.h"
#include "rhi/Driver.h"

namespace Zephyr
{
    Handle<RHIShaderSet> MaterialInstance::GetShaderHandle() { return m_Material->GetShaderSet(); }

    void MaterialInstance::Bind(Driver* driver)
    {

        driver->BindShaderSet(GetShaderHandle());
        for (auto& texture : m_TextureNames)
        {
            auto [index, set, binding, usage] = texture.second;
            driver->BindTexture(m_Textures[index]->GetHandle(), set, binding, usage);
        }

        for (auto& constant : m_Constants)
        {
            driver->BindConstantBuffer(constant.offset, constant.size, constant.stage, m_ConstantBuffer.data());
        }
    }

    MaterialInstance::MaterialInstance(Material* material) : m_Material(material)
    {
        // textures
        m_Textures.resize(material->m_TextureCount);
        for (size_t i = 0; i < material->m_TextureCount; i++)
        {
            auto& descriptor = material->m_TextureDescriptors[i];

            auto  set     = descriptor.set;
            auto  binding = descriptor.binding;
            auto& name    = descriptor.name;
            auto  type    = descriptor.type;
            auto  usage   = descriptor.usage;

            m_TextureNames.insert({name, {i, set, binding, usage}});
        }
        // uniform blocks
        // TODO: implement

        // constant block
        uint32_t totalSize = 0;
        uint32_t maxOffset = 0;
        for (size_t i = 0; i < material->m_ConstantBlockCount; i++)
        {
            auto& descriptor  = material->m_ConstantBlockDescriptors[i];
            auto  blockOffset = descriptor.offset;
            auto& constant    = m_Constants.emplace_back();
            constant.offset   = blockOffset;
            constant.size     = descriptor.size;
            constant.stage    = descriptor.stage;
            if (blockOffset > maxOffset)
            {
                maxOffset = blockOffset;
                totalSize = blockOffset + descriptor.size;
            }

            for (auto& member : descriptor.members)
            {
                auto memberOffset = member.offset;
                m_ConstantNames.insert({member.name, {blockOffset + memberOffset, member.size}});
            }
        }
        m_ConstantBuffer.resize(totalSize);

        for (auto& material : material->m_ConstantBlockDescriptors)
        {
            memcpy(m_ConstantBuffer.data() + material.offset, material.defaultValue.data(), material.size);
        }
    }
} // namespace Zephyr