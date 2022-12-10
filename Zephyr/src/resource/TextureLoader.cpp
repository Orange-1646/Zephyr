#include "TextureLoader.h"
#include "Texture.h"
#include "rhi/RHIEnums.h"
#include "ResourceManager.h"
#include "engine/Engine.h"
#include <stb_image.h>

namespace Zephyr
{
    TextureLoader::TextureLoader(ResourceManager* manager) : m_Manager(manager) {}

    TextureLoader::~TextureLoader() {}

    Texture* TextureLoader::Load(const std::string& path) { 
        auto iter = m_TextureCache.find(path);

        if (iter != m_TextureCache.end())
        {
            return iter->second;
        }

        // load image data
        int width, height, channels;
        uint8_t* imageData = nullptr;
        uint32_t imageSize = 0;
        TextureFormat imageFormat = TextureFormat::RGBA8_UNORM;

        // TODO: support hdr texturee
        //if (stbi_is_hdr(path.c_str()))
        //{
        //    imageData = (uint8_t*)stbi_loadf(path.c_str(), &width, &height, &channels, 4);
        //}
        stbi_set_flip_vertically_on_load(true);
        imageData   = stbi_load(path.c_str(), &width, &height, &channels, 4);
        imageSize = width * height * 4;

        assert(imageData);

        // create texture
        TextureDescription textureDesc {};
        textureDesc.width = width;
        textureDesc.height = height;
        textureDesc.depth  = 1;
        textureDesc.format = imageFormat;
        // right now we assume all external textures are consumed by graphics pipeline.
        // this is not always true, and we'll change it if needed
        textureDesc.pipelines = PipelineTypeBits::Graphics;
        textureDesc.sampler   = SamplerType::Sampler2D;
        textureDesc.usage     = TextureUsageBits::Sampled;
        textureDesc.levels    = 1;
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

    Texture* TextureLoader::Load(std::string path[6]) {
        // load
        uint8_t* imageData[6] = {};
        uint32_t imageSize[6] = {};
        int width[6]     = {};
        int height[6]    = {};
        int channels[6]  = {};

        TextureFormat format = TextureFormat::RGBA8_UNORM;

        for (uint32_t i = 0; i < 6; i++)
        {
            //stbi_set_flip_vertically_on_load(true);
            imageData[i] = stbi_load(path[i].data(), &width[i], &height[i], &channels[i], 4);
            imageSize[i] = width[i] * height[i] * 4;
        }

        // create texture
        // assume that all textures are of the same size. this should always be the case for cubemap
        TextureDescription textureDesc {};
        textureDesc.width = width[0];
        textureDesc.height = height[0];
        textureDesc.depth  = 1;
        textureDesc.levels = 1;
        textureDesc.format = format;
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
} // namespace Zephyr