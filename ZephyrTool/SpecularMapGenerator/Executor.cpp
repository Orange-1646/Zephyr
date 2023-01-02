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
        //std::string path[6] = {"D:/Programming/ZephyrEngine/asset/cubemap/Yokohama3/posx.jpg",
        //                       "D:/Programming/ZephyrEngine/asset/cubemap/Yokohama3/negx.jpg",
        //                       "D:/Programming/ZephyrEngine/asset/cubemap/Yokohama3/posy.jpg",
        //                       "D:/Programming/ZephyrEngine/asset/cubemap/Yokohama3/negy.jpg",
        //                       "D:/Programming/ZephyrEngine/asset/cubemap/Yokohama3/posz.jpg",
        //                       "D:/Programming/ZephyrEngine/asset/cubemap/Yokohama3/negz.jpg"};
        std::string path[6] = {"D:/Programming/ZephyrEngine/asset/cubemap/Indoor/posx.jpg",
                               "D:/Programming/ZephyrEngine/asset/cubemap/Indoor/negx.jpg",
                               "D:/Programming/ZephyrEngine/asset/cubemap/Indoor/posy.jpg",
                               "D:/Programming/ZephyrEngine/asset/cubemap/Indoor/negy.jpg",
                               "D:/Programming/ZephyrEngine/asset/cubemap/Indoor/posz.jpg",
                               "D:/Programming/ZephyrEngine/asset/cubemap/Indoor/negz.jpg"};

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
        fp.open("D:/Programming/ZephyrEngine/ZephyrTool/SpecularMapGenerator/result/Indoor.bin",
                std::ios::out | std::ios::binary);
        uint32_t size = 256;
        fp.write((char*)&size, sizeof(size));
        assert(fp.tellp() == 4);
        fp.write((char*)data, totalSize);
        fp.close();

        m_Driver->DestroyBuffer(buf);
        // VkImage image;
        //// create linear tiling image and blit from src to this
        //{
        //    VkImageCreateInfo createInfo {};
        //    createInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        //    createInfo.imageType     = VK_IMAGE_TYPE_2D;
        //    createInfo.format        = VK_FORMAT_R8G8B8A8_UNORM;
        //    createInfo.extent.width  = 256;
        //    createInfo.extent.height = 256;
        //    createInfo.extent.depth  = 1;
        //    createInfo.mipLevels     = 1;
        //    createInfo.arrayLayers   = 1;
        //    createInfo.samples               = VK_SAMPLE_COUNT_1_BIT;
        //    createInfo.tiling                = VK_IMAGE_TILING_LINEAR;
        //    createInfo.usage                 = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        //    createInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
        //    //createInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        //    VK_CHECK(vkCreateImage(context.Device(), &createInfo, nullptr, &image), "Image Creation");
        //}
        // VkDeviceMemory mem;
        //// allocate and bind memory
        //{
        //    VkMemoryRequirements memoryRequirements;
        //    vkGetImageMemoryRequirements(context.Device(), image, &memoryRequirements);

        //    uint32_t requiredSize = memoryRequirements.size;
        //    uint32_t memoryTypeIndex =
        //        VulkanUtil::FindMemoryTypeIndex(context.PhysicalDevice(),
        //                                        memoryRequirements.memoryTypeBits,
        //                                        VulkanUtil::GetBufferMemoryProperties(BufferMemoryType::Dynamic));

        //    VkMemoryAllocateInfo allocInfo {};
        //    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        //    allocInfo.allocationSize  = requiredSize;
        //    allocInfo.memoryTypeIndex = memoryTypeIndex;

        //    VK_CHECK(vkAllocateMemory(context.Device(), &allocInfo, nullptr, &mem), "Texture Memory Allocation");

        //    vkBindImageMemory(context.Device(), image, mem, 0);
        //}
        // auto cb = m_Driver->BeginSingleTimeCommandBuffer();

        // transition to transfer dst optimal
        //{
        //    VkImageMemoryBarrier barrier {};
        //    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        //    barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        //    barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        //    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        //    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        //    barrier.image                           = image;
        //    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        //    barrier.subresourceRange.baseArrayLayer = 0;
        //    barrier.subresourceRange.layerCount     = 1;
        //    barrier.subresourceRange.baseMipLevel   = 0;
        //    barrier.subresourceRange.levelCount     = 1;
        //    barrier.srcAccessMask                   = 0;
        //    barrier.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;

        //    vkCmdPipelineBarrier(cb,
        //                         VK_PIPELINE_STAGE_TRANSFER_BIT,
        //                         VK_PIPELINE_STAGE_TRANSFER_BIT,
        //                         0,
        //                         0,
        //                         nullptr,
        //                         0,
        //                         nullptr,
        //                         1,
        //                         &barrier);
        //}
        // transition to transfer src optimal
        // auto r = m_Driver->GetResource<VulkanTexture>(m_Target);
        // r->TransitionLayout(cb, 0, 0, 6, 0, 5, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        // bool supportsBlit = true;

        //// Check blit support for source and destination
        // VkFormatProperties formatProps;

        //// Check if the device supports blitting from optimal images (the swapchain images are in optimal format)
        // vkGetPhysicalDeviceFormatProperties(physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
        // if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
        //{
        //     std::cerr << "Device does not support blitting from optimal tiled images, using copy instead of
        //     blit!"
        //               << std::endl;
        //     supportsBlit = false;
        // }

        //// Check if the device supports blitting to linear images
        // vkGetPhysicalDeviceFormatProperties(physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
        // if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT))
        //{
        //     std::cerr << "Device does not support blitting to linear tiled images, using copy instead of blit!"
        //               << std::endl;
        //     supportsBlit = false;
        // }

        // if (supportsBlit)
        //{
        //     // Define the region to blit (we will blit the whole swapchain image)
        //     VkOffset3D blitSize;
        //     blitSize.x = 256;
        //     blitSize.y = 256;
        //     blitSize.z = 1;

        //    std::vector<VkImageBlit> regions;

        //    for (uint32_t i = 0; i < 6; i++)
        //    {
        //        VkImageBlit imageBlitRegion {};
        //        imageBlitRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        //        imageBlitRegion.srcSubresource.mipLevel       = i;
        //        imageBlitRegion.srcSubresource.baseArrayLayer = 0;
        //        imageBlitRegion.srcSubresource.layerCount     = 6;
        //        imageBlitRegion.srcOffsets[1]                 = blitSize;

        //        imageBlitRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        //        imageBlitRegion.dstSubresource.baseArrayLayer = 0;
        //        imageBlitRegion.dstSubresource.layerCount     = 6;
        //        imageBlitRegion.dstSubresource.mipLevel       = i;
        //        imageBlitRegion.dstOffsets[1]                 = blitSize;

        //        regions.push_back(imageBlitRegion);
        //    }

        //    // Issue the blit command
        //    vkCmdBlitImage(cb,
        //                   r->GetImage(),
        //                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        //                   image,
        //                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        //                   regions.size(),
        //                   regions.data(),
        //                   VK_FILTER_NEAREST);
        //}
        // else
        //{

        //    std::vector<VkImageCopy> regions;

        //    for (uint32_t i = 0; i < 1; i++)
        //    {
        //        // Otherwise use image copy (requires us to manually flip components)
        //        VkImageCopy imageCopyRegion {};
        //        imageCopyRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        //        imageCopyRegion.srcSubresource.baseArrayLayer = 0;
        //        imageCopyRegion.srcSubresource.layerCount     = 1;
        //        imageCopyRegion.srcSubresource.mipLevel       = 0;

        //        imageCopyRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        //        imageCopyRegion.dstSubresource.baseArrayLayer = 0;
        //        imageCopyRegion.dstSubresource.layerCount     = 1;
        //        imageCopyRegion.dstSubresource.mipLevel       = 0;

        //        imageCopyRegion.extent.width                  = 256;
        //        imageCopyRegion.extent.height                 = 256;
        //        imageCopyRegion.extent.depth                  = 1;

        //        regions.push_back(imageCopyRegion);
        //    }

        //    // Issue the copy command
        //    vkCmdCopyImage(cb,
        //                   r->GetImage(),
        //                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        //                   image,
        //                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        //                   regions.size(),
        //                   regions.data());
        //}
        // transition to general layout for readback
        //{
        //    VkImageMemoryBarrier barrier {};
        //    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        //    barrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        //    barrier.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
        //    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        //    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        //    barrier.image                           = image;
        //    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        //    barrier.subresourceRange.baseArrayLayer = 0;
        //    barrier.subresourceRange.layerCount     = 1;
        //    barrier.subresourceRange.baseMipLevel   = 0;
        //    barrier.subresourceRange.levelCount     = 1;
        //    barrier.srcAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
        //    barrier.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;

        //    vkCmdPipelineBarrier(cb,
        //                         VK_PIPELINE_STAGE_TRANSFER_BIT,
        //                         VK_PIPELINE_STAGE_TRANSFER_BIT,
        //                         0,
        //                         0,
        //                         nullptr,
        //                         0,
        //                         nullptr,
        //                         1,
        //                         &barrier);
        //}
        // m_Driver->EndSingleTimeCommandBuffer(cb);

        // read from each mip and write to file
        // std::string fileName = "D:/Programming/ZephyrEngine/ZephyrTool/SpecularMapGenerator/result/specularMap";

        // for (uint32_t layer = 0; layer < 1; layer++)
        //{
        //     for (uint32_t level = 0; level < 1; level++)
        //     {
        //         uint32_t width  = 256 / (pow(2, level));
        //         uint32_t height = width;

        //        VkImageSubresource  subResource {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
        //        VkSubresourceLayout subResourceLayout;
        //        vkGetImageSubresourceLayout(device, image, &subResource, &subResourceLayout);

        //        // Map image memory so we can start copying from it
        //        const uint8_t* data;
        //        vkMapMemory(device, mem, 0, VK_WHOLE_SIZE, 0, (void**)&data);
        //        data += subResourceLayout.offset;

        //        // save

        //        std::string fn = fileName + std::to_string(layer) + std::to_string(level) + ".jpg";

        //        stbi_write_jpg(fn.c_str(), width, height, 4, data, 100);

        //        // std::ofstream file(fileName + std::to_string(layer) + std::to_string(level) + ".ppm",
        //        //                    std::ios::out | std::ios::binary);

        //        //// ppm header
        //        // file << "P3\n" << width << "\n" << height << "\n" << 255 << "\n";

        //        //// If source is BGR (destination is always RGB) and we can't use blit (which does automatic
        //        /// conversion), / we'll have to manually swizzle color components / Check if source is BGR / Note:
        //        Not
        //        /// complete, only contains most common and basic BGR surface formats for demonstration / purposes

        //        //// ppm binary pixel data
        //        // for (uint32_t y = 0; y < height; y++)
        //        //{
        //        //     unsigned int* row = (unsigned int*)data;
        //        //     for (uint32_t x = 0; x < width; x++)
        //        //     {
        //        //         file.write((char*)row, 3);
        //        //         row++;
        //        //     }
        //        //     data += subResourceLayout.rowPitch;
        //        // }
        //        // file.close();

        //        std::cout << "Screenshot saved to disk" << std::endl;

        //        // Clean up resources
        //        vkUnmapMemory(device, mem);
        //        vkFreeMemory(device, mem, nullptr);
        //        vkDestroyImage(device, image, nullptr);
        //    }
        //}
    }

} // namespace Zephyr