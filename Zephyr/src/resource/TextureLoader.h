#pragma once
#include "pch.h"

namespace Zephyr
{
    class ResourceManager;
    class Texture;

    // note that texture loader doesn't own any texture.
    class TextureLoader
    {
    public:
    private:
        TextureLoader(ResourceManager* manager);
        ~TextureLoader();

        Texture* Load(const std::string& path, bool srgb = false, bool flip = true);
        Texture* Load(std::string path[6]);
        Texture* LoadEnv(const std::string& path);
    private:
        ResourceManager* m_Manager;
        std::unordered_map<std::string, Texture*> m_TextureCache;

        friend class ResourceManager;
    };
}