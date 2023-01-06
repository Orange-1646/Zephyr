#include "Executor.h"
#include "rhi/vulkan/VulkanShaderSet.h"
#include "rhi/vulkan/VulkanUtil.h"
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

namespace Zephyr
{
    std::vector<uint32_t> LoadShaderFromFile(const std::string& path)
    {
        std::ifstream   file(path, std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint32_t> data(size / 4);

        if (!file.read((char*)data.data(), size))
        {
            printf("Error Loading Shader: %s", path.c_str());
            assert(false);
        }

        return data;
    }

    SMExecutor::SMExecutor() { m_Driver = new VulkanDriver(nullptr, true); }
    SMExecutor::~SMExecutor()
    {
        m_Driver->DestroyTexture(m_Cubemap);
        m_Driver->DestroyTexture(m_Target);
        m_Driver->DestroyShaderSet(m_Shader);
        m_Driver->WaitIdle();
        delete m_Driver;
    }

    bool SMExecutor::Run()
    {
        LoadCubemap();
        CreateComputeTarget();
        CreatePipeline();
        DispatchCompute();
        Save();

        return true;
    }

    void SMExecutor::LoadCubemap()
    {
        std::string path[6] = {"D:/Programming/ZephyrEngine/asset/cubemap/Yokohama3/posx.jpg",
                               "D:/Programming/ZephyrEngine/asset/cubemap/Yokohama3/negx.jpg",
                               "D:/Programming/ZephyrEngine/asset/cubemap/Yokohama3/posy.jpg",
                               "D:/Programming/ZephyrEngine/asset/cubemap/Yokohama3/negy.jpg",
                               "D:/Programming/ZephyrEngine/asset/cubemap/Yokohama3/posz.jpg",
                               "D:/Programming/ZephyrEngine/asset/cubemap/Yokohama3/negz.jpg"};

        uint8_t* imageData[6] = {};
        uint32_t imageSize[6] = {};
        int      width[6]     = {};
        int      height[6]    = {};
        int      channels[6]  = {};

        TextureFormat format = TextureFormat::RGBA8_UNORM;

        for (uint32_t i = 0; i < 6; i++)
        {
            // stbi_set_flip_vertically_on_load(true);
            imageData[i] = stbi_load(path[i].data(), &width[i], &height[i], &channels[i], 4);
            imageSize[i] = width[i] * height[i] * 4;
        }

        // create texture
        // assume that all textures are of the same size. this should always be the case for cubemap
        TextureDescription textureDesc {};
        textureDesc.width     = width[0];
        textureDesc.height    = height[0];
        textureDesc.depth     = 1;
        textureDesc.levels    = 10;
        textureDesc.format    = format;
        textureDesc.pipelines = PipelineTypeBits::Graphics | PipelineTypeBits::Compute;
        textureDesc.sampler   = SamplerType::SamplerCubeMap;
        textureDesc.samples   = 1;
        textureDesc.usage     = TextureUsageBits::Sampled;

        m_Cubemap = m_Driver->CreateTexture(textureDesc);

        std::string p;
        for (uint32_t i = 0; i < 6; i++)
        {
            TextureUpdateDescriptor update {};
            update.layer = i;
            update.depth = 1;
            update.data  = imageData[i];
            update.size  = imageSize[i];

            m_Driver->UpdateTexture(update, m_Cubemap);

            stbi_image_free(imageData[i]);
            p += path[i];
        }

        m_Driver->GenerateMips(m_Cubemap);
    }

    void SMExecutor::CreateComputeTarget()
    {
        TextureDescription textureDesc {};
        textureDesc.width     = 256;
        textureDesc.height    = 256;
        textureDesc.depth     = 1;
        textureDesc.levels    = 5;
        textureDesc.format    = TextureFormat::RGBA8_UNORM;
        textureDesc.pipelines = PipelineTypeBits::Graphics | PipelineTypeBits::Compute;
        textureDesc.sampler   = SamplerType::SamplerCubeMap;
        textureDesc.samples   = 1;
        textureDesc.usage     = TextureUsageBits::Storage;

        m_Target = m_Driver->CreateTexture(textureDesc);

        auto r = m_Driver->GetResource<VulkanTexture>(m_Target);

        auto cb = m_Driver->BeginSingleTimeCommandBuffer();
        r->TransitionLayout(cb, 1, 0, 6, 0, 5, VK_IMAGE_LAYOUT_GENERAL);
        m_Driver->EndSingleTimeCommandBuffer(cb);
    }

    void SMExecutor::CreatePipeline()
    {
        ShaderSetDescription shaderDesc {};
        // lit shader
        shaderDesc.pipeline = PipelineTypeBits::Compute;
        shaderDesc.compute  = LoadShaderFromFile("D:/Programming/ZephyrEngine/asset/shader/spv/specularMap.comp.spv");

        m_Shader = m_Driver->CreateShaderSet(shaderDesc);
    }

    void SMExecutor::DispatchCompute()
    {
        m_Driver->BindShaderSet(m_Shader);
        m_Driver->BindTexture(m_Cubemap, 0, 1, TextureUsageBits::Sampled);
        for (uint32_t i = 0; i < 5; i++)
        {
            ViewRange range {};
            range.baseLayer  = 0;
            range.layerCount = 6;
            range.baseLevel  = i;
            range.levelCount = 1;

            m_Driver->BindTexture(m_Target, range, 0, 0, TextureUsageBits::Storage);

            float roughness = float(i) / 4.f;

            m_Driver->BindConstantBuffer(0, 4, ShaderStageBits::Compute, &roughness);

            uint32_t x = 256 / 16 / pow(2, i);
            uint32_t y = x;

            m_Driver->Dispatch(x, y, 6);
        }
        m_Driver->SubmitJobCompute(false);
        m_Driver->WaitIdle();
    }

    void SMExecutor::Save()
    {
        auto& context        = *m_Driver->GetContext();
        auto  physicalDevice = context.PhysicalDevice();
        auto  device         = context.Device();

        auto cb = m_Driver->BeginSingleTimeCommandBuffer();
        auto r  = m_Driver->GetResource<VulkanTexture>(m_Target);
        r->TransitionLayout(cb, 0, 0, 6, 0, 5, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        // 6 faces with size 256x256, each face with 5 mip levels
        // (256 * 256 + 512 * 512 + 256 * 256 + 128 * 128 + 64 * 64) * 32
        uint32_t totalSize = 5586944 * 6;

        BufferDescription desc {};
        desc.size         = totalSize;
        desc.usage        = BufferUsageBits::None;
        desc.memoryType   = BufferMemoryType::Dynamic;
        desc.shaderStages = ShaderStageBits::Compute;
        desc.pipelines    = PipelineTypeBits::Compute;

        auto buf = m_Driver->CreateBuffer(desc);
        auto b   = m_Driver->GetResource<VulkanBuffer>(buf);

        uint32_t                       offset = 0;
        std::vector<VkBufferImageCopy> copies;
        for (uint32_t level = 0; level < 5; level++)
        {
            uint32_t width  = 256 / pow(2, level);
            uint32_t height = width;

            VkBufferImageCopy copy {};
            copy.bufferOffset                    = offset;
            copy.bufferRowLength                 = 0;
            copy.imageOffset.x                   = 0;
            copy.imageOffset.y                   = 0;
            copy.imageOffset.z                   = 0;
            copy.imageExtent.width               = 256 / pow(2, level);
            copy.imageExtent.height              = 256 / pow(2, level);
            copy.imageExtent.depth               = 1;
            copy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            copy.imageSubresource.mipLevel       = level;
            copy.imageSubresource.baseArrayLayer = 0;
            copy.imageSubresource.layerCount     = 6;
            copies.push_back(copy);

            offset += width * height * 4 * 6;
        }

        vkCmdCopyImageToBuffer(
            cb, r->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, b->GetBuffer(), copies.size(), copies.data());

        m_Driver->EndSingleTimeCommandBuffer(cb);

        auto vkb = b->GetBuffer();
        auto vkm = b->GetMemory();

        uint8_t*    data     = (uint8_t*)b->GetMapped();
        std::string fileName = "D:/Programming/ZephyrEngine/ZephyrTool/SpecularMapGenerator/result/Indoor/specularMap";

        uint32_t o = 0;
        for (uint32_t level = 0; level < 5; level++)
        {
            for (uint32_t layer = 0; layer < 6; layer++)
            {
                uint32_t width  = 256 / pow(2, level);
                uint32_t height = width;

                std::string fn = fileName + std::to_string(layer) + std::to_string(level) + ".jpg";

                stbi_write_jpg(fn.c_str(), width, height, 4, data + o, 100);
                std::cout << "Screenshot saved to disk" << std::endl;
                o += width * height * 4;
            }
        }

        std::ofstream fp;
        fp.open("D:/Programming/ZephyrEngine/ZephyrTool/SpecularMapGenerator/result/Yokohama3.bin",
                std::ios::out | std::ios::binary);
        uint32_t size = 256;
        fp.write((char*)&size, sizeof(size));
        assert(fp.tellp() == 4);
        fp.write((char*)data, totalSize);
        fp.close();

        m_Driver->DestroyBuffer(buf);
    }

} // namespace Zephyr