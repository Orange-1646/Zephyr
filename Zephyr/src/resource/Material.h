#pragma once
#include "pch.h"
#include "rhi/Handle.h"
#include "rhi/RHIEnums.h"

namespace Zephyr
{
    class Texture;
    class RHIShaderSet;

    // A material is a collection of descriptors for the resources it needs
    // There are 3 kind of resources in total: texture, uniform buffer, constant buffer(push constant in vulkan)
    // note that for materials we reserve set = 0 binding = 0 for global render data

    struct TextureDescriptor
    {
        std::string  name;
        uint32_t     set;
        uint32_t     binding;
        TextureUsage usage;
        SamplerType  type;
    };

    struct UniformBlockMemberDescriptor
    {
        std::string name;
        uint32_t    size;
        uint32_t    offset;
        uint32_t    length;
    };

    struct UniformBlockDescriptor
    {
        std::string name;
        uint32_t    set;
        uint32_t    binding;
        // total size of the block in bytes
        uint32_t size;
        // if it's a uniform buffer array, length = array.size. otherwise length = 1
        uint32_t                                  length;
        std::vector<UniformBlockMemberDescriptor> members;
    };

    struct ConstantBlockMemberDescriptor
    {
        std::string name;
        uint32_t    size;
        uint32_t    offset;
    };

    struct ConstantBlockDescriptor
    {
        std::string                                name;
        uint32_t                                   size;
        uint32_t                                   offset;
        ShaderStage                                stage;
        std::vector<ConstantBlockMemberDescriptor> members;
        std::vector<uint8_t>                       defaultValue;
    };

    class Material
    {
    public:
        inline Handle<RHIShaderSet> GetShaderSet() { return m_ShaderSet; }

    private:
        Material(Handle<RHIShaderSet> shader) : m_ShaderSet(shader) {}
        virtual ~Material() = default;

        uint32_t m_TextureCount;
        // TODO: implement runtime fixed-size vector
        std::vector<TextureDescriptor>       m_TextureDescriptors;
        uint32_t                             m_UniformBlockCount = 0;
        std::vector<UniformBlockDescriptor>  m_UniformBlockDescriptors;
        uint32_t                             m_ConstantBlockCount = 0;
        std::vector<ConstantBlockDescriptor> m_ConstantBlockDescriptors;

        Handle<RHIShaderSet> m_ShaderSet;

        friend class ResourceManager;
        friend class MaterialInstance;
    };
} // namespace Zephyr