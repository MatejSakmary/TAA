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
struct RuntimeSceneObject
{
    f32mat4x4 transform;
    std::vector<RuntimeMesh> meshes;
};

struct ProcessMeshInfo
{
    const aiMesh * mesh;
    const aiScene * scene;
    RuntimeSceneObject & object;
};

struct Scene
{
    f32vec3 light_position;

    std::vector<RuntimeSceneObject> runtime_scene_objects;

    explicit Scene(const std::string & scene_path);

    private:
        void process_scene(const aiScene * scene);
        void process_mesh(const ProcessMeshInfo & info);
        void convert_to_raytrace_scene();
};