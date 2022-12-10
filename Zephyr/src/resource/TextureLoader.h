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

        Texture* Load(const std::string& path);
        Texture* Load(std::string path[6]);
    private:
        ResourceManager* m_Manager;
        std::unordered_map<std::string, Texture*> m_TextureCache;

        friend class ResourceManager;
    };
}