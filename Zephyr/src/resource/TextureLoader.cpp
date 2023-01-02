#include "TextureLoader.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "engine/Engine.h"
#include "rhi/RHIEnums.h"
#include <stb_image.h>

namespace Zephyr
{
    TextureLoader::TextureLoader(ResourceManager* manager) : m_Manager(manager) {}

    TextureLoader::~TextureLoader() {}

    Texture* TextureLoader::Load(const std::string& path, bool srgb, bool flip)
    {
        auto iter = m_TextureCache.find(path);

        if (iter != m_TextureCache.end())
        {
            return iter->second;
        }

        // load image data
        int           width, height, channels;
        uint8_t*      imageData   = nullptr;
        uint32_t      imageSize   = 0;
        TextureFormat imageFormat = srgb ? TextureFormat::RGBA8_SRGB : TextureFormat::RGBA8_UNORM;

        if (flip)
        {
            stbi_set_flip_vertically_on_load(true);
        }
        else
        {
            stbi_set_flip_vertically_on_load(false);
        }
        // TODO: support hdr texturee
        if (stbi_is_hdr(path.c_str()))
        {
            imageData = (uint8_t*)stbi_loadf(path.c_str(), &width, &height, &channels, 4);
            imageSize   = width * height * 4 * 4;
            imageFormat = TextureFormat::RGBA32_SFLOAT;
        }
        else
        {
            imageData = stbi_load(path.c_str(), &width, &height, &channels, 4);
            imageSize = width * height * 4;
        }

        assert(imageData);

        // create texture
        TextureDescription textureDesc {};
        textureDesc.width  = width;
        textureDesc.height = height;
        textureDesc.depth  = 1;
        textureDesc.format = imageFormat;
        // right now we assume all external textures are consumed by graphics pipeline.
        // this is not always true, and we'll change it if needed
        textureDesc.pipelines = PipelineTypeBits::Graphics;
        textureDesc.sampler   = SamplerType::Sampler2D;
        textureDesc.usage     = TextureUsageBits::Sampled;
        textureDesc.levels    = 1;
        textureDesc.levels    = log2(std::max(width, height)) + 1;
        // TODO: support multisampling
        textureDesc.samples = 1;

        Texture* texture = m_Manager->CreateTexture(textureDesc);

        // update
        TextureUpdateDescriptor update {};
        update.layer = 0;
        update.depth = 1;
        update.data  = imageData;
        update.size  = imageSize;

        texture->Update(update, m_Manager->m_Engine->GetDriver());

        m_TextureCache.insert({path, texture});

        stbi_image_free(imageData);

        return texture;
    }

    Texture* TextureLoader::Load(std::string path[6])
    {
        // load
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
        textureDesc.levels    = 6;
        textureDesc.format    = format;
        textureDesc.pipelines = PipelineTypeBits::Graphics;
        textureDesc.sampler   = SamplerType::SamplerCubeMap;
        textureDesc.samples   = 1;
        textureDesc.usage     = TextureUsageBits::Sampled;

        Texture* texture = m_Manager->CreateTexture(textureDesc);

        std::string p;
        for (uint32_t i = 0; i < 6; i++)
        {
            TextureUpdateDescriptor update {};
            update.layer = i;
            update.depth = 1;
            update.data  = imageData[i];
            update.size  = imageSize[i];

            texture->Update(update, m_Manager->m_Engine->GetDriver());

            stbi_image_free(imageData[i]);
            p += path[i];
        }

        m_TextureCache.insert({p, texture});

        return texture;
    }

    Texture* TextureLoader::LoadEnv(const std::string& path)
    {

        std::ifstream   file(path, std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> data(size);

        if (!file.read((char*)data.data(), size))
        {
            printf("Error Loading Shader: %s", path.c_str());
            assert(false);
        }

        for (uint32_t i = 0; i < 1; i++)
        {
            std::cout << (uint32_t)*data.data() << std::endl;
        }

        uint32_t width0  = *(uint32_t*)data.data();
        uint32_t height0 = width0;

        TextureDescription textureDesc {};
        textureDesc.width  = width0;
        textureDesc.height = height0;
        textureDesc.depth  = 1;
        textureDesc.format = TextureFormat::RGBA8_UNORM;
        // right now we assume all external textures are consumed by graphics pipeline.
        // this is not always true, and we'll change it if needed
        textureDesc.pipelines = PipelineTypeBits::Graphics;
        textureDesc.sampler   = SamplerType::SamplerCubeMap;
        textureDesc.usage     = TextureUsageBits::Sampled;
        textureDesc.levels    = 5;
        // TODO: support multisampling
        textureDesc.samples = 1;

        Texture* texture = m_Manager->CreateTexture(textureDesc);

        uint32_t offset = 4;
        for (uint32_t level = 0; level < 5; level++)
        {
            uint32_t width  = width0 / pow(2, level);
            uint32_t height = width;

            uint32_t size = width * height * 4;
            for (uint32_t layer = 0; layer < 6; layer++)
            {
                TextureUpdateDescriptor update {};
                update.layer = layer;
                update.level = level;
                update.depth = 1;
                update.data  = data.data() + offset;
                update.size  = size;

                texture->Update(update, m_Manager->m_Engine->GetDriver());

                offset += size;
            }
        }

        m_TextureCache.insert({path, texture});

        return texture;
    }
} // namespace Zephyr