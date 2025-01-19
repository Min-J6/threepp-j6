
#ifndef THREEPP_ASSIMPLOADER_HPP
#define THREEPP_ASSIMPLOADER_HPP

#include "threepp/loaders/Loader.hpp"
#include "threepp/loaders/TextureLoader.hpp"
#include "threepp/materials/MeshStandardMaterial.hpp"
#include "threepp/materials/MeshPhongMaterial.hpp"
#include "threepp/materials/MeshToonMaterial.hpp"
#include "threepp/objects/Group.hpp"
#include "threepp/objects/Mesh.hpp"
#include "threepp/objects/SkinnedMesh.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>
#include <sstream>
#include <utility>

namespace threepp {

    class AssimpLoader: public Loader<Group> {

    public:
        std::shared_ptr<Group> load(const std::filesystem::path& path) override {
            // aiProcessPreset_TargetRealtime_Quality aiProcessPreset_TargetRealtime_Fast
            auto aiScene = importer_.ReadFile(path.string().c_str(), aiProcessPreset_TargetRealtime_Fast);

            if (!aiScene) {
                throw std::runtime_error(importer_.GetErrorString());
            }

            SceneInfo info(path);
            preParse(info, aiScene, aiScene->mRootNode);

            auto group = Group::create();
            group->name = path.filename().stem().string();
            parseNodes(info, aiScene, aiScene->mRootNode, *group);

            return group;
        }

    private:
        TextureLoader texLoader_;
        Assimp::Importer importer_;

        struct SceneInfo;

        void parseNodes(const SceneInfo& info, const aiScene* aiScene, aiNode* aiNode, Object3D& parent) {

            std::string nodeName(aiNode->mName.data);

            std::shared_ptr<Object3D> group = info.getBone(nodeName);
            if (!group) group = Group::create();
            group->name = nodeName;
            setTransform(*group, aiNode->mTransformation);
            parent.add(group);

            auto meshes = parseNodeMeshes(info, aiScene, aiNode);
            for (const auto& mesh : meshes) {

                // mesh->rotateX(AI_DEG_TO_RAD(90)); // rotate x 90
                mesh->castShadow = true;
                mesh->receiveShadow = true;
                group->add(mesh);
            }

            for (unsigned i = 0; i < aiNode->mNumChildren; ++i) {
                parseNodes(info, aiScene, aiNode->mChildren[i], *group);
            }
        }

        std::vector<std::shared_ptr<Mesh>> parseNodeMeshes(const SceneInfo& info, const aiScene* aiScene, const aiNode* aiNode) {

            std::vector<std::shared_ptr<Mesh>> children;

            for (unsigned i = 0; i < aiNode->mNumMeshes; ++i) {

                const auto meshIndex = aiNode->mMeshes[i];
                const auto aiMesh = aiScene->mMeshes[meshIndex];

                auto geometry = BufferGeometry::create();
                auto material = MeshStandardMaterial::create();
                setupMaterial(info.path, aiScene, aiMesh, *material);

                std::shared_ptr<Mesh> mesh;
                if (info.hasSkeleton(meshIndex)) {

                    const auto boneData = info.boneData.at(meshIndex);

                    geometry->setAttribute("skinIndex", FloatBufferAttribute::create(boneData.boneIndices, 4));
                    geometry->setAttribute("skinWeight", FloatBufferAttribute::create(boneData.boneWeights, 4));

                    auto skinnedMesh = SkinnedMesh::create(geometry, material);
                    skinnedMesh->normalizeSkinWeights();

                    auto skeleton = Skeleton::create(boneData.bones, boneData.boneInverses);
                    skinnedMesh->bind(skeleton, Matrix4());

                    mesh = skinnedMesh;
                } else {

                    mesh = Mesh::create(geometry, material);
                }

                auto name_ = std::filesystem::path(info.path).stem().string();
                mesh->name = aiMesh->mName.C_Str();
                mesh->frustumCulled = false;
                if (mesh->name.empty()) {
                    mesh->name = name_ + "_" + "mesh";
                }

                std::vector<unsigned int> indices;
                std::vector<float> vertices;
                std::vector<float> normals;
                std::vector<float> colors;
                std::vector<float> uvs;
                std::vector<std::vector<float>> morphPositions(aiMesh->mNumAnimMeshes);

                if (aiMesh->HasFaces()) {

                    // Populate the index buffer
                    for (unsigned j = 0; j < aiMesh->mNumFaces; j++) {
                        const aiFace& face = aiMesh->mFaces[j];
                        indices.push_back(face.mIndices[0]);
                        indices.push_back(face.mIndices[1]);
                        indices.push_back(face.mIndices[2]);
                    }

                    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);
                    // Populate the vertex attribute vectors
                    for (unsigned j = 0; j < aiMesh->mNumVertices; j++) {
                        const auto pos = aiMesh->mVertices[j];
                        const auto normal = aiMesh->mNormals[j];
                        const auto texCoord = aiMesh->HasTextureCoords(0) ? aiMesh->mTextureCoords[0][j] : Zero3D;
                        if (aiMesh->HasVertexColors(0)) {
                            const auto color = aiMesh->mColors[0][j];
                            //colors.insert(colors.end(), {color.r, color.g, color.b, color.a});
                            colors.insert(colors.end(), {1.0f, 1.0f, 1.0f, 1.0f});
                        }

                        for (auto k = 0; k < aiMesh->mNumAnimMeshes; k++) {
                            auto& list = morphPositions[k];
                            const auto mp = aiMesh->mAnimMeshes[k]->mVertices[j];
                            list.insert(list.end(), {mp.x, mp.y, mp.z});
                        }

                        vertices.insert(vertices.end(), {pos.x, pos.y, pos.z});
                        normals.insert(normals.end(), {normal.x, normal.y, normal.z});
                        uvs.insert(uvs.end(), {texCoord.x, texCoord.y});
                    }
                }

                if (!indices.empty()) {
                    geometry->setIndex(indices);
                }

                geometry->setAttribute("position", FloatBufferAttribute::create(vertices, 3));
                if (!normals.empty()) {
                    geometry->setAttribute("normal", FloatBufferAttribute::create(normals, 3));
                }
                if (!colors.empty()) {
                    material->vertexColors = true;
                    geometry->setAttribute("color", FloatBufferAttribute::create(colors, 4));
                }
                if (!uvs.empty()) {
                    geometry->setAttribute("uv", FloatBufferAttribute::create(uvs, 2));
                }

                if (!morphPositions.empty()) {
                    for (const auto& pos : morphPositions) {
                        geometry->getOrCreateMorphAttribute("position")->emplace_back(FloatBufferAttribute::create(pos, 3));
                        mesh->morphTargetInfluences().emplace_back();
                    }
                }
                children.emplace_back(mesh);
            }

            return children;
        }

        struct BoneData {

            std::vector<float> boneIndices;
            std::vector<float> boneWeights;

            std::vector<Matrix4> boneInverses;
            std::vector<std::shared_ptr<Bone>> bones;
        };

        struct SceneInfo {

            std::filesystem::path path;
            std::unordered_map<unsigned int, BoneData> boneData;

            explicit SceneInfo(std::filesystem::path path): path(std::move(path)) {}

            [[nodiscard]] bool hasSkeleton(unsigned int meshIndex) const {
                return boneData.count(meshIndex);
            }

            [[nodiscard]] std::shared_ptr<Bone> getBone(const std::string& name) const {
                for (const auto& [idx, data] : boneData) {
                    for (const auto& bone : data.bones) {
                        if (bone->name.substr(5) == name) {
                            return bone;
                        }
                    }
                }
                return nullptr;
            }
        };

        void preParse(SceneInfo& info, const aiScene* aiScene, aiNode* aiNode) {

            for (unsigned i = 0; i < aiNode->mNumMeshes; ++i) {
                auto meshIndex = aiNode->mMeshes[i];
                auto aiMesh = aiScene->mMeshes[meshIndex];

                if (aiMesh->HasBones()) {
                    BoneData data;

                    std::vector<std::vector<float>> boneIndices;
                    std::vector<std::vector<float>> boneWeights;

                    for (auto j = 0; j < aiMesh->mNumBones; j++) {

                        const auto aiBone = aiMesh->mBones[j];
                        std::string boneName(aiBone->mName.data);

                        std::shared_ptr<Bone> bone;
                        if (auto oldBone = info.getBone(boneName)) {
                            bone = oldBone;
                        } else {
                            bone = Bone::create();
                            bone->name = "Bone:" + boneName;
                        }

                        data.bones.emplace_back(bone);

                        data.boneInverses.emplace_back(aiMatrixToMatrix4(aiBone->mOffsetMatrix));

                        for (auto k = 0; k < aiBone->mNumWeights; k++) {
                            const auto aiWeight = aiBone->mWeights[k];

                            while (boneWeights.size() <= aiWeight.mVertexId) boneWeights.emplace_back();
                            while (boneIndices.size() <= aiWeight.mVertexId) boneIndices.emplace_back();

                            boneWeights[aiWeight.mVertexId].emplace_back(aiWeight.mWeight);
                            boneIndices[aiWeight.mVertexId].emplace_back(static_cast<float>(j));
                        }
                    }

                    for (unsigned j = 0; j < boneIndices.size(); j++) {

                        sortWeights(boneIndices[j], boneWeights[j]);
                    }

                    for (unsigned j = 0; j < boneWeights.size(); j++) {

                        for (unsigned k = 0; k < 4; k++) {

                            if (!boneWeights[j].empty() && !boneIndices[j].empty()) {

                                const auto weight = boneWeights[j][k];
                                const auto index = boneIndices[j][k];

                                data.boneWeights.emplace_back(weight);
                                data.boneIndices.emplace_back(index);

                            } else {

                                data.boneWeights.emplace_back(0.f);
                                data.boneIndices.emplace_back(0.f);
                            }
                        }
                    }
                    info.boneData[meshIndex] = data;
                }
            }

            for (unsigned i = 0; i < aiNode->mNumChildren; ++i) {
                preParse(info, aiScene, aiNode->mChildren[i]);
            }
        }

        static void sortWeights(std::vector<float>& indexes, std::vector<float>& weights) {

            std::vector<std::pair<float, float>> pairs;

            for (unsigned i = 0; i < indexes.size(); i++) {

                pairs.emplace_back(indexes[i], weights[i]);
            }

            std::stable_sort(pairs.begin(), pairs.end(), [](const auto& a, const auto& b) {
                return b.second < a.second;
            });

            while (pairs.size() < 4) pairs.emplace_back(0.f, 0.f);
            while (pairs.size() > 4) pairs.pop_back();

            float sum = 0;
            for (unsigned i = 0; i < 4; i++) {

                sum += pairs[i].second * pairs[i].second;
            }
            sum = std::sqrt(sum);

            for (unsigned i = 0; i < 4; i++) {

                pairs[i].second = pairs[i].second / sum;

                while (indexes.size() <= i) indexes.emplace_back();
                while (weights.size() <= i) weights.emplace_back();

                indexes[i] = pairs[i].first;
                weights[i] = pairs[i].second;
            }
        }

        void handleWrapping(const aiMaterial* mat, aiTextureType mode, Texture& tex) {

            aiTextureMapMode wrapS;
            if (AI_SUCCESS == mat->Get(AI_MATKEY_MAPPINGMODE_U(mode, 0), wrapS)) {
                switch (wrapS) {
                    case aiTextureMapMode_Wrap:
                        tex.wrapS = TextureWrapping::Repeat;
                        break;
                    case aiTextureMapMode_Mirror:
                        tex.wrapS = TextureWrapping::MirroredRepeat;
                        break;
                    case aiTextureMapMode_Clamp:
                        tex.wrapS = TextureWrapping::ClampToEdge;
                        break;
                }
            }
            aiTextureMapMode wrapT;
            if (AI_SUCCESS == mat->Get(AI_MATKEY_MAPPINGMODE_V(mode, 0), wrapT)) {
                switch (wrapT) {
                    case aiTextureMapMode_Wrap:
                        tex.wrapT = TextureWrapping::Repeat;
                        break;
                    case aiTextureMapMode_Mirror:
                        tex.wrapT = TextureWrapping::MirroredRepeat;
                        break;
                    case aiTextureMapMode_Clamp:
                        tex.wrapT = TextureWrapping::ClampToEdge;
                        break;
                }
            }
        }

        void debugMaterialTextures(const aiMaterial* material) {
            aiString path;
            aiString name;
            material->Get(AI_MATKEY_NAME, name);
            printf("\n[Material: %s]\n", name.C_Str());

            // 기본 material 속성들 출력
            aiColor4D baseColor, diffuse, specular, emissive, ambient;
            float metallic, roughness, opacity, shininess;
            int wireframe, twosided;

            printf("Properties:\n");
            if(AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &baseColor)) {
                printf("  Base Color: RGB(%.3f, %.3f, %.3f) A(%.3f)\n",
                    baseColor.r, baseColor.g, baseColor.b, baseColor.a);
            }
            if(AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuse)) {
                printf("  Diffuse: RGB(%.3f, %.3f, %.3f) A(%.3f)\n",
                    diffuse.r, diffuse.g, diffuse.b, diffuse.a);
            }
            if(AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &specular)) {
                printf("  Specular: RGB(%.3f, %.3f, %.3f) A(%.3f)\n",
                    specular.r, specular.g, specular.b, specular.a);
            }
            if(AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_EMISSIVE, &emissive)) {
                printf("  Emissive: RGB(%.3f, %.3f, %.3f) A(%.3f)\n",
                    emissive.r, emissive.g, emissive.b, emissive.a);
            }
            if(AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_AMBIENT, &ambient)) {
                printf("  Ambient: RGB(%.3f, %.3f, %.3f) A(%.3f)\n",
                    ambient.r, ambient.g, ambient.b, ambient.a);
            }

            // PBR 속성들
            if(AI_SUCCESS == aiGetMaterialFloat(material, AI_MATKEY_METALLIC_FACTOR, &metallic)) {
                printf("  Metallic Factor: %.3f\n", metallic);
            }
            if(AI_SUCCESS == aiGetMaterialFloat(material, AI_MATKEY_ROUGHNESS_FACTOR, &roughness)) {
                printf("  Roughness Factor: %.3f\n", roughness);
            }

            // 기타 속성들
            if(AI_SUCCESS == aiGetMaterialFloat(material, AI_MATKEY_OPACITY, &opacity)) {
                printf("  Opacity: %.3f\n", opacity);
            }
            if(AI_SUCCESS == aiGetMaterialFloat(material, AI_MATKEY_SHININESS, &shininess)) {
                printf("  Shininess: %.3f\n", shininess);
            }
            if(AI_SUCCESS == aiGetMaterialInteger(material, AI_MATKEY_ENABLE_WIREFRAME, &wireframe)) {
                printf("  Wireframe: %s\n", wireframe ? "Yes" : "No");
            }
            if(AI_SUCCESS == aiGetMaterialInteger(material, AI_MATKEY_TWOSIDED, &twosided)) {
                printf("  Two Sided: %s\n", twosided ? "Yes" : "No");
            }

            // 텍스처 정보 출력
            printf("Textures:\n");
            const struct {
                aiTextureType type;
                const char* name;
                const char* description;
            } textureTypes[] = {
                // 기본 텍스처 타입
                {aiTextureType_DIFFUSE, "Diffuse", "Base/Diffuse color texture"},
                {aiTextureType_SPECULAR, "Specular", "Specular/Glossiness texture"},
                {aiTextureType_AMBIENT, "Ambient", "Ambient/Occlusion texture"},
                {aiTextureType_EMISSIVE, "Emissive", "Emissive color texture"},
                {aiTextureType_HEIGHT, "Height", "Height/Bump map texture"},
                {aiTextureType_NORMALS, "Normals", "Normal map texture"},
                {aiTextureType_SHININESS, "Shininess", "Shininess/Glossiness texture"},
                {aiTextureType_OPACITY, "Opacity", "Opacity/Transparency texture"},
                {aiTextureType_DISPLACEMENT, "Displacement", "Displacement map"},
                {aiTextureType_LIGHTMAP, "Lightmap", "Lightmap texture"},
                {aiTextureType_REFLECTION, "Reflection", "Reflection/Environment map"},

                // PBR 텍스처 타입
                {aiTextureType_BASE_COLOR, "Base Color", "PBR base color texture"},
                {aiTextureType_NORMAL_CAMERA, "Normal Camera", "PBR normal map texture"},
                {aiTextureType_EMISSION_COLOR, "Emission Color", "PBR emission color texture"},
                {aiTextureType_METALNESS, "Metalness", "PBR metallic texture"},
                {aiTextureType_DIFFUSE_ROUGHNESS, "Roughness", "PBR roughness texture"},
                {aiTextureType_AMBIENT_OCCLUSION, "Ambient Occlusion", "PBR ambient occlusion texture"},

                // 추가 PBR 수식자
                {aiTextureType_SHEEN, "Sheen", "PBR sheen texture"},
                {aiTextureType_CLEARCOAT, "Clearcoat", "PBR clearcoat texture"},
                {aiTextureType_TRANSMISSION, "Transmission", "PBR transmission texture"},
                {aiTextureType_UNKNOWN, "Unknown", "Unknown texture type"}
            };

            for(const auto& texType : textureTypes) {
                unsigned int numTextures = material->GetTextureCount(texType.type);
                if(numTextures > 0) {
                    printf("  %s (%s):\n", texType.name, texType.description);
                    for(unsigned int i = 0; i < numTextures; i++) {
                        aiString texPath;
                        float blend;

                        if(material->GetTexture(texType.type, i, &texPath, NULL, NULL, &blend) == AI_SUCCESS) {
                            printf("    - Path: %s\n", texPath.C_Str());
                            printf("      Index: %u\n", i);
                            printf("      Blend: %.3f\n", blend);
                        }
                    }
                }
            }
        }


        void setupMaterial(const std::filesystem::path& path, const aiScene* aiScene, const aiMesh* aiMesh, MeshStandardMaterial& material) {
            if (!aiScene->HasMaterials()) return;

            auto mi = aiMesh->mMaterialIndex;
            auto mat = aiScene->mMaterials[mi];
            aiString p;

            // debugMaterialTextures(mat);

            // Base Color/Diffuse
            if (aiGetMaterialTextureCount(mat, aiTextureType_BASE_COLOR) > 0) {
                if (aiGetMaterialTexture(mat, aiTextureType_BASE_COLOR, 0, &p) == aiReturn_SUCCESS) {
                    auto tex = loadTexture(aiScene, path, p.C_Str());
                    handleWrapping(mat, aiTextureType_BASE_COLOR, *tex);
                    material.map = tex;
                }
            } else if (aiGetMaterialTextureCount(mat, aiTextureType_DIFFUSE) > 0) {
                if (aiGetMaterialTexture(mat, aiTextureType_DIFFUSE, 0, &p) == aiReturn_SUCCESS) {
                    auto tex = loadTexture(aiScene, path, p.C_Str());
                    handleWrapping(mat, aiTextureType_DIFFUSE, *tex);
                    material.map = tex;
                }
            }


            // Base/Diffuse Color Values
            aiColor4D color;
            if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_BASE_COLOR, &color)) {
                // material.color.setRGB(1, 1, 1);
            } else if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &color)) {
                material.color.setRGB(color.r, color.g, color.b);
            }


            // Specular
            // if (aiGetMaterialTextureCount(mat, aiTextureType_SPECULAR) > 0) {
            //     if (aiGetMaterialTexture(mat, aiTextureType_SPECULAR, 0, &p) == aiReturn_SUCCESS) {
            //         auto tex = loadTexture(aiScene, path, p.C_Str());
            //         handleWrapping(mat, aiTextureType_SPECULAR, *tex);
            //         material.specularMap = tex;
            //     }
            // }


            // Emissive
            if (aiGetMaterialTextureCount(mat, aiTextureType_EMISSIVE) > 0 ||
                aiGetMaterialTextureCount(mat, aiTextureType_EMISSION_COLOR) > 0) {
                auto type = aiGetMaterialTextureCount(mat, aiTextureType_EMISSIVE) > 0 ?
                           aiTextureType_EMISSIVE : aiTextureType_EMISSION_COLOR;
                if (aiGetMaterialTexture(mat, type, 0, &p) == aiReturn_SUCCESS) {
                    auto tex = loadTexture(aiScene, path, p.C_Str());
                    handleWrapping(mat, type, *tex);
                    material.emissiveMap = tex;
                }
            }
            if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_EMISSIVE, &color)) {
                material.emissive.setRGB(color.r, color.g, color.b);
            }
            float emissiveIntensity;
            if (AI_SUCCESS == aiGetMaterialFloat(mat, AI_MATKEY_EMISSIVE_INTENSITY, &emissiveIntensity)) {
                material.emissiveIntensity = emissiveIntensity;
            }


            // Normal Maps
            if (aiGetMaterialTextureCount(mat, aiTextureType_NORMALS) > 0 ||
                aiGetMaterialTextureCount(mat, aiTextureType_NORMAL_CAMERA) > 0) {
                auto type = aiGetMaterialTextureCount(mat, aiTextureType_NORMALS) > 0 ?
                           aiTextureType_NORMALS : aiTextureType_NORMAL_CAMERA;
                if (aiGetMaterialTexture(mat, type, 0, &p) == aiReturn_SUCCESS) {
                    auto tex = loadTexture(aiScene, path, p.C_Str());
                    handleWrapping(mat, type, *tex);
                    material.normalMap = tex;
                }
            } else if (aiGetMaterialTextureCount(mat, aiTextureType_HEIGHT) > 0) {
                if (aiGetMaterialTexture(mat, aiTextureType_HEIGHT, 0, &p) == aiReturn_SUCCESS) {
                    auto tex = loadTexture(aiScene, path, p.C_Str());
                    handleWrapping(mat, aiTextureType_HEIGHT, *tex);
                    material.normalMap = tex;
                }
            }

            // PBR: Metalness & Roughness
            if (aiGetMaterialTextureCount(mat, aiTextureType_METALNESS) > 0) {
                if (aiGetMaterialTexture(mat, aiTextureType_METALNESS, 0, &p) == aiReturn_SUCCESS) {
                    auto tex = loadTexture(aiScene, path, p.C_Str());
                    handleWrapping(mat, aiTextureType_METALNESS, *tex);
                    material.metalnessMap = tex;
                }
            }
            if (aiGetMaterialTextureCount(mat, aiTextureType_DIFFUSE_ROUGHNESS) > 0) {
                if (aiGetMaterialTexture(mat, aiTextureType_DIFFUSE_ROUGHNESS, 0, &p) == aiReturn_SUCCESS) {
                    auto tex = loadTexture(aiScene, path, p.C_Str());
                    handleWrapping(mat, aiTextureType_DIFFUSE_ROUGHNESS, *tex);
                    material.roughnessMap = tex;
                }
            }

            float metallic, roughness;
            if (AI_SUCCESS == aiGetMaterialFloat(mat, AI_MATKEY_METALLIC_FACTOR, &metallic)) {
                material.metalness = metallic;
            }
            if (AI_SUCCESS == aiGetMaterialFloat(mat, AI_MATKEY_ROUGHNESS_FACTOR, &roughness)) {
                material.roughness = roughness;
            }


            // Opacity/Transparency/Transmission
            if (aiGetMaterialTextureCount(mat, aiTextureType_OPACITY) > 0) {
                if (aiGetMaterialTexture(mat, aiTextureType_OPACITY, 0, &p) == aiReturn_SUCCESS) {
                    auto tex = loadTexture(aiScene, path, p.C_Str());
                    handleWrapping(mat, aiTextureType_OPACITY, *tex);
                    material.alphaMap = tex;
                    material.transparent = true;
                }
            } else if (aiGetMaterialTextureCount(mat, aiTextureType_TRANSMISSION) > 0) {
                unsigned int numTextures = mat->GetTextureCount(aiTextureType_TRANSMISSION)-1;
                if (aiGetMaterialTexture(mat, aiTextureType_TRANSMISSION, numTextures, &p) == aiReturn_SUCCESS) {
                    auto tex = loadTexture(aiScene, path, p.C_Str());
                    handleWrapping(mat, aiTextureType_TRANSMISSION, *tex);
                    material.alphaMap = tex;
                    material.transparent = true;
                    material.map = tex;
                }
            }

            float opacity;
            if (AI_SUCCESS == aiGetMaterialFloat(mat, AI_MATKEY_OPACITY, &opacity)) {
                material.opacity = opacity;
                if (opacity < 1.0f) {
                    material.transparent = true;
                }
            }
        }


        std::shared_ptr<Texture> loadTexture(const aiScene* aiScene, const std::filesystem::path& path, const std::string& name) {

            std::shared_ptr<Texture> tex;

            if (name[0] == '*') {

                // embedded texture
                const auto embed = aiScene->GetEmbeddedTexture(name.c_str());

                std::stringstream ss;
                ss << embed->mFilename.C_Str() << "." << embed->achFormatHint;

                if (embed->mHeight == 0) {

                    std::vector<unsigned char> data(embed->mWidth);
                    std::copy((unsigned char*) embed->pcData, (unsigned char*) embed->pcData + data.size(), data.begin());
                    tex = texLoader_.loadFromMemory(ss.str(), data);

                } else {

                    std::vector<unsigned char> data(embed->mWidth * embed->mHeight);
                    std::copy((unsigned char*) embed->pcData, (unsigned char*) embed->pcData + data.size(), data.begin());
                    tex = texLoader_.loadFromMemory(ss.str(), data);
                }
            } else {

                auto texPath = path.parent_path() / name;
                tex = texLoader_.load(texPath);
            }

            return tex;
        }

        Matrix4 aiMatrixToMatrix4(const aiMatrix4x4& t) {
            Matrix4 m;
            m.set(t.a1, t.a2, t.a3, t.a4,
                  t.b1, t.b2, t.b3, t.b4,
                  t.c1, t.c2, t.c3, t.c4,
                  t.d1, t.d2, t.d3, t.d4);

            return m;
        }

        void setTransform(Object3D& obj, const aiMatrix4x4& t) {
            aiVector3t<float> pos;
            aiQuaterniont<float> quat;
            aiVector3t<float> scale;
            t.Decompose(scale, quat, pos);

            Matrix4 m;
            m.makeRotationFromQuaternion(Quaternion{quat.x, quat.y, quat.z, quat.w});
            m.setPosition({pos.x, pos.y, pos.z});

            obj.applyMatrix4(m);
            obj.scale.set(scale.x, scale.y, scale.z);
        }
    };

}// namespace threepp

#endif//THREEPP_ASSIMPLOADER_HPP
