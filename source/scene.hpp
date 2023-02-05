#pragma once

#include <string>
#include <vector>
#include <utility>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "types.hpp"
#include "utils.hpp"

struct Vertex
{
    f32vec3 position;
    f32vec3 normal;
};

struct RuntimeMesh
{
    std::vector<u32> indices;
    std::vector<Vertex> vertices;
};

// Scene object used in real time visualisation
struct SceneObject
{
    f32mat4x4 transform;
    std::vector<RuntimeMesh> meshes;
};

struct SceneLight
{
    f32vec4 position;
    f32mat4x4 transform;
};

struct ProcessMeshInfo
{
    const aiMesh * mesh;
    const aiScene * scene;
    SceneObject & object;
};

struct Scene
{
    std::vector<SceneObject> scene_objects;
    std::vector<SceneLight> scene_lights;

    explicit Scene(const std::string & scene_path);

    private:
        void process_scene(const aiScene * scene);
        void process_mesh(const ProcessMeshInfo & info);
        void convert_to_raytrace_scene();
};