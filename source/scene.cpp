#include "scene.hpp"

#include <stack>
#include <string>
#include <unordered_map>

void Scene::process_mesh(const ProcessMeshInfo & info)
{
    auto & new_mesh = info.object.meshes.emplace_back(RuntimeMesh{});
    new_mesh.vertices.reserve(info.mesh->mNumVertices);
    new_mesh.indices.reserve(static_cast<std::vector<u32>::size_type>(info.mesh->mNumFaces) * 3); // expect triangles
    for(u32 vertex = 0; vertex < info.mesh->mNumVertices; vertex++)
    {
        f32vec3 pre_transform_position{ 
            info.mesh->mVertices[vertex].x,
            info.mesh->mVertices[vertex].y,
            info.mesh->mVertices[vertex].z
        };
        f32vec3 pre_transform_normal{
            info.mesh->mNormals[vertex].x,
            info.mesh->mNormals[vertex].y,
            info.mesh->mNormals[vertex].z
        };

        // Runtime rendering data
        new_mesh.vertices.emplace_back(Vertex{
            .position = pre_transform_position,
            .normal = pre_transform_normal,
        });
    }

    f32mat4x4 m_model = info.object.transform;

    // NOTE(msakmary) I am assuming triangles here
    for(u32 face = 0; face < info.mesh->mNumFaces; face++)
    {
        aiFace face_obj = info.mesh->mFaces[face];

        new_mesh.indices.emplace_back(face_obj.mIndices[0]);
        new_mesh.indices.emplace_back(face_obj.mIndices[1]);
        new_mesh.indices.emplace_back(face_obj.mIndices[2]);
    }
}

void Scene::process_scene(const aiScene * scene)
{
    auto mat_assimp_to_glm = [](const aiMatrix4x4 & mat) -> f32mat4x4
    {
        return {{mat.a1, mat.b1, mat.c1, mat.d1},
                {mat.a2, mat.b2, mat.c2, mat.d2},
                {mat.a3, mat.b3, mat.c3, mat.d3},
                {mat.a4, mat.b4, mat.c4, mat.d4}};
    };

    // light name hash + it's index
    std::unordered_map<size_t, u32> lights;

    std::hash<std::string_view> hasher;
    for(int i = 0; i < scene->mNumLights; i++)
    {
        lights.emplace(hasher(std::string_view(scene->mLights[i]->mName.data, scene->mLights[i]->mName.length)), i);
    }

    using node_element = std::tuple<const aiNode *, aiMatrix4x4>; 
    std::stack<node_element> node_stack;

    node_stack.push({scene->mRootNode, aiMatrix4x4()});

    while(!node_stack.empty())
    {
        auto [node, parent_transform] = node_stack.top();
        auto node_transform = parent_transform * node->mTransformation;

        if(auto light_id = lights.find(hasher(std::string_view(node->mName.data, node->mName.length))); light_id != lights.end())
        {
            const auto & light = scene->mLights[(*light_id).second];
            auto & new_scene_light = scene_lights.emplace_back(SceneLight{
                .position = f32vec4{light->mPosition.x, light->mPosition.y, light->mPosition.z, 1.0f},
                .transform = mat_assimp_to_glm(node_transform)
            });
        }
        else if(node->mNumMeshes > 0)
        {
            auto & new_scene_object = scene_objects.emplace_back(SceneObject{
                .transform = mat_assimp_to_glm(node_transform)
            });

            for(u32 i = 0; i < node->mNumMeshes; i++)
            {
                process_mesh({
                    .mesh = scene->mMeshes[node->mMeshes[i]],
                    .scene = scene,
                    .object = new_scene_object
                });
            }
        }
        node_stack.pop();
        for(int i = 0; i < node->mNumChildren; i++)
        {
            auto *child = node->mChildren[i];
            node_stack.push({child, node_transform});
        }
    }
};

Scene::Scene(const std::string & scene_path)
{
    Assimp::Importer importer;
    const aiScene * scene = importer.ReadFile( 
        scene_path,
        0u 
        // aiProcess_Triangulate           |
        // aiProcess_JoinIdenticalVertices |
        // aiProcess_SortByPType
    );

    if((scene == nullptr) || ((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0u) || (scene->mRootNode == nullptr))
    {
        std::string err_string = importer.GetErrorString();
        DEBUG_OUT("[Scene::Scene()] Error assimp");
        DEBUG_OUT(err_string);
        return;
    }

    process_scene(scene);
}