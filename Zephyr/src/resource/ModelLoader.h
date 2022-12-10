#pragma once
#include "pch.h"
#include <glm/mat4x4.hpp>

struct aiNode;
namespace Zephyr
{
    class Mesh;
    class Texture;
    class ResourceManager;
    class ModelLoader final
    {
    public:
        Mesh* LoadModel(const std::string& path);
    private: 
        ModelLoader(ResourceManager* manager);
        ~ModelLoader() = default;

        void TraverseNodes(
            Mesh*                                                    mesh,
            aiNode*                                                  node,
            std::unordered_map<aiNode*, std::vector<uint32_t>>& nodeMap, const glm::mat4& parentTransform =
                glm::mat4(1.0f),
            uint32_t level = 0);

    private:
        ResourceManager* m_ResourceManager;
        // model loader doesn't own the resources it loads, it just keeps the reference for caching purpose
        std::unordered_map<std::string, Mesh*> m_MeshCache;
        std::unordered_map<std::string, Texture*> m_Texture;

        friend class ResourceManager;
    };
}