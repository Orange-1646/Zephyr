#include "ResourceManager.h"
#include "Material.h"
#include "MaterialInstance.h"
#include "Mesh.h"
#include "ShaderSet.h"
#include "Texture.h"
#include "engine/Engine.h"
#include "platform/Path.h"
#include "preset/geometry/BoxMesh.h"
#include "rhi/Driver.h"

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

    ResourceManager::ResourceManager(Engine* engine) : m_Engine(engine), m_ModelLoader(this), m_TextureLoader(this) {}

    ResourceManager::~ResourceManager() {}

    void ResourceManager::Shutdown()
    {
        auto driver = m_Engine->GetDriver();

        m_WhiteTexture->Destroy(driver);
        m_BlackTexture->Destroy(driver);
        m_DitherTexture->Destroy(driver);
        // m_DefaultSkybox->Destroy(driver);
        delete m_WhiteTexture;
        delete m_BlackTexture;
        delete m_DitherTexture;
        // delete m_DefaultSkybox;

        for (auto& texture : m_Textures)
        {
            texture->Destroy(driver);
            delete texture;
        }

        for (auto& buffer : m_Buffers)
        {
            buffer->Destroy(driver);
            delete buffer;
        }

        for (auto& mesh : m_Meshes)
        {
            mesh->Destroy(driver);
            delete mesh;
        }

        for (auto& material : m_Materials)
        {
            delete material.second;
        }

        for (auto& shaderSet : m_ShaderSets)
        {
            shaderSet.second->Destroy(driver);
            delete shaderSet.second;
        }
    }

    void ResourceManager::InitResources(Driver* driver)
    {
        InitShaderSets(driver);
        InitMaterials(driver);
        InitDefaultTextures(driver);

        m_SkyboxMesh = CreateMesh(BoxMeshDescription {2.f, 2.f, 2.f});
        // create materials
    }

    void ResourceManager::InitShaderSets(Driver* driver)
    {
        // load shaders
        ShaderSetDescription shaderDesc {};
        // lit shader
        shaderDesc.pipeline   = PipelineTypeBits::Graphics;
        shaderDesc.vertex     = LoadShaderFromFile(Path::GetFilePath("/asset/shader/spv/lit.vert.spv"));
        shaderDesc.fragment   = LoadShaderFromFile(Path::GetFilePath("/asset/shader/spv/lit.frag.spv"));
        shaderDesc.vertexType = VertexType::Static;

        auto litShader = new ShaderSet(shaderDesc, driver);
        m_ShaderSets.insert({"lit", litShader});

        // deferred geometry
        shaderDesc.vertex     = LoadShaderFromFile(Path::GetFilePath("/asset/shader/spv/deferredGeometry.vert.spv"));
        shaderDesc.fragment   = LoadShaderFromFile(Path::GetFilePath("/asset/shader/spv/deferredGeometry.frag.spv"));
        shaderDesc.vertexType = VertexType::Static;

        auto dgShader = new ShaderSet(shaderDesc, driver);
        m_ShaderSets.insert({"deferredGeometry", dgShader});

        // deferred lighting
        shaderDesc.vertex     = LoadShaderFromFile(Path::GetFilePath("/asset/shader/spv/deferredLighting.vert.spv"));
        shaderDesc.fragment   = LoadShaderFromFile(Path::GetFilePath("/asset/shader/spv/deferredLighting.frag.spv"));
        shaderDesc.vertexType = VertexType::None;

        auto dlShader = new ShaderSet(shaderDesc, driver);
        m_ShaderSets.insert({"deferredLighting", dlShader});

        // cascade shadow
        shaderDesc.vertex     = LoadShaderFromFile(Path::GetFilePath("/asset/shader/spv/cascadeShadow.vert.spv"));
        shaderDesc.fragment   = LoadShaderFromFile(Path::GetFilePath("/asset/shader/spv/cascadeShadow.frag.spv"));
        shaderDesc.vertexType = VertexType::Static;

        auto shadowShader = new ShaderSet(shaderDesc, driver);
        m_ShaderSets.insert({"shadow", shadowShader});

        // skybox
        shaderDesc.vertex   = LoadShaderFromFile(Path::GetFilePath("/asset/shader/spv/skybox.vert.spv"));
        shaderDesc.fragment = LoadShaderFromFile(Path::GetFilePath("/asset/shader/spv/skybox.frag.spv"));

        auto skyboxShader = new ShaderSet(shaderDesc, driver);
        m_ShaderSets.insert({"skybox", skyboxShader});

        // post
        shaderDesc.pipeline = PipelineTypeBits::Compute;
        shaderDesc.compute  = LoadShaderFromFile(Path::GetFilePath("/asset/shader/spv/post.comp.spv"));

        auto postShader = new ShaderSet(shaderDesc, driver);
        m_ShaderSets.insert({"postCompute", postShader});
        // bloom prefilter
        shaderDesc.pipeline = PipelineTypeBits::Compute;
        shaderDesc.compute  = LoadShaderFromFile(Path::GetFilePath("/asset/shader/spv/bloomPrefilter.comp.spv"));

        auto bloomPrefilterShader = new ShaderSet(shaderDesc, driver);
        m_ShaderSets.insert({"bloomPrefilter", bloomPrefilterShader});

        // bloom downsample
        shaderDesc.pipeline = PipelineTypeBits::Compute;
        shaderDesc.compute  = LoadShaderFromFile(Path::GetFilePath("/asset/shader/spv/bloomDownsample.comp.spv"));

        auto bloomDownsampleShader = new ShaderSet(shaderDesc, driver);
        m_ShaderSets.insert({"bloomDownsample", bloomDownsampleShader});

        // bloom upsample
        shaderDesc.pipeline = PipelineTypeBits::Compute;
        shaderDesc.compute  = LoadShaderFromFile(Path::GetFilePath("/asset/shader/spv/bloomUpsample.comp.spv"));

        auto bloomUpsampleShader = new ShaderSet(shaderDesc, driver);
        m_ShaderSets.insert({"bloomUpsample", bloomUpsampleShader});

        // bloom composite
        shaderDesc.pipeline = PipelineTypeBits::Compute;
        shaderDesc.compute  = LoadShaderFromFile(Path::GetFilePath("/asset/shader/spv/bloomComposite.comp.spv"));

        auto bloomCompositeShader = new ShaderSet(shaderDesc, driver);
        m_ShaderSets.insert({"bloomComposite", bloomCompositeShader});

        // fxaa
        shaderDesc.pipeline = PipelineTypeBits::Compute;
        shaderDesc.compute  = LoadShaderFromFile(Path::GetFilePath("/asset/shader/spv/fxaa.comp.spv"));

        auto fxaaShader = new ShaderSet(shaderDesc, driver);
        m_ShaderSets.insert({"fxaa", fxaaShader});

        // resolve
        shaderDesc.pipeline   = PipelineTypeBits::Graphics;
        shaderDesc.vertex     = LoadShaderFromFile(Path::GetFilePath("/asset/shader/spv/quad.vert.spv"));
        shaderDesc.fragment   = LoadShaderFromFile(Path::GetFilePath("/asset/shader/spv/resolve.frag.spv"));
        shaderDesc.vertexType = VertexType::None;

        auto resolveShader = new ShaderSet(shaderDesc, driver);
        m_ShaderSets.insert({"resolve", resolveShader});
    }

    void ResourceManager::InitMaterials(Driver* driver)
    {
        // right now we're hardcoding this, but we can easily create config files and generate material from file

        // lit material
        {
            auto litShader   = m_ShaderSets.find("lit")->second;
            auto litMaterial = new Material(litShader->m_Handle);
            m_Materials.insert({"lit", litMaterial});
            // setup texture descriptor
            {
                litMaterial->m_TextureCount = 4;

                litMaterial->m_TextureDescriptors.resize(4);
                // skybox
                // auto& textureDesc0   = litMaterial->m_TextureDescriptors[0];
                // textureDesc0.name    = "skybox";
                // textureDesc0.set     = 0;
                // textureDesc0.binding = 1;
                // textureDesc0.type    = SamplerType::SamplerCubeMap;
                // textureDesc0.usage   = TextureUsageBits::Sampled;
                //// shadowmap
                // auto& textureDesc1   = litMaterial->m_TextureDescriptors[1];
                // textureDesc1.name    = "shadowmap";
                // textureDesc1.set     = 0;
                // textureDesc1.binding = 2;
                // textureDesc1.type    = SamplerType::Sampler2DArray;
                // textureDesc1.usage   = TextureUsageBits::Sampled;
                //  albedoMap
                auto& textureDesc2   = litMaterial->m_TextureDescriptors[0];
                textureDesc2.name    = "albedoMap";
                textureDesc2.set     = 1;
                textureDesc2.binding = 0;
                textureDesc2.type    = SamplerType::Sampler2D;
                textureDesc2.usage   = TextureUsageBits::Sampled;
                // normalMap
                auto& textureDesc3   = litMaterial->m_TextureDescriptors[1];
                textureDesc3.name    = "normalMap";
                textureDesc3.set     = 1;
                textureDesc3.binding = 1;
                textureDesc3.type    = SamplerType::Sampler2D;
                textureDesc3.usage   = TextureUsageBits::Sampled;
                // metallicRoughnessMap
                auto& textureDesc4   = litMaterial->m_TextureDescriptors[2];
                textureDesc4.name    = "mrMap";
                textureDesc4.set     = 1;
                textureDesc4.binding = 2;
                textureDesc4.type    = SamplerType::Sampler2D;
                textureDesc4.usage   = TextureUsageBits::Sampled;
                // emissionMap
                auto& textureDesc5   = litMaterial->m_TextureDescriptors[3];
                textureDesc5.name    = "emissionMap";
                textureDesc5.set     = 1;
                textureDesc5.binding = 3;
                textureDesc5.type    = SamplerType::Sampler2D;
                textureDesc5.usage   = TextureUsageBits::Sampled;
            }

            // setup uniform block descriptor
            {
                litMaterial->m_UniformBlockCount = 0;
            }

            // setup constant block descriptor
            {
                litMaterial->m_ConstantBlockCount = 1;
                litMaterial->m_ConstantBlockDescriptors.resize(1);

                // note this should not be in material
                // world matrix
                // auto& constantBlock0  = litMaterial->m_ConstantBlockDescriptors[0];
                // constantBlock0.name   = "worldMatrix";
                // constantBlock0.offset = 0;
                // constantBlock0.size   = 4 * 4 * 4;
                // constantBlock0.members.resize(1);
                // constantBlock0.stage = ShaderStageBits::Vertex;
                // auto& block0Member0  = constantBlock0.members[0];
                // block0Member0.name   = "worldMatrix";
                // block0Member0.offset = 0;
                // block0Member0.size   = 4 * 4 * 4;

                // material info
                auto& constantBlock1  = litMaterial->m_ConstantBlockDescriptors[0];
                constantBlock1.name   = "materialUniforms";
                constantBlock1.offset = 64;
                constantBlock1.size   = 10 * 4;
                constantBlock1.members.resize(6);
                constantBlock1.stage = ShaderStageBits::Fragment;
                // albedo
                auto& block1Member0  = constantBlock1.members[0];
                block1Member0.name   = "albedo";
                block1Member0.size   = 3 * 4;
                block1Member0.offset = 0;
                // metalness
                auto& block1Member1  = constantBlock1.members[1];
                block1Member1.name   = "metalness";
                block1Member1.size   = 4;
                block1Member1.offset = 3 * 4;
                // emission
                auto& block1Member2  = constantBlock1.members[2];
                block1Member2.name   = "emission";
                block1Member2.size   = 3 * 4;
                block1Member2.offset = 4 * 4;

                // roughness
                auto& block1Member3  = constantBlock1.members[3];
                block1Member3.name   = "roughness";
                block1Member3.size   = 4;
                block1Member3.offset = 7 * 4;
                // useNormalMap
                auto& block1Member4  = constantBlock1.members[4];
                block1Member4.name   = "useNormalMap";
                block1Member4.size   = 4;
                block1Member4.offset = 8 * 4;
                // receive shadow
                auto& block1Member5  = constantBlock1.members[5];
                block1Member5.name   = "receiveShadow";
                block1Member5.size   = 4;
                block1Member5.offset = 9 * 4;
                // TODO: this is so ugly
                float dv[10]                = {.2f, .3f, .4f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f};
                constantBlock1.defaultValue = std::vector<uint8_t>(10 * 4);
                memcpy(constantBlock1.defaultValue.data(), dv, 10 * 4);
            }
        }
    }

    void ResourceManager::InitDefaultTextures(Driver* driver)
    {
        // single color texture
        {
            TextureDescription desc {};
            desc.width     = 1;
            desc.height    = 1;
            desc.depth     = 1;
            desc.levels    = 1;
            desc.format    = TextureFormat::RGBA8_UNORM;
            desc.pipelines = PipelineTypeBits::Graphics;
            desc.sampler   = SamplerType::Sampler2D;
            desc.samples   = 1;
            desc.usage     = TextureUsageBits::Sampled;

            m_WhiteTexture = new Texture(desc, driver);
            m_BlackTexture = new Texture(desc, driver);

            uint32_t white = 0xffffff;
            uint32_t black = 0x000000;

            TextureUpdateDescriptor update {};
            update.data  = &white;
            update.layer = 0;
            update.level = 0;
            update.depth = 1;
            update.size  = 4;

            m_WhiteTexture->Update(update, driver);

            update.data = &black;

            m_BlackTexture->Update(update, driver);
        }
        // dither texture
        {
            TextureDescription desc {};
            desc.width     = 8;
            desc.height    = 8;
            desc.depth     = 1;
            desc.levels    = 1;
            desc.format    = TextureFormat::R8_UNORM;
            desc.pipelines = PipelineTypeBits::Graphics || PipelineTypeBits::Compute;
            desc.sampler   = SamplerType::Sampler2D;
            desc.samples   = 1;
            desc.usage     = TextureUsageBits::Sampled;
            // 8x8 Bayer ordered dithering pattern.  Each input pixel is scaled to the 0..63 range
            // before looking in this table to determine the action.
            m_DitherTexture         = new Texture(desc, driver);
            const uint8_t pattern[] = {0,  32, 8,  40, 2,  34, 10, 42, 48, 16, 56, 24, 50, 18, 58, 26,
                                       12, 44, 4,  36, 14, 46, 6,  38, 60, 28, 52, 20, 62, 30, 54, 22,
                                       3,  35, 11, 43, 1,  33, 9,  41, 51, 19, 59, 27, 49, 17, 57, 25,
                                       15, 47, 7,  39, 13, 45, 5,  37, 63, 31, 55, 23, 61, 29, 53, 21};

            TextureUpdateDescriptor update {};
            update.data  = (void*)pattern;
            update.layer = 0;
            update.level = 0;
            update.depth = 1;
            update.size  = 8 * 8 * 4;

            m_DitherTexture->Update(update, driver);
        }

        // default skybox
        {
            // std::string view[6] = {Path::GetFilePath("asset/cubemap/Milkyway/px.png"),
            //                        Path::GetFilePath("asset/cubemap/Milkyway/nx.png"),
            //                        Path::GetFilePath("asset/cubemap/Milkyway/py.png"),
            //                        Path::GetFilePath("asset/cubemap/Milkyway/ny.png"),
            //                        Path::GetFilePath("asset/cubemap/Milkyway/pz.png"),
            //                        Path::GetFilePath("asset/cubemap/Milkyway/nz.png")};
            std::string view[6] = {Path::GetFilePath("asset/cubemap/Yokohama3/posx.jpg"),
                                   Path::GetFilePath("asset/cubemap/Yokohama3/negx.jpg"),
                                   Path::GetFilePath("asset/cubemap/Yokohama3/posy.jpg"),
                                   Path::GetFilePath("asset/cubemap/Yokohama3/negy.jpg"),
                                   Path::GetFilePath("asset/cubemap/Yokohama3/posz.jpg"),
                                   Path::GetFilePath("asset/cubemap/Yokohama3/negz.jpg")};
            // std::string view[6] = {Path::GetFilePath("asset/cubemap/GraceCathedral/posx.jpg"),
            //                        Path::GetFilePath("asset/cubemap/GraceCathedral/negx.jpg"),
            //                        Path::GetFilePath("asset/cubemap/GraceCathedral/posy.jpg"),
            //                        Path::GetFilePath("asset/cubemap/GraceCathedral/negy.jpg"),
            //                        Path::GetFilePath("asset/cubemap/GraceCathedral/posz.jpg"),
            //                        Path::GetFilePath("asset/cubemap/GraceCathedral/negz.jpg")};
            // std::string view[6] = {Path::GetFilePath("asset/cubemap/Indoor/posx.jpg"),
            //                        Path::GetFilePath("asset/cubemap/Indoor/negx.jpg"),
            //                        Path::GetFilePath("asset/cubemap/Indoor/posy.jpg"),
            //                        Path::GetFilePath("asset/cubemap/Indoor/negy.jpg"),
            //                        Path::GetFilePath("asset/cubemap/Indoor/posz.jpg"),
            //                        Path::GetFilePath("asset/cubemap/Indoor/negz.jpg")};
            m_DefaultSkybox = m_TextureLoader.Load(view);
            driver->GenerateMips(m_DefaultSkybox->GetHandle());
        }
        {
            // prefiltered env map
            m_DefaultPrefilterEnv = m_TextureLoader.LoadEnv(Path::GetFilePath("asset/cubemap/Yokohama3/yokohama.bin"));
            // m_DefaultPrefilterEnv = m_TextureLoader.LoadEnv(Path::GetFilePath("asset/cubemap/Indoor/Indoor.bin"));
        }
        // brdf lut for split sum approximation
        {
            std::string view = Path::GetFilePath("asset/ibl/brdf_lut.hdr");

            m_BrdfLut = m_TextureLoader.Load(view, false);
            driver->GenerateMips(m_BrdfLut->GetHandle());
        }
    }

    Texture* ResourceManager::CreateTexture(const TextureDescription& desc)
    {
        auto texture = new Texture(desc, m_Engine->GetDriver());
        m_Textures.push_back(texture);

        return texture;
    }

    Texture* ResourceManager::CreateTexture(const std::string& path, bool srgb, bool flip)
    {
        auto texture = m_TextureLoader.Load(path, srgb, flip);
        m_Engine->GetDriver()->GenerateMips(texture->GetHandle());

        return texture;
    }

    Texture* ResourceManager::CreateTexture(std::string path[6]) { return m_TextureLoader.Load(path); }

    Buffer* ResourceManager::CreateBuffer(const BufferDescription& desc)
    {
        auto buffer = new Buffer(desc, m_Engine->GetDriver());
        m_Buffers.push_back(buffer);

        return buffer;
    }

    Mesh* ResourceManager::CreateMesh(const std::string& path)
    {
        // TODO: better hierachy
        auto mesh = m_ModelLoader.LoadModel(Path::GetFilePath(path));

        mesh->InitResource(m_Engine->GetDriver());

        m_Meshes.push_back(mesh);

        return mesh;
    }

    Mesh* ResourceManager::CreateMesh(const BoxMeshDescription& desc)
    {
        auto mesh = new BoxMesh(desc);
        m_Meshes.push_back(mesh);
        auto mi = CreateMaterialInstance(ShadingModel::Lit);
        mesh->SetMaterials({mi});
        mesh->InitResource(m_Engine->GetDriver());
        return mesh;
    }

    MaterialInstance* ResourceManager::CreateMaterialInstance(ShadingModel shading)
    {
        MaterialInstance* instance = nullptr;
        switch (shading)
        {
            case ShadingModel::Lit:
                instance = new MaterialInstance(m_Materials.find("lit")->second);

                // set lit material default value
                instance->SetTexture("albedoMap", GetWhiteTexture());
                instance->SetTexture("normalMap", GetWhiteTexture());
                instance->SetTexture("mrMap", GetWhiteTexture());
                instance->SetTexture("emissionMap", GetBlackTexture());
                instance->SetConstantBlock("albedo", glm::vec3(1.));
                instance->SetConstantBlock("metalness", 1.f);
                instance->SetConstantBlock("roughness", 1.f);
                instance->SetConstantBlock("useNormapMap", 0.f);
                instance->SetConstantBlock("emission", glm::vec3(0));
                break;
        }

        assert(instance);
        m_MaterialInstances.push_back(instance);

        return instance;
    }

    Material* ResourceManager::GetMaterial(const std::string& name)
    {
        auto iter = m_Materials.find(name);
        if (iter == m_Materials.end())
        {
            return nullptr;
        }

        return iter->second;
    }

    ShaderSet* ResourceManager::GetShaderSet(const std::string& name)
    {
        auto iter = m_ShaderSets.find(name);

        if (iter == m_ShaderSets.end())
        {
            return nullptr;
        }
        return iter->second;
    }

} // namespace Zephyr