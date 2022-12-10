#pragma once
#include "Material.h"
#include "rhi/RHIEnums.h"
#include "rhi/Handle.h"

namespace Zephyr
{
    class Driver;
    class RHIShaderSet;
    struct ConstantDescriptor
    {
        uint32_t offset;
        uint32_t size;
        ShaderStage stage;
    };

    // material instance allocate actual memory blocks based on material description
    class MaterialInstance
    {
    public:
        void SetTexture(const std::string& name, Texture* texture)
        {
            auto iter = m_TextureNames.find(name);

            if (iter == m_TextureNames.end())
            {
                return;
            }

            auto index = std::get<0>(iter->second);

            m_Textures[index] = texture;
        }

        template<typename T>
        void SetConstantBlock(const std::string& name, const T& data)
        {
            auto iter = m_ConstantNames.find(name);
            if (iter == m_ConstantNames.end())
            {
                return;
            }

            auto [offset, size] = iter->second;

            assert(size >= sizeof(T));

            memcpy(m_ConstantBuffer.data() + offset, &data, size);
        }

        Handle<RHIShaderSet> GetShaderHandle();

        void Bind(Driver* driver);
    private:
        MaterialInstance(Material* material);
        ~MaterialInstance() = default;

    private:
        Material* m_Material;
        //-------texture mapping
        // name ->index && set && binding
        std::unordered_map<std::string, std::tuple<uint32_t, uint32_t, uint32_t, TextureUsage>> m_TextureNames;
        std::vector<Texture*>                                                     m_Textures;

        //------------constant buffer mapping
        // name ->  offset && size
        std::vector<ConstantDescriptor>                                                                m_Constants;
        std::unordered_map<std::string, std::tuple<uint32_t, uint32_t>> m_ConstantNames;
        std::vector<uint8_t>                                          m_ConstantBuffer;
        friend class ResourceManager;
    };
} // namespace Zephyr