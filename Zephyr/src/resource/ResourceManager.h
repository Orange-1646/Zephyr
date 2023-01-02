#pragma once
#include "ModelLoader.h"
#include "TextureLoader.h"
#include "core/macro.h"
#include "pch.h"
#include "rhi/RHIEnums.h"

namespace Zephyr
{
    struct BoxMeshDescription;
    struct TextureDescription;
    struct BufferDescription;
    struct MaterialDescription;

    class Engine;
    class Texture;
    class Buffer;
    class Mesh;
    class Material;
    class MaterialInstance;
    class ShaderSet;
    class Driver;

    class ResourceManager final
    {
    public:
        ResourceManager(Engine* engine);
        void InitResources(Driver* driver);
        void Shutdown();
        ~ResourceManager();
        DISALE_COPY_AND_MOVE(ResourceManager);

        // create empty texture
        Texture* CreateTexture(const TextureDescription& desc);
        // create samplable texture2d and load image into it
        Texture* CreateTexture(const std::string& path, bool srgb = false, bool flip = true);
        // create samplable texturecubemap and load image into it
        Texture* CreateTexture(std::string path[6]);

        Buffer*           CreateBuffer(const BufferDescription& desc);
        Mesh*             CreateMesh(const std::string& path);
        Mesh*             CreateMesh(const BoxMeshDescription& desc);
        MaterialInstance* CreateMaterialInstance(ShadingModel shading);

        Material* GetMaterial(const std::string& name);

        ShaderSet* GetShaderSet(const std::string& name);

        inline Texture* GetWhiteTexture() const { return m_WhiteTexture; }
        inline Texture* GetBlackTexture() const { return m_BlackTexture; }
        inline Texture* GetDefaultSkybox() const { return m_DefaultSkybox; }
        inline Texture* GetDefaultPrefilteredEnv() const { return m_DefaultPrefilterEnv; }
        inline Texture* GetBRDFLut() const { return m_BrdfLut; }
        inline Texture* GetDither() const { return m_DitherTexture; }
        inline Mesh*    GetSkyboxMesh() { return m_SkyboxMesh; }

    private:
        void InitShaderSets(Driver* driver);
        void InitMaterials(Driver* driver);
        void InitDefaultTextures(Driver* driver);

    private:
        Engine* m_Engine;

        std::vector<Texture*>                       m_Textures;
        std::vector<Buffer*>                        m_Buffers;
        std::vector<Mesh*>                          m_Meshes;
        std::unordered_map<std::string, ShaderSet*> m_ShaderSets;
        std::unordered_map<std::string, Material*>  m_Materials;
        std::vector<MaterialInstance*>              m_MaterialInstances;
        // this is just cache, m_Textures owns the texture;
        std::unordered_map<std::string, Texture*> m_TexturePathMap;
        Mesh*                                     m_SkyboxMesh = nullptr;

        Texture* m_DefaultSkybox = nullptr;
        Texture* m_DefaultPrefilterEnv = nullptr;
        Texture* m_BrdfLut             = nullptr;
        Texture* m_WhiteTexture  = nullptr;
        Texture* m_BlackTexture  = nullptr;
        Texture* m_DitherTexture = nullptr;

        ModelLoader   m_ModelLoader;
        TextureLoader m_TextureLoader;

        friend class Engine;
        friend class ModelLoader;
        friend class TextureLoader;
    };
} // namespace Zephyr