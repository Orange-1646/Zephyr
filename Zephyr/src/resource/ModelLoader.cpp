#include "ModelLoader.h"
#include "Material.h"
#include "MaterialInstance.h"
#include "Mesh.h"
#include "ResourceManager.h"
#include "Texture.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace Zephyr
{
    static inline constexpr uint32_t s_MeshImportFlags =
        aiProcess_CalcTangentSpace | // Create binormals/tangents just in case
        aiProcess_Triangulate |      // Make sure we're triangles
        aiProcess_SortByPType |      // Split meshes by primitive type
        aiProcess_GenNormals |       // Make sure we have legit normals
        aiProcess_GenUVCoords |      // Convert UVs if required
                                     //		aiProcess_OptimizeGraph |
        aiProcess_OptimizeMeshes |   // Batch draws where possible
        aiProcess_JoinIdenticalVertices |
        aiProcess_GlobalScale |          // e.g. convert cm to m for fbx import (and other formats where cm is native)
        aiProcess_ValidateDataStructure; // Validation

    glm::mat4 Mat4FromAssimpMat4(const aiMatrix4x4& matrix)
    {
        glm::mat4 result;
        // the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
        result[0][0] = matrix.a1;
        result[1][0] = matrix.a2;
        result[2][0] = matrix.a3;
        result[3][0] = matrix.a4;
        result[0][1] = matrix.b1;
        result[1][1] = matrix.b2;
        result[2][1] = matrix.b3;
        result[3][1] = matrix.b4;
        result[0][2] = matrix.c1;
        result[1][2] = matrix.c2;
        result[2][2] = matrix.c3;
        result[3][2] = matrix.c4;
        result[0][3] = matrix.d1;
        result[1][3] = matrix.d2;
        result[2][3] = matrix.d3;
        result[3][3] = matrix.d4;
        return result;
    }

    Mesh* ModelLoader::LoadModel(const std::string& path)
    {
        auto iter = m_MeshCache.find(path);
        if (iter != m_MeshCache.end())
        {
            return iter->second;
        }

        auto m = new Mesh();

        // load mesh through assimp
        Assimp::Importer importer;
        const aiScene*   scene = importer.ReadFile(path, s_MeshImportFlags);
        if (!scene || !scene->HasMeshes())
        {
            assert(false);
            return nullptr;
        }

        uint32_t vertexCount = 0;
        uint32_t indexCount  = 0;

        m->m_Submeshes.reserve(scene->mNumMeshes);

        m->m_Aabb.min = {FLT_MAX, FLT_MAX, FLT_MAX};
        m->m_Aabb.max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

        for (uint32_t i = 0; i < scene->mNumMeshes; i++)
        {

            aiMesh* mesh = scene->mMeshes[i];

            Submesh& submesh = m->m_Submeshes.emplace_back();

            submesh.aabb.min      = {FLT_MAX, FLT_MAX, FLT_MAX};
            submesh.aabb.max      = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
            submesh.baseVertex    = vertexCount;
            submesh.baseIndex     = indexCount;
            submesh.vertexCount   = mesh->mNumVertices;
            submesh.indexCount    = mesh->mNumFaces * 3;
            submesh.materialIndex = mesh->mMaterialIndex;

            vertexCount += mesh->mNumVertices;
            indexCount += submesh.indexCount;

            assert(mesh->HasPositions() && mesh->HasNormals());

            for (size_t j = 0; j < mesh->mNumVertices; j++)
            {
                Vertex vertex;

                vertex.px = mesh->mVertices[j].x;
                vertex.py = mesh->mVertices[j].y;
                vertex.pz = mesh->mVertices[j].z;
                vertex.nx = mesh->mNormals[j].x;
                vertex.ny = mesh->mNormals[j].y;
                vertex.nz = mesh->mNormals[j].z;

                submesh.aabb.min.x = glm::min(vertex.px, submesh.aabb.min.x);
                submesh.aabb.min.y = glm::min(vertex.py, submesh.aabb.min.y);
                submesh.aabb.min.z = glm::min(vertex.pz, submesh.aabb.min.z);
                submesh.aabb.max.x = glm::max(vertex.px, submesh.aabb.max.x);
                submesh.aabb.max.y = glm::max(vertex.py, submesh.aabb.max.y);
                submesh.aabb.max.z = glm::max(vertex.pz, submesh.aabb.max.z);

                if (mesh->HasTangentsAndBitangents())
                {
                    vertex.tx = mesh->mTangents[j].x;
                    vertex.ty = mesh->mTangents[j].y;
                    vertex.tz = mesh->mTangents[j].z;
                    vertex.bx = mesh->mBitangents[j].x;
                    vertex.by = mesh->mBitangents[j].y;
                    vertex.bz = mesh->mBitangents[j].z;
                }

                if (mesh->HasTextureCoords(0))
                {
                    vertex.tu = mesh->mTextureCoords[0][j].x;
                    vertex.tv = mesh->mTextureCoords[0][j].y;
                }

                m->m_Vertices.push_back(vertex);
            }

            for (size_t j = 0; j < mesh->mNumFaces; j++)
            {
                m->m_Indices.push_back(mesh->mFaces[j].mIndices[0]);
                m->m_Indices.push_back(mesh->mFaces[j].mIndices[1]);
                m->m_Indices.push_back(mesh->mFaces[j].mIndices[2]);
            }
        }

        std::unordered_map<aiNode*, std::vector<uint32_t>> nodeMap;
        TraverseNodes(m, scene->mRootNode, nodeMap);

        // set mesh bounding box
        for (auto& submesh : m->m_Submeshes)
        {
            auto&     boudingBoxSubmesh = submesh.aabb;
            glm::vec3 min               = glm::vec3(submesh.transform * glm::vec4(boudingBoxSubmesh.min, 1.0f));
            glm::vec3 max               = glm::vec3(submesh.transform * glm::vec4(boudingBoxSubmesh.max, 1.0f));

            m->m_Aabb.min.x = glm::min(m->m_Aabb.min.x, min.x);
            m->m_Aabb.min.y = glm::min(m->m_Aabb.min.y, min.y);
            m->m_Aabb.min.z = glm::min(m->m_Aabb.min.z, min.z);

            m->m_Aabb.max.x = glm::max(m->m_Aabb.max.x, max.x);
            m->m_Aabb.max.y = glm::max(m->m_Aabb.max.y, max.y);
            m->m_Aabb.max.z = glm::max(m->m_Aabb.max.z, max.z);
        }

        if (scene->HasMaterials())
        {
            m->m_Materials.reserve(scene->mNumMaterials);

            for (uint32_t i = 0; i < scene->mNumMaterials; i++)
            {
                auto materialInstance = m_ResourceManager->CreateMaterialInstance(ShadingModel::Lit);

                auto     aiMaterial = scene->mMaterials[i];
                aiString aiTexPath;

                bool hasAlbedoMap = aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiTexPath) == AI_SUCCESS;

                if (hasAlbedoMap)
                {
                    // TODO: add filesystem
                    std::filesystem::path p       = path;
                    auto                  parentPath = p.parent_path();
                    parentPath /= std::string(aiTexPath.data);
                    std::string texturePath = parentPath.string();

                    materialInstance->SetTexture("albedoMap", m_ResourceManager->CreateTexture(texturePath));
                    materialInstance->SetConstantBlock("albedo", glm::vec3(1.));
                }
                else
                {
                    materialInstance->SetTexture("albedoMap", m_ResourceManager->GetWhiteTexture());
                    aiColor3D aiColor;
                    if (aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor) == AI_SUCCESS)
                    {
                        materialInstance->SetConstantBlock("albedo", glm::vec3(aiColor.r, aiColor.g, aiColor.b));
                    }
                    else
                    {
                        assert(false);
                    }
                }

                bool hasNormalMap = aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &aiTexPath) == AI_SUCCESS;

                if (hasNormalMap)
                {
                    // TODO: add filesystem
                    std::filesystem::path p          = path;
                    auto                  parentPath = p.parent_path();
                    parentPath /= std::string(aiTexPath.data);
                    std::string texturePath = parentPath.string();

                    materialInstance->SetTexture("normalMap", m_ResourceManager->CreateTexture(texturePath));
                    materialInstance->SetConstantBlock("useNormalMap", 1.f);
                }
                else
                {
                    materialInstance->SetTexture("normalMap", m_ResourceManager->GetWhiteTexture());
                    materialInstance->SetConstantBlock("useNormalMap", 0.f);
                }

                bool hasMr = aiMaterial->GetTexture(aiTextureType_UNKNOWN, 0, &aiTexPath) == AI_SUCCESS;

                if (hasMr)
                {
                    // TODO: add filesystem
                    std::filesystem::path p          = path;
                    auto                  parentPath = p.parent_path();
                    parentPath /= std::string(aiTexPath.data);
                    std::string texturePath = parentPath.string();
                    materialInstance->SetTexture("mrMap", m_ResourceManager->CreateTexture(texturePath));
                    materialInstance->SetConstantBlock("metalness", 1.f);
                    materialInstance->SetConstantBlock("roughness", 1.f);
                }
                else
                {
                    materialInstance->SetTexture("mrMap", m_ResourceManager->GetWhiteTexture());

                    float metalness;
                    float roughness;
                    if (aiMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metalness) != aiReturn_SUCCESS)
                    {
                        materialInstance->SetConstantBlock("metalness", 0.f);
                    }
                    else
                    {
                        materialInstance->SetConstantBlock("metalness", metalness);
                    }
                    if (aiMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) != aiReturn_SUCCESS)
                    {
                        materialInstance->SetConstantBlock("roughness", 0.f);
                    }
                    else
                    {
                        materialInstance->SetConstantBlock("roughness", roughness);
                    }
                }

                bool hasEmission = aiMaterial->GetTexture(aiTextureType_EMISSIVE, 0, &aiTexPath) == AI_SUCCESS;

                if (hasEmission)
                {
                    // TODO: add filesystem
                    std::filesystem::path p       = path;
                    auto                  parentPath = p.parent_path();
                    parentPath /= std::string(aiTexPath.data);
                    std::string texturePath = parentPath.string();

                    materialInstance->SetTexture("emissionMap", m_ResourceManager->CreateTexture(texturePath));
                }
                else
                {
                    materialInstance->SetTexture("emissionMap", m_ResourceManager->GetBlackTexture());
                }
            }
        }
    }

    void ModelLoader::TraverseNodes(Mesh*                                               mesh,
                                    aiNode*                                             node,
                                    std::unordered_map<aiNode*, std::vector<uint32_t>>& nodeMap,
                                    const glm::mat4&                                    parentTransform,
                                    uint32_t                                            level)
    {

        glm::mat4 localTransform = Mat4FromAssimpMat4(node->mTransformation);
        glm::mat4 transform      = parentTransform * localTransform;

        nodeMap[node].resize(node->mNumMeshes);
        for (uint32_t i = 0; i < node->mNumMeshes; i++)
        {
            uint32_t meshIndex     = node->mMeshes[i];
            auto&    submesh       = mesh->m_Submeshes[meshIndex];
            submesh.transform      = transform;
            submesh.localTransform = localTransform;
            nodeMap[node][i]       = meshIndex;
        }

        // HZ_MESH_LOG("{0} {1}", LevelToSpaces(level), node->mName.C_Str());

        for (uint32_t i = 0; i < node->mNumChildren; i++)
            TraverseNodes(mesh, node->mChildren[i], nodeMap, transform, level + 1);
    }

    ModelLoader::ModelLoader(ResourceManager* manager) : m_ResourceManager(manager) {}
} // namespace Zephyr