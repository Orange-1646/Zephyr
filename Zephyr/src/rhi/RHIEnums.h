#pragma once
#include "pch.h"

namespace Zephyr
{
    inline constexpr uint32_t MAX_CONCURRENT_FRAME = 2;

    enum class DriverType
    {
        Vulkan = 0,
        D3D12,
        OpenGL,
        None
    };

    enum class ShadingModel
    {
        Lit = 0 // standard pbr shading
    };
    struct BufferUsageBits
    {
        enum : uint32_t
        {
            None           = 0,
            Vertex         = 1,
            Index          = Vertex << 1,
            Uniform        = Index << 1,
            UniformDynamic = Uniform << 1,
            Storage        = UniformDynamic << 1,
            StorageDynamic = Storage << 1
        };
    };

    using BufferUsage = uint32_t;

    enum class BufferMemoryType
    {
        Static = 0,
        Dynamic,
        DynamicRing
    };

    struct DepthRange
    {
        float n = 0.f;
        float f = 1.f;
    };

    enum class PrimitiveType
    {
        Points = 0,
        Lines,
        LineStrips,
        Triangles,
        TriagleStrips,
        None
    };

    enum class Culling
    {
        None = 0,
        FrontFace,
        BackFace,
        FrontAndBack
    };

    enum class FrontFace
    {
        Clockwise = 0,
        CounterClockwise
    };

    struct TextureUsageBits
    {
        enum : uint32_t
        {
            None                   = 0,
            ColorAttachment        = 1,
            DepthStencilAttachment = ColorAttachment << 1,
            Sampled                = DepthStencilAttachment << 1,
            SampledDepthStencil    = Sampled << 1,
            Storage                = SampledDepthStencil << 1
        };
    };

    using TextureUsage = uint32_t;

    enum class TextureCubemapFace
    {
        PositiveX = 0,
        NegativeX,
        PositiveY,
        NegativeY,
        PositiveZ,
        NegativeZ
    };

    // TODO: add more format and compression format support
    enum class TextureFormat : uint32_t
    {
        DEFAULT     = 0,
        RGBA8_UNORM = 1,
        RGBA8_SRGB,
        RGBA16_UNORM,
        RGBA16_SRGB,

        DEPTH32F,
        DEPTH24_STENCIL8
    };

    enum class SamplerType : uint32_t
    {
        Sampler2D = 0,
        Sampler2DArray,
        Sampler3D,
        SamplerCubeMap,
        SamplerCubeMapArray
    };

    enum class SamplerWrap
    {
        ClampToEdge = 0,
        Repeat,
        RepeatMirror
    };

    enum class SamplerFilter
    {
        Nearest = 0,
        Linear
    };

    enum class SampleCount
    {
        SampleCount1,
        SampleCount2,
        SampleCount4,
        SampleCount8,
        SampleCount16,
        SampleCount32,
        SampleCount64
    };

    struct ShaderStageBits
    {
        enum : uint32_t
        {
            None     = 0,
            Vertex   = 1,
            Fragment = Vertex << 1,
            Compute  = Fragment << 1,
        };
    };

    using ShaderStage = uint32_t;

    enum class VertexType
    {
        None = 0,
        Static,
        Dynamic,
    };

    struct PipelineTypeBits
    {
        enum : uint32_t
        {
            None     = 0,
            Graphics = 1,
            Compute  = Graphics << 1
        };
    };

    using PipelineType = uint32_t;

    enum class DescriptorType
    {
        None = 0,
        UniformBuffer,
        UniformBufferDynamic,
        StorageBuffer,
        StorageBufferDynamic,
        CombinedImageSampler,
        StorageImage,
        PushConstant
    };

    struct PipielineStageBits
    {
        enum : uint32_t
        {
            TopOfPipe          = 1,
            BottomOfPipe       = TopOfPipe << 1,
            VertexShader       = BottomOfPipe << 1,
            FragmentShader     = VertexShader << 1,
            ComputeShader      = FragmentShader << 1,
            DepthStencilOutput = ComputeShader << 1,
            ColorOutput        = DepthStencilOutput << 1
        };
    };

    using PipelineStage = uint32_t;

    inline constexpr int32_t INVALID_DIMENSION = INT32_MAX;

    struct Viewport
    {
        uint32_t left   = INT32_MAX;
        uint32_t bottom = INT32_MAX;
        int32_t  width  = INT32_MAX;
        int32_t  height = INT32_MAX;

        bool IsValid() const
        {
            return left != INVALID_DIMENSION && bottom != INVALID_DIMENSION && width != INVALID_DIMENSION &&
                   height != INVALID_DIMENSION;
        }
    };

    struct Scissor
    {
        uint32_t x      = INT32_MAX;
        uint32_t y      = INT32_MAX;
        uint32_t width  = INT32_MAX;
        uint32_t height = INT32_MAX;

        bool IsValid() const
        {
            return x != INVALID_DIMENSION && y != INVALID_DIMENSION && width != INVALID_DIMENSION &&
                   height != INVALID_DIMENSION;
        }
    };

    enum class BindingType
    {
        None,
        StorageBuffer,
        StorageBufferDynamic,
        UniformBuffer,
        UniformBufferDynamic,
        StorageImage,
        Sampler2D,
        Sampler2DArray,
        SamplerCubemap,
    };

    struct RasterState
    {
        Culling   cull;
        FrontFace frontFace;
        bool      depthTestEnabled;
        bool      depthWriteEnabled;

        bool operator==(const RasterState& rhs) const
        {
            return rhs.cull == cull && rhs.frontFace == frontFace && depthTestEnabled == rhs.depthTestEnabled &&
                   depthWriteEnabled == rhs.depthWriteEnabled;
        }
    };
} // namespace Zephyr